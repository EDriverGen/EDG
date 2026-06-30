"""Driver synthesis executor."""
from __future__ import annotations

import dataclasses
import hashlib
import json
import logging
import re
import time
from typing import Any, List, Mapping, Optional, Sequence, Tuple

from ..llm.providers import BaseProvider, ProviderError
from .classify_device import ClassifyResult
from .ir_to_expected_transactions import ExpectedTransaction
from .route import RoutingResult
from .synthesis_schema import (
    contract_test_plan_schema_for,
    driver_code_schema,
    synthesis_schema_for,
    validate_contract_test_plan_response,
    validate_driver_code_response,
    validate_synthesis_response,
)
from .prompt_builder import (
    build_contract_test_plan_prompt,
    build_driver_code_prompt,
    build_synthesis_prompt,
)

logger = logging.getLogger(__name__)


# Data classes

@dataclasses.dataclass(frozen=True)
class SynthesisBundle:
    """Validated result of one synthesis call."""
    device_id: str
    rtos_id: str
    eval_class: str
    bus_kind: str

    driver_header: str
    driver_source: str
    api_contract: Mapping[str, Any]
    test_plan: Mapping[str, Any]

    # Echo routing and expected transactions for downstream consumers.
    runtime_path: str = ""
    slave_kind: str = ""
    spi_sub_mode: str = ""

    # Provenance / metadata
    attempt: int = 1
    model: str = ""
    provider_name: str = ""
    generation_time_s: float = 0.0
    prompt_chars: int = 0
    raw_response: str = ""
    plan_hash: str = ""

    def to_dict(self) -> dict:
        """Convert to a plain dict (safe for JSON serialisation)."""
        d = dataclasses.asdict(self)
        d["api_contract"] = dict(d["api_contract"])
        d["test_plan"] = dict(d["test_plan"])
        return d


@dataclasses.dataclass(frozen=True)
class PlanBundle:
    """Frozen split-stage output containing adapter contract and tests."""

    device_id: str
    rtos_id: str
    eval_class: str
    bus_kind: str
    api_contract: Mapping[str, Any]
    test_plan: Mapping[str, Any]
    plan_hash: str
    attempt: int = 1
    model: str = ""
    provider_name: str = ""
    generation_time_s: float = 0.0
    prompt_chars: int = 0
    raw_response: str = ""

    def to_dict(self) -> dict:
        d = dataclasses.asdict(self)
        d["api_contract"] = dict(d["api_contract"])
        d["test_plan"] = dict(d["test_plan"])
        return d


@dataclasses.dataclass(frozen=True)
class DriverCodeBundle:
    """Split-stage output containing driver files only."""

    device_id: str
    rtos_id: str
    eval_class: str
    bus_kind: str
    driver_header: str
    driver_source: str
    plan_hash: str
    attempt: int = 1
    model: str = ""
    provider_name: str = ""
    generation_time_s: float = 0.0
    prompt_chars: int = 0
    raw_response: str = ""

    def to_dict(self) -> dict:
        return dataclasses.asdict(self)


class SynthesisError(RuntimeError):
    """Raised when a synthesis call fails end-to-end."""

    def __init__(
        self,
        message: str,
        *,
        source: str,
        errors: Optional[Sequence[str]] = None,
        raw_response: Optional[str] = None,
    ) -> None:
        super().__init__(message)
        self.source = source
        self.errors: Tuple[str, ...] = tuple(errors or ())
        self.raw_response: Optional[str] = raw_response


# Response-parsing helpers

_FENCE_RE = re.compile(
    r"```(?:json|JSON)?\s*(\{.*?\})\s*```",
    re.DOTALL,
)
_FIRST_OBJECT_RE = re.compile(r"\{.*\}", re.DOTALL)

# Fallback repair for non-strict JSON after strict parsing fails.
_HEX_LITERAL_RE = re.compile(
    r"(?<=[:\[,])(\s*)(-?0[xX][0-9a-fA-F]+)"
)
_TRAILING_COMMA_RE = re.compile(r",(\s*[\]}])")


def _strip_string_segments(text: str) -> List[Tuple[int, int]]:
    """Return ``(start, end)`` index pairs covering JSON string literals."""
    spans: List[Tuple[int, int]] = []
    in_string = False
    escape = False
    start = -1
    for i, ch in enumerate(text):
        if in_string:
            if escape:
                escape = False
                continue
            if ch == "\\":
                escape = True
                continue
            if ch == '"':
                in_string = False
                spans.append((start, i + 1))
                start = -1
            continue
        if ch == '"':
            in_string = True
            start = i
    if in_string and start >= 0:
        spans.append((start, len(text)))
    return spans


