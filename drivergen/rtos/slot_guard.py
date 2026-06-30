"""Generic slot-fit guards for RTOS symbol selection."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Iterable

from .types import RankedSymbolCandidate, SlotGoal, SymbolSketch


PARSED_SYMBOL_KINDS: frozenset[str] = frozenset(
    {"function", "macro", "typedef", "struct", "enum"}
)

_SPLIT_RE = re.compile(r"[^A-Za-z0-9]+")

_EXAMPLE_TOKENS = {
    "example",
    "examples",
    "demo",
    "demos",
    "sample",
    "samples",
    "test",
    "tests",
    "testing",
}
_TEMPLATE_TOKENS = {"template", "templates"}
_DOC_TOKENS = {"doc", "docs", "documentation", "readme"}
_BOARD_TOKENS = {"board", "boards", "bsp", "platform", "platforms", "target", "targets"}
_PORT_TOKENS = {"port", "ports", "arch", "cpu", "soc"}
_PRIVATE_LOW_LEVEL_TOKENS = {
    "ll",
    "lld",
    "lowlevel",
    "low_level",
    "private",
    "internal",
}
_INTEGRATION_STOPWORDS = {
    "board",
    "bus",
    "binding",
    "instance",
    "pin",
    "port",
    "device",
    "default",
    "slot",
    "line",
    "mode",
    "config",
    "gpio",
    "i2c",
    "spi",
    "uart",
}
_STATUS_SUCCESS_TOKENS = {
    "ok",
    "eok",
    "success",
    "succeeded",
    "no_error",
    "none",
}
_STATUS_BRANCH_RISK_TOKENS = {
    "busy",
    "timeout",
    "timedout",
    "errno",
    "fault",
    "invalid",
    "overflow",
    "underflow",
    "abort",
    "fail",
    "failed",
    "flag",
    "mask",
    "msk",
    "state",
}


@dataclass(frozen=True)
class CandidateAssessment:
    """Result of checking one candidate against one slot."""

    hard_reject: bool = False
    reasons: tuple[str, ...] = ()
    score_multiplier: float = 1.0
    tags: frozenset[str] = field(default_factory=frozenset)

    @property
    def prompt_note(self) -> str:
        parts: list[str] = []
        if self.tags:
            parts.append("tags=" + ",".join(sorted(self.tags)))
        if self.reasons:
            parts.append("fit=" + ",".join(self.reasons))
        if self.score_multiplier != 1.0:
            parts.append(f"score_multiplier={self.score_multiplier:.2f}")
        if self.hard_reject:
            parts.append("not_eligible=true")
        return "; ".join(parts) or "fit=normal"


def expected_parsed_kinds(slot: SlotGoal | None) -> frozenset[str]:
    """Return parsed symbol kinds that may satisfy ``slot``."""
    if slot is None:
        return frozenset()
    return frozenset(k for k in (slot.expected_kinds or []) if k in PARSED_SYMBOL_KINDS)


def kind_allowed_for_slot(slot: SlotGoal | None, kind: str | None) -> bool:
    allowed = expected_parsed_kinds(slot)
    if not allowed:
        return True
    return bool(kind and kind in allowed)


def _tokens_from_path(path: str) -> list[str]:
    return [t.lower() for t in _SPLIT_RE.split(path or "") if t]


def provenance_tags(
    *,
    path: str,
    file_kind: str | None = None,
    dir_role_hint: str | None = None,
) -> frozenset[str]:
    """Classify source provenance using generic path/file metadata."""
    tokens = set(_tokens_from_path(path))
    tags: set[str] = set()

    fk = (file_kind or "").strip().lower()
    role = (dir_role_hint or "").strip().lower()
    if fk:
        tags.add(f"file:{fk}")
    if role:
        tags.add(f"role:{role}")

    if fk in {"doc"} or tokens & _DOC_TOKENS or role == "docs":
        tags.add("docs")
    if fk == "example" or tokens & _EXAMPLE_TOKENS or role in {"demo", "exemplar"}:
        tags.add("example")
    if tokens & _TEMPLATE_TOKENS:
        tags.add("template")
    if fk in {"config", "build"}:
        tags.add("config")
    if tokens & _BOARD_TOKENS or role in {"board", "board_integration"}:
        tags.add("board_specific")
    if tokens & _PORT_TOKENS:
        tags.add("port_specific")
    if tokens & _PRIVATE_LOW_LEVEL_TOKENS:
        tags.add("private_low_level")
    if tokens & {"include", "includes", "inc", "api"}:
        tags.add("public_include")

    return frozenset(tags)


def _slot_wants_low_level(slot: SlotGoal | None) -> bool:
    if slot is None:
        return False
    text = " ".join([slot.slot_id, *(slot.query_intents or [])]).lower()
    return any(t in text for t in ("low level", "low-level", "lld", " ll ", "_ll"))


def _slot_text(slot: SlotGoal | None, sym: SymbolSketch) -> str:
    if slot is None:
        return " ".join([sym.name, sym.signature or "", sym.file]).lower()
    return " ".join(
        [
            slot.slot_id,
            *(slot.query_intents or []),
            sym.name,
            sym.signature or "",
            sym.file,
        ]
    ).lower()


def _norm_identifier(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "_", value or "").strip("_").lower()


def _identifier_tokens(value: str) -> set[str]:
    out = {t.lower() for t in _SPLIT_RE.split(value or "") if t}
    norm = _norm_identifier(value)
    if norm:
        out.add(norm)
    return out


def _looks_status_type_symbol(sym: SymbolSketch) -> bool:
    name = sym.name or ""
    lowered = name.lower()
    if lowered.endswith("_t"):
        return True
    return any(marker in lowered for marker in ("status", "error", "err", "result", "msg"))


def _apply_error_status_policy(slot: SlotGoal | None, sym: SymbolSketch) -> tuple[bool, float, list[str]]:
    """Constrain runtime.error_status to reusable status evidence."""
    if slot is None or slot.slot_id.lower() != "runtime.error_status":
        return False, 1.0, []

    kind = (sym.kind or "").lower()
    name = sym.name or ""
    tokens = _identifier_tokens(name)
    reasons: list[str] = []

    if kind in {"typedef", "enum"} and _looks_status_type_symbol(sym):
        return False, 1.0, []

    if kind == "macro":
        if tokens & _STATUS_SUCCESS_TOKENS or any(t.endswith("_ok") for t in tokens):
            return False, 1.0, []
        if tokens & _STATUS_BRANCH_RISK_TOKENS:
            reasons.append("status-branch-constant-not-slot-answer")
            return True, 0.2, reasons
        # Other error/status macros may still be useful, but as weaker
        # evidence than a return type or canonical success value.
        if any(marker in _norm_identifier(name) for marker in ("status", "error", "err")):
            reasons.append("status-macro-prefers-multi-symbol-evidence")
            return False, 0.55, reasons

    if kind == "enum" and tokens & _STATUS_BRANCH_RISK_TOKENS:
        reasons.append("status-branch-constant-not-slot-answer")
        return True, 0.2, reasons

    return False, 1.0, []


def _apply_bus_primitive_policy(slot: SlotGoal | None, sym: SymbolSketch) -> tuple[bool, float, list[str]]:
    if slot is None:
        return False, 1.0, []
    slot_id = slot.slot_id.lower()
    if not slot_id.startswith(("i2c.", "spi.", "uart.", "gpio.")):
        return False, 1.0, []
    if not slot_id.endswith((".read", ".write", ".transfer", ".acquire_or_bind")):
        return False, 1.0, []
    if (sym.kind or "").lower() != "function":
        return False, 1.0, []

    name = _norm_identifier(sym.name)
    tokens = _identifier_tokens(sym.name)
    if (
        name.endswith("_cb")
        or "_cb_" in name
        or "callback" in tokens
        or "handler" in tokens
        or name.endswith("_handler")
        or "signal_cb" in name
    ):
        return True, 0.2, ["bus-primitive-callback-or-handler"]
    return False, 1.0, []


def _integration_context_tokens(slot: SlotGoal | None) -> set[str]:
    """Extract exact task-context tokens from integration slot intents."""
    if slot is None:
        return set()
    out: set[str] = set()
    for intent in slot.query_intents or []:
        raw_intent = str(intent)
        norm = _norm_identifier(raw_intent)
        if not norm:
            continue
        if norm not in _INTEGRATION_STOPWORDS:
            out.add(norm)
        for piece in re.split(r"[:;,=\s]+", raw_intent):
            npiece = _norm_identifier(piece)
            if npiece and npiece not in _INTEGRATION_STOPWORDS:
                out.add(npiece)
        for tok in _tokens_from_path(str(intent)):
            ntok = _norm_identifier(tok)
            if not ntok or ntok in _INTEGRATION_STOPWORDS:
                continue
            # Single-letter-ish generic tokens are too broad, but
            # identifiers like GPIOA / I2C1 are meaningful context.
            if len(ntok) >= 4 or any(ch.isdigit() for ch in ntok):
                out.add(ntok)
    return out


def integration_symbol_matches_context(slot: SlotGoal | None, sym: SymbolSketch) -> bool:
    """True iff a repo symbol exactly matches an integration context token."""
    tokens = _integration_context_tokens(slot)
    if not tokens:
        return False
    name = _norm_identifier(sym.name)
    return name in tokens


def _apply_delay_unit_penalty(slot: SlotGoal | None, sym: SymbolSketch) -> tuple[float, list[str]]:
    if slot is None:
        return 1.0, []
    slot_id = slot.slot_id.lower()
    text = _slot_text(slot, sym)
    name = (sym.name or "").lower()
    reasons: list[str] = []
    mult = 1.0

    if slot_id.endswith("delay_ms"):
        micro_cues = ("udelay", "usec", "micro", "delay_us", "us_delay")
        cycle_cues = ("cycle", "cycles", "busy", "polled", "spin")
        if any(c in text for c in micro_cues) or any(c in name for c in cycle_cues):
            mult *= 0.25
            reasons.append("delay-ms-unit-risk")
    elif slot_id.endswith("delay_us"):
        milli_cues = ("mdelay", "msec", "milli", "delay_ms", "ms_delay")
        if any(c in text for c in milli_cues):
            mult *= 0.25
            reasons.append("delay-us-unit-risk")

    return mult, reasons


def assess_symbol_fit(
    slot: SlotGoal | None,
    candidate: RankedSymbolCandidate | SymbolSketch,
    *,
    file_kind: str | None = None,
    dir_role_hint: str | None = None,
) -> CandidateAssessment:
    """Assess whether ``candidate`` is eligible and how risky it is."""
    sym = candidate.sketch if isinstance(candidate, RankedSymbolCandidate) else candidate
    tags = provenance_tags(
        path=sym.file,
        file_kind=file_kind if file_kind is not None else getattr(sym, "file_kind", None),
        dir_role_hint=(
            dir_role_hint if dir_role_hint is not None else getattr(sym, "dir_role_hint", None)
        ),
    )
    reasons: list[str] = []
    hard_reject = False
    mult = 1.0

    if not kind_allowed_for_slot(slot, sym.kind):
        hard_reject = True
        reasons.append(f"kind-not-expected:{sym.kind}")

    layer = (slot.layer if slot else "").lower()
    slot_id = (slot.slot_id if slot else "").lower()
    is_authoritative_api_slot = (
        layer in {"bus", "runtime", "timing", "integration", "task_helper"}
        or slot_id.startswith(("i2c.", "spi.", "uart.", "gpio.", "runtime.", "timing.", "integration."))
    )

    non_authoritative = tags & {"docs", "example", "template"}
    if is_authoritative_api_slot and non_authoritative:
        hard_reject = True
        reasons.append("non-authoritative-source:" + "+".join(sorted(non_authoritative)))

    if is_authoritative_api_slot and "config" in tags and layer != "integration":
        hard_reject = True
        reasons.append("config-not-api-source")

    if layer == "integration":
        # Integration-layer symbols must match existing task context.
        if not integration_symbol_matches_context(slot, sym):
            hard_reject = True
            reasons.append("integration-symbol-not-in-task-context")
        mult *= 0.45
        reasons.append("integration-prefers-task-context")

    if "private_low_level" in tags and not _slot_wants_low_level(slot):
        mult *= 0.35
        reasons.append("private-low-level-source")

    if "port_specific" in tags and not _slot_wants_low_level(slot):
        mult *= 0.55
        reasons.append("port-specific-source")

    delay_mult, delay_reasons = _apply_delay_unit_penalty(slot, sym)
    if delay_mult != 1.0:
        mult *= delay_mult
        reasons.extend(delay_reasons)

    status_reject, status_mult, status_reasons = _apply_error_status_policy(slot, sym)
    if status_reject:
        hard_reject = True
    if status_mult != 1.0:
        mult *= status_mult
    reasons.extend(status_reasons)

    bus_reject, bus_mult, bus_reasons = _apply_bus_primitive_policy(slot, sym)
    if bus_reject:
        hard_reject = True
    if bus_mult != 1.0:
        mult *= bus_mult
    reasons.extend(bus_reasons)

    return CandidateAssessment(
        hard_reject=hard_reject,
        reasons=tuple(reasons),
        score_multiplier=mult,
        tags=tags,
    )


def slot_policy_prompt(slot: SlotGoal) -> str:
    """Human-readable policy block for LLM prompts."""
    allowed = sorted(expected_parsed_kinds(slot))
    allowed_text = ", ".join(allowed) if allowed else "any parsed symbol kind"
    lines = [
        f"Allowed parsed kinds for this slot: {allowed_text}.",
        "Do not bind symbols from docs/examples/templates as authoritative RTOS APIs.",
        "Prefer public API/include declarations over private low-level or port-specific internals unless the slot explicitly asks for low-level internals.",
    ]
    if (slot.layer or "").lower() == "integration":
        lines.append(
            "Integration binding slots should prefer task/board context values; parsed repo symbols are eligible only when their names exactly match task context tokens and must not be functions."
        )
    if slot.slot_id.lower().endswith("delay_ms"):
        lines.append("For delay_ms slots, prefer millisecond sleep/delay APIs; avoid microsecond, busy-wait, cycle, or polled-delay APIs.")
    if slot.slot_id.lower().endswith("delay_us"):
        lines.append("For delay_us slots, prefer microsecond delay/timer helpers; avoid millisecond sleep APIs.")
    if slot.slot_id.lower() == "runtime.error_status":
        lines.append(
            "For runtime.error_status, prefer API return/status types or canonical success/no-error values; avoid binding concrete timeout/busy/errno branch constants as the single slot answer."
        )
    return "\n".join(f"- {line}" for line in lines)


def filter_eligible_candidates(
    slot: SlotGoal | None,
    candidates: Iterable[RankedSymbolCandidate],
) -> tuple[list[RankedSymbolCandidate], list[tuple[RankedSymbolCandidate, CandidateAssessment]]]:
    """Split candidates into eligible and rejected lists."""
    eligible: list[RankedSymbolCandidate] = []
    rejected: list[tuple[RankedSymbolCandidate, CandidateAssessment]] = []
    for cand in candidates:
        assessment = assess_symbol_fit(slot, cand)
        if assessment.hard_reject:
            rejected.append((cand, assessment))
            continue
        eligible.append(cand)
    return eligible, rejected


__all__ = [
    "CandidateAssessment",
    "PARSED_SYMBOL_KINDS",
    "assess_symbol_fit",
    "expected_parsed_kinds",
    "filter_eligible_candidates",
    "kind_allowed_for_slot",
    "provenance_tags",
    "integration_symbol_matches_context",
    "slot_policy_prompt",
]
