"""pipeline step - LLM call infrastructure for the RTOS pipeline."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import Any

from .config import load_thresholds

logger = logging.getLogger(__name__)


# Token budgets


@dataclass
class TokenBudget:
    """Configured cap for one LLM call kind (Directory Router, File
    Selector, Symbol Binder, Gap Diagnoser, Transaction Translator).
    Loaded from ``thresholds.json/llm_budget.per_call.<kind>``.
    """

    input_tokens_max: int
    output_tokens_max: int
    max_per_round: int = 1
    max_per_task: int = 999


@dataclass
class BudgetTracker:
    """Per-task accounting for LLM calls + token usage."""

    max_calls_per_task: int
    max_input_tokens_per_task: int
    max_output_tokens_per_task: int

    calls_by_kind: dict[str, int] = field(default_factory=dict)
    input_tokens_total: int = 0
    output_tokens_total: int = 0

    def can_afford(self, call_kind: str, est_input: int, est_output: int) -> tuple[bool, str]:
        """Return ``(ok, reason)`` — ``ok=False`` means refuse the call."""
        total_calls = sum(self.calls_by_kind.values())
        if total_calls >= self.max_calls_per_task:
            return False, f"max_calls_per_task={self.max_calls_per_task} reached"
        if self.input_tokens_total + est_input > self.max_input_tokens_per_task:
            return False, (
                f"max_input_tokens_per_task={self.max_input_tokens_per_task} "
                f"would be exceeded ({self.input_tokens_total}+{est_input})"
            )
        if self.output_tokens_total + est_output > self.max_output_tokens_per_task:
            return False, (
                f"max_output_tokens_per_task={self.max_output_tokens_per_task} "
                f"would be exceeded ({self.output_tokens_total}+{est_output})"
            )
        return True, ""

    def record(self, call_kind: str, input_tokens: int, output_tokens: int) -> None:
        self.calls_by_kind[call_kind] = self.calls_by_kind.get(call_kind, 0) + 1
        self.input_tokens_total += input_tokens
        self.output_tokens_total += output_tokens

    def to_dict(self) -> dict:
        """Snapshot suitable for inclusion in the ExtractionLedger."""
        return {
            "calls_by_kind": dict(self.calls_by_kind),
            "input_tokens_total": self.input_tokens_total,
            "output_tokens_total": self.output_tokens_total,
            "limits": {
                "max_calls_per_task": self.max_calls_per_task,
                "max_input_tokens_per_task": self.max_input_tokens_per_task,
                "max_output_tokens_per_task": self.max_output_tokens_per_task,
            },
        }


def estimate_tokens(text: str) -> int:
    """Char-count proxy for tokens."""
    return max(len(text or "") // 4, 1)


def make_budget_tracker(*, mode: str = "default") -> BudgetTracker:
    """Build a :class:`BudgetTracker` from ``thresholds.json``."""
    cfg = load_thresholds()
    llm_cfg = cfg.get("llm_budget", {})
    if mode == "exhaustive":
        max_calls = llm_cfg.get(
            "max_calls_per_task_exhaustive",
            llm_cfg.get("max_calls_per_task_default", 25),
        )
    else:
        max_calls = llm_cfg.get("max_calls_per_task_default", 10)
    return BudgetTracker(
        max_calls_per_task=int(max_calls),
        max_input_tokens_per_task=int(llm_cfg.get("max_input_tokens_per_task_default", 40000)),
        max_output_tokens_per_task=int(llm_cfg.get("max_output_tokens_per_task_default", 6000)),
    )


def get_call_budget(call_kind: str) -> TokenBudget:
    """Return per-call-kind input/output caps from ``thresholds.json/llm_budget.per_call.<kind>``."""
    cfg = load_thresholds()
    llm_cfg = cfg.get("llm_budget", {})
    per_call = llm_cfg.get("per_call", {})
    spec = per_call.get(call_kind, {})
    if not spec:
        logger.warning("No per_call config for kind=%s; using safe defaults", call_kind)

    # Different call kinds use different field names (e.g. symbol_binder
    # is configured per-slot), so accept either spelling.
    input_tokens = (
        spec.get("input_tokens")
        or spec.get("input_tokens_per_slot")
        or 4000
    )
    output_tokens = (
        spec.get("output_tokens")
        or spec.get("output_tokens_per_slot")
        or 700
    )
    max_per_round = int(spec.get("max_per_round", spec.get("max_slots_per_call", 1)))
    max_per_task = int(spec.get("max_per_task", 999))
    return TokenBudget(
        input_tokens_max=int(input_tokens),
        output_tokens_max=int(output_tokens),
        max_per_round=max_per_round,
        max_per_task=max_per_task,
    )


# Single-shot LLM call wrapper


def call_llm_json(
    *,
    provider: Any | None,
    call_kind: str,
    task_name: str,
    schema: dict,
    system_prompt: str,
    user_prompt: str,
    budget: BudgetTracker,
    metadata: dict | None = None,
) -> tuple[dict | None, dict]:
    """Single LLM JSON call, gated by *budget* and recorded into it."""
    if provider is None:
        return None, {"skipped": True, "reason": "no_provider"}

    est_input = estimate_tokens(system_prompt) + estimate_tokens(user_prompt)
    # Use the configured output cap as the conservative estimate.
    est_output = get_call_budget(call_kind).output_tokens_max

    ok, reason = budget.can_afford(call_kind, est_input, est_output)
    if not ok:
        logger.warning("LLM call %s skipped: %s", call_kind, reason)
        return None, {"skipped": True, "reason": reason}

    try:
        payload = provider.generate_json(
            task_name=task_name,
            schema=schema,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata or {},
        )
    except Exception as exc:  # ProviderError or transport
        logger.warning("LLM call %s failed: %s", call_kind, exc)
        return None, {"skipped": True, "reason": f"provider_error: {exc}"}

    budget.record(call_kind, est_input, est_output)
    return payload, {
        "skipped": False,
        "input_tokens_est": est_input,
        "output_tokens_est": est_output,
    }


__all__ = [
    "TokenBudget",
    "BudgetTracker",
    "estimate_tokens",
    "make_budget_tracker",
    "get_call_budget",
    "call_llm_json",
]