def _is_inside_string(idx: int, spans: Sequence[Tuple[int, int]]) -> bool:
    for s, e in spans:
        if s <= idx < e:
            return True
        if s > idx:
            return False
    return False


def _relax_json_literals(text: str) -> str:
    """Apply forgiving repair passes for non-strict JSON output."""
    spans = _strip_string_segments(text)

    def _rewrite_hex(match: "re.Match[str]") -> str:
        # Preserve hex literals as strings for schema-typed byte fields.
        leading_ws = match.group(1)
        token = match.group(2)
        if _is_inside_string(match.start(2), spans):
            return match.group(0)
        return f'{leading_ws}"{token}"'

    rewritten = _HEX_LITERAL_RE.sub(_rewrite_hex, text)
    rewritten = _TRAILING_COMMA_RE.sub(lambda m: m.group(1), rewritten)
    return rewritten


def _parse_response_text(raw: str) -> Tuple[dict, str]:
    """Parse a free-form provider text response into a JSON dict."""
    if raw is None:
        raise SynthesisError(
            "provider returned None instead of a string response",
            source="empty",
            raw_response=None,
        )
    stripped = raw.strip()
    if not stripped:
        raise SynthesisError(
            "provider returned an empty response",
            source="empty",
            raw_response=raw,
        )

    try:
        parsed = json.loads(stripped)
        if isinstance(parsed, dict):
            return parsed, stripped
        raise SynthesisError(
            f"response parsed to {type(parsed).__name__}, expected object",
            source="parse",
            raw_response=raw,
        )
    except json.JSONDecodeError:
        pass

    fence_match = _FENCE_RE.search(stripped)
    candidate = fence_match.group(1) if fence_match else None
    if candidate is None:
        obj_match = _FIRST_OBJECT_RE.search(stripped)
        candidate = obj_match.group(0) if obj_match else None

    if candidate is None:
        raise SynthesisError(
            "response contains no parseable JSON object",
            source="parse",
            raw_response=raw,
        )

    parse_err: Optional[json.JSONDecodeError] = None
    try:
        parsed = json.loads(candidate)
    except json.JSONDecodeError as e:
        parse_err = e
        relaxed = _relax_json_literals(candidate)
        try:
            parsed = json.loads(relaxed)
        except json.JSONDecodeError:
            raise SynthesisError(
                f"embedded JSON failed to parse: {parse_err.msg} at line {parse_err.lineno}",
                source="parse",
                raw_response=raw,
            ) from parse_err
        candidate = relaxed
    if not isinstance(parsed, dict):
        raise SynthesisError(
            f"embedded JSON parsed to {type(parsed).__name__}, expected object",
            source="parse",
            raw_response=raw,
        )
    return parsed, candidate


def _try_generate_json(
    provider: BaseProvider,
    *,
    task_name: str,
    schema: Mapping[str, Any],
    system_prompt: str,
    user_prompt: str,
    metadata: Mapping[str, Any],
) -> Tuple[Optional[dict], Optional[str], Optional[str]]:
    """Call ``provider.generate_json`` defensively."""
    try:
        data = provider.generate_json(
            task_name, dict(schema), system_prompt, user_prompt, dict(metadata)
        )
    except NotImplementedError:
        return None, None, "provider does not implement generate_json"
    except ProviderError as e:
        return None, None, f"provider generate_json raised ProviderError: {e}"
    if not isinstance(data, Mapping):
        return None, None, (
            f"provider.generate_json returned {type(data).__name__}, "
            "expected mapping"
        )
    try:
        raw_text = json.dumps(data, ensure_ascii=False)
    except (TypeError, ValueError):
        raw_text = str(data)
    return dict(data), raw_text, None


def _plan_hash(api_contract: Mapping[str, Any], test_plan: Mapping[str, Any]) -> str:
    payload = {
        "api_contract": dict(api_contract),
        "test_plan": dict(test_plan),
    }
    blob = json.dumps(payload, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(blob.encode("utf-8")).hexdigest()[:16]


def _call_provider_payload(
    provider: BaseProvider,
    *,
    task_name: str,
    schema: Mapping[str, Any],
    system_prompt: str,
    user_prompt: str,
    metadata: Mapping[str, Any],
    prefer_json_mode: bool,
) -> Tuple[dict, str, float]:
    start = time.time()
    data: Optional[dict] = None
    raw_text: Optional[str] = None

    if prefer_json_mode:
        data, raw_text, fallback_reason = _try_generate_json(
            provider,
            task_name=task_name,
            schema=schema,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if fallback_reason:
            logger.info("Synthesis falling back to generate_text: %s", fallback_reason)

    if data is None:
        try:
            raw_text = provider.generate_text(system_prompt, user_prompt, dict(metadata))
        except ProviderError as e:
            raise SynthesisError(
                f"provider.generate_text failed: {e}",
                source="provider",
                raw_response=None,
            ) from e
        except Exception as e:   # pragma: no cover - defensive
            raise SynthesisError(
                f"provider.generate_text raised {type(e).__name__}: {e}",
                source="provider",
                raw_response=None,
            ) from e
        data, cleaned = _parse_response_text(raw_text or "")
        raw_text = cleaned

    return data, raw_text or "", time.time() - start


# Public API

def generate_synthesis(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
    attempt: int = 1,
    task_name: str = "synthesis",
    extra_metadata: Optional[Mapping[str, Any]] = None,
    prefer_json_mode: bool = True,
) -> SynthesisBundle:
    """Execute driver synthesis against ``provider``."""
    device_id = str(device_ir.get("device_id") or "").strip() or "unknown"
    rtos_id = str(rtos_contract.get("rtos") or "").strip() or "unknown"
    eval_class = classify_result.eval_class
    bus_kind = routing.bus_kind or classify_result.bus_type

    system_prompt, user_prompt = build_synthesis_prompt(
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        expected_transactions=expected_transactions,
        artifact=artifact,
        channel_alias_map=channel_alias_map,
        output_semantics_map=output_semantics_map,
        prior_feedback=prior_feedback,
    )
    prompt_chars = len(system_prompt) + len(user_prompt)

    metadata = {
        "device_id": device_id,
        "rtos_id": rtos_id,
        "eval_class": eval_class,
        "bus_kind": bus_kind,
        "attempt": attempt,
        "stage": "synthesis",
    }
    if extra_metadata:
        for k, v in extra_metadata.items():
            metadata.setdefault(k, v)

    schema = synthesis_schema_for(eval_class)
    logger.info(
        "Synthesis: device=%s rtos=%s eval_class=%s bus=%s attempt=%d prompt_chars=%d",
        device_id, rtos_id, eval_class, bus_kind, attempt, prompt_chars,
    )

    start = time.time()
    data: Optional[dict] = None
    raw_text: Optional[str] = None

    if prefer_json_mode:
        data, raw_text, fallback_reason = _try_generate_json(
            provider,
            task_name=task_name,
            schema=schema,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if fallback_reason:
            logger.info(
                "Synthesis falling back to generate_text: %s", fallback_reason,
            )

    if data is None:
        try:
            raw_text = provider.generate_text(system_prompt, user_prompt, dict(metadata))
        except ProviderError as e:
            raise SynthesisError(
                f"provider.generate_text failed: {e}",
                source="provider",
                raw_response=None,
            ) from e
        except Exception as e:   # pragma: no cover - defensive
            raise SynthesisError(
                f"provider.generate_text raised {type(e).__name__}: {e}",
                source="provider",
                raw_response=None,
            ) from e
        data, cleaned = _parse_response_text(raw_text or "")
        raw_text = cleaned

    elapsed = time.time() - start

    ok, errors = validate_synthesis_response(data, eval_class)
    if not ok:
        raise SynthesisError(
            f"response failed synthesis schema ({len(errors)} error(s))",
            source="schema",
            errors=errors,
            raw_response=raw_text,
        )

    bundle = SynthesisBundle(
        device_id=device_id,
        rtos_id=rtos_id,
        eval_class=eval_class,
        bus_kind=bus_kind,
        driver_header=str(data.get("driver_header", "")),
        driver_source=str(data.get("driver_source", "")),
        api_contract=dict(data.get("api_contract", {})),
        test_plan=dict(data.get("test_plan", {})),
        runtime_path=routing.runtime_path,
        slave_kind=routing.slave_kind,
        spi_sub_mode=routing.spi_sub_mode,
        attempt=attempt,
        model=getattr(provider, "model", "") or "",
        provider_name=getattr(provider, "name", "") or "",
        generation_time_s=round(elapsed, 3),
        prompt_chars=prompt_chars,
        raw_response=raw_text or "",
    )
    logger.info(
        "Synthesis ok: device=%s attempt=%d time=%.2fs model=%s",
        device_id, attempt, elapsed, bundle.model,
    )
    return bundle


def generate_contract_test_plan(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
    attempt: int = 1,
    task_name: str = "contract_test_plan",
    extra_metadata: Optional[Mapping[str, Any]] = None,
    prefer_json_mode: bool = True,
) -> PlanBundle:
    """Generate and schema-validate the split frozen plan."""
    device_id = str(device_ir.get("device_id") or "").strip() or "unknown"
    rtos_id = str(rtos_contract.get("rtos") or "").strip() or "unknown"
    eval_class = classify_result.eval_class
    bus_kind = routing.bus_kind or classify_result.bus_type

    system_prompt, user_prompt = build_contract_test_plan_prompt(
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        expected_transactions=expected_transactions,
        artifact=artifact,
        channel_alias_map=channel_alias_map,
        output_semantics_map=output_semantics_map,
        prior_feedback=prior_feedback,
    )
    metadata = {
        "device_id": device_id,
        "rtos_id": rtos_id,
        "eval_class": eval_class,
        "bus_kind": bus_kind,
        "attempt": attempt,
        "stage": "contract_test_plan",
    }
    if extra_metadata:
        for k, v in extra_metadata.items():
            metadata.setdefault(k, v)

    schema = contract_test_plan_schema_for(eval_class)
    data, raw_text, elapsed = _call_provider_payload(
        provider,
        task_name=task_name,
        schema=schema,
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        metadata=metadata,
        prefer_json_mode=prefer_json_mode,
    )
    ok, errors = validate_contract_test_plan_response(data, eval_class)
    if not ok:
        raise SynthesisError(
            f"response failed CONTRACT_TEST_PLAN_SCHEMA ({len(errors)} error(s))",
            source="schema",
            errors=errors,
            raw_response=raw_text,
        )

    api_contract = dict(data.get("api_contract", {}))
    test_plan = dict(data.get("test_plan", {}))
    ph = _plan_hash(api_contract, test_plan)
    return PlanBundle(
        device_id=device_id,
        rtos_id=rtos_id,
        eval_class=eval_class,
        bus_kind=bus_kind,
        api_contract=api_contract,
        test_plan=test_plan,
        plan_hash=ph,
        attempt=attempt,
        model=getattr(provider, "model", "") or "",
        provider_name=getattr(provider, "name", "") or "",
        generation_time_s=round(elapsed, 3),
        prompt_chars=len(system_prompt) + len(user_prompt),
        raw_response=raw_text,
    )


def generate_driver_code(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    frozen_plan: Mapping[str, Any],
    plan_hash: str,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
    repair_context: Optional[Mapping[str, Any]] = None,
    attempt: int = 1,
    task_name: str = "driver_code",
    extra_metadata: Optional[Mapping[str, Any]] = None,
    prefer_json_mode: bool = True,
) -> DriverCodeBundle:
    """Generate driver files against a frozen plan."""
    device_id = str(device_ir.get("device_id") or "").strip() or "unknown"
    rtos_id = str(rtos_contract.get("rtos") or "").strip() or "unknown"
    eval_class = classify_result.eval_class
    bus_kind = routing.bus_kind or classify_result.bus_type

    system_prompt, user_prompt = build_driver_code_prompt(
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        frozen_plan=frozen_plan,
        expected_transactions=expected_transactions,
        artifact=artifact,
        channel_alias_map=channel_alias_map,
        output_semantics_map=output_semantics_map,
        prior_feedback=prior_feedback,
        repair_context=repair_context,
    )
    metadata = {
        "device_id": device_id,
        "rtos_id": rtos_id,
        "eval_class": eval_class,
        "bus_kind": bus_kind,
        "attempt": attempt,
        "stage": "driver_code",
        "plan_hash": plan_hash,
    }
    if extra_metadata:
        for k, v in extra_metadata.items():
            metadata.setdefault(k, v)

    data, raw_text, elapsed = _call_provider_payload(
        provider,
        task_name=task_name,
        schema=driver_code_schema(),
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        metadata=metadata,
        prefer_json_mode=prefer_json_mode,
    )
    ok, errors = validate_driver_code_response(data)
    if not ok:
        raise SynthesisError(
            f"response failed DRIVER_CODE_SCHEMA ({len(errors)} error(s))",
            source="schema",
            errors=errors,
            raw_response=raw_text,
        )

    return DriverCodeBundle(
        device_id=device_id,
        rtos_id=rtos_id,
        eval_class=eval_class,
        bus_kind=bus_kind,
        driver_header=str(data.get("driver_header", "")),
        driver_source=str(data.get("driver_source", "")),
        plan_hash=plan_hash,
        attempt=attempt,
        model=getattr(provider, "model", "") or "",
        provider_name=getattr(provider, "name", "") or "",
        generation_time_s=round(elapsed, 3),
        prompt_chars=len(system_prompt) + len(user_prompt),
        raw_response=raw_text,
    )


__all__ = [
    "DriverCodeBundle",
    "PlanBundle",
    "SynthesisBundle",
    "SynthesisError",
    "generate_contract_test_plan",
    "generate_driver_code",
    "generate_synthesis",
]
