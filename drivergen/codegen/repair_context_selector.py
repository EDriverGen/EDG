"""Failure-aware context selection for the repair loop."""
from __future__ import annotations

import re
from collections.abc import Mapping, Sequence
from typing import Any


COMPILE_SYMBOL_RE = re.compile(
    r"(?:undefined reference to|implicit declaration of function|"
    r"undeclared|unresolved symbol|missing symbol)\s+[`'\"]?"
    r"([A-Za-z_][A-Za-z0-9_]*)",
    re.IGNORECASE,
)
MISSING_HEADER_RE = re.compile(
    r"(?:fatal\s+)?error:\s*[\"<']?([^\"<>'\s:]+\.h)[\"<>']?:"
    r"\s*No such file or directory",
    re.IGNORECASE,
)


def _stable_add(items: list[str], item: str) -> None:
    item = str(item or "").strip()
    if item and item not in items:
        items.append(item)


def _as_mapping(value: Any) -> Mapping[str, Any]:
    return value if isinstance(value, Mapping) else {}


def _compile_terms(errors: Sequence[str], adapter_error: str | None) -> tuple[str, ...]:
    terms: list[str] = []
    for text in list(errors or ()) + ([adapter_error] if adapter_error else []):
        if not text:
            continue
        for match in COMPILE_SYMBOL_RE.finditer(str(text)):
            _stable_add(terms, match.group(1))
        for match in MISSING_HEADER_RE.finditer(str(text)):
            _stable_add(terms, match.group(1))
    return tuple(terms)


def _coverage_has_gaps(report: Any) -> bool:
    return bool(getattr(report, "missing", None) or getattr(report, "llm_extras", None))


def _stim_expected_error(stim: Any) -> bool:
    expected = getattr(stim, "expected_err", None)
    try:
        return expected is not None and int(expected) != 0
    except (TypeError, ValueError):
        return False


def _failing_probe_names(probe_outcomes: Sequence[Any], probe_stimuli: Sequence[Any]) -> tuple[str, ...]:
    stim_map = {
        str(getattr(stim, "name", "") or ""): stim
        for stim in probe_stimuli or ()
    }
    names: list[str] = []
    for outcome in probe_outcomes or ():
        name = str(getattr(outcome, "stimulus_name", "") or "").strip()
        stim = stim_map.get(name)
        failed = bool(getattr(outcome, "error", ""))
        failed = failed or not bool(getattr(outcome, "boot_detected", False))
        failed = failed or not bool(getattr(outcome, "test_done", False))
        if _stim_expected_error(stim):
            failed = failed or bool(getattr(outcome, "result_pass", False))
        else:
            failed = failed or not bool(getattr(outcome, "result_pass", False))
        if failed:
            _stable_add(names, name)
    return tuple(names)


def _infer_focus_tags(
    *,
    layer_failed: str,
    synthesis_error: Any,
    compile_errors: Sequence[str],
    adapter_error: str | None,
    consistency_report: Any,
    coverage_report: Any,
    probe_outcomes: Sequence[Any],
    probe_stimuli: Sequence[Any],
) -> tuple[str, ...]:
    tags: list[str] = []
    layer = (layer_failed or "").lower()

    if synthesis_error is not None:
        _stable_add(tags, "output-schema")
        _stable_add(tags, "frozen-plan")

    if layer in {"syntax", "link"} or compile_errors or adapter_error:
        _stable_add(tags, "rtos-api")
        _stable_add(tags, "headers")
        _stable_add(tags, "api-contract")

    if _coverage_has_gaps(coverage_report):
        _stable_add(tags, "protocol")
        _stable_add(tags, "expected-transactions")

    for stim in getattr(consistency_report, "stimuli", ()) or ():
        verdict = str(getattr(stim, "verdict", "") or "").lower()
        if verdict in {"inconsistent", "llm_only", "llm-only"}:
            _stable_add(tags, "test-stimuli")
            _stable_add(tags, "output-semantics")
            break

    stim_map = {
        str(getattr(stim, "name", "") or ""): stim
        for stim in probe_stimuli or ()
    }
    for outcome in probe_outcomes or ():
        stim = stim_map.get(str(getattr(outcome, "stimulus_name", "") or ""))
        if getattr(outcome, "error", ""):
            _stable_add(tags, "runtime-routing")
            continue
        if not bool(getattr(outcome, "boot_detected", False)):
            _stable_add(tags, "startup")
            _stable_add(tags, "rtos-api")
            _stable_add(tags, "hardware-context")
            continue
        if not bool(getattr(outcome, "test_done", False)):
            _stable_add(tags, "protocol")
            _stable_add(tags, "timing")
            _stable_add(tags, "fault-handling")
            continue
        if bool(getattr(outcome, "result_pass", False)) and not _stim_expected_error(stim):
            continue

        _stable_add(tags, "protocol")
        if _stim_expected_error(stim) or getattr(outcome, "expected_err", None):
            _stable_add(tags, "fault-handling")
        if getattr(outcome, "result_err", False) or getattr(outcome, "read_err", None) not in (None, 0):
            _stable_add(tags, "transfer-sequence")
            _stable_add(tags, "rtos-api")
        if getattr(outcome, "read_channels", None) or getattr(stim, "expected_channels", None):
            _stable_add(tags, "output-semantics")
            _stable_add(tags, "conversion")
        elif getattr(outcome, "mem_bytes", None) or getattr(stim, "expected_mem_bytes", None):
            _stable_add(tags, "memory-protocol")
        elif getattr(outcome, "rtc_time", None) or getattr(stim, "expected_time", None):
            _stable_add(tags, "rtc-registers")
        elif getattr(outcome, "display_frame_err", None) or getattr(outcome, "display_status_err", None):
            _stable_add(tags, "display-protocol")
        elif getattr(outcome, "read_raw", None) is not None or getattr(stim, "expected_read_raw", None) is not None:
            _stable_add(tags, "output-semantics")
            _stable_add(tags, "conversion")

    return tuple(tags)


def _select_device_fields(device_ir: Mapping[str, Any], tags: Sequence[str]) -> tuple[str, ...]:
    selected: list[str] = []
    tagset = set(tags)

    def add_many(keys: Sequence[str]) -> None:
        for key in keys:
            if device_ir.get(key) not in (None, "", [], {}):
                _stable_add(selected, key)

    if tagset & {
        "protocol", "expected-transactions", "startup", "transfer-sequence",
        "runtime-routing", "hardware-context", "rtos-api",
    }:
        add_many((
            "address_rule",
            "registers_or_commands",
            "init_sequence",
            "read_sequence",
            "operation_flows",
        ))
    if tagset & {"timing", "fault-handling"}:
        add_many(("timing_constraints", "error_conditions", "bitfields", "operation_flows"))
    if tagset & {"output-semantics", "conversion", "test-stimuli"}:
        add_many(("read_channels", "raw_encoding", "conversion_formulae", "bitfields"))
    if tagset & {"memory-protocol", "display-protocol", "rtc-registers"}:
        add_many(("registers_or_commands", "read_sequence", "operation_flows", "bitfields"))
    return tuple(selected)


def _binding_matches(binding: Mapping[str, Any], terms: Sequence[str]) -> bool:
    if not terms:
        return False
    blob_parts: list[str] = []
    for key in ("symbol", "signature", "semantic_role", "declared_in", "declaration_path"):
        value = binding.get(key)
        if value:
            blob_parts.append(str(value))
    headers = binding.get("required_headers")
    if isinstance(headers, Sequence) and not isinstance(headers, (str, bytes, bytearray)):
        blob_parts.extend(str(header) for header in headers)
    blob = " ".join(blob_parts)
    return any(term and term in blob for term in terms)


def _select_rtos_slots(
    rtos_contract: Mapping[str, Any],
    tags: Sequence[str],
    compile_terms: Sequence[str],
) -> tuple[str, ...]:
    bindings = _as_mapping(rtos_contract.get("api_bindings"))
    if not bindings:
        return ()

    matched: list[str] = []
    fallback: list[str] = []
    for slot_id, raw in bindings.items():
        binding = _as_mapping(raw)
        if not binding:
            continue
        slot = str(slot_id)
        if _binding_matches(binding, compile_terms):
            _stable_add(matched, slot)
        else:
            _stable_add(fallback, slot)

    tagset = set(tags)
    if matched:
        return tuple(matched)
    if tagset & {
        "rtos-api", "headers", "api-contract", "transfer-sequence",
        "startup", "protocol", "runtime-routing", "fault-handling",
        "output-semantics", "conversion",
    }:
        return tuple(fallback)
    return ()


def _tx_to_dict(tx: Any) -> dict[str, Any]:
    if hasattr(tx, "to_dict") and callable(tx.to_dict):
        try:
            data = tx.to_dict()
            if isinstance(data, dict):
                return data
        except Exception:
            pass
    if isinstance(tx, Mapping):
        return dict(tx)
    return {
        "phase": getattr(tx, "phase", None),
        "addr_or_pin": getattr(tx, "addr_or_pin", None),
        "write_prefix_any_of": getattr(tx, "write_prefix_any_of", None),
        "read_any": getattr(tx, "read_any", None),
        "note": getattr(tx, "note", None),
    }


def _select_transactions(
    expected_transactions: Sequence[Any],
    coverage_report: Any,
    tags: Sequence[str],
) -> tuple[dict[str, Any], ...]:
    missing = [
        getattr(match, "mechanical", None)
        for match in getattr(coverage_report, "missing", ()) or ()
    ]
    if missing:
        return tuple(_tx_to_dict(tx) for tx in missing if tx is not None)
    if set(tags) & {"protocol", "expected-transactions", "transfer-sequence", "startup", "fault-handling"}:
        return tuple(_tx_to_dict(tx) for tx in expected_transactions or ())
    return ()


def build_repair_context(
    *,
    layer_failed: str,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    expected_transactions: Sequence[Any],
    frozen_plan: Mapping[str, Any],
    synthesis_error: Any = None,
    compile_errors: Sequence[str] = (),
    consistency_report: Any = None,
    coverage_report: Any = None,
    adapter_error: str | None = None,
    probe_outcomes: Sequence[Any] = (),
    probe_stimuli: Sequence[Any] = (),
) -> dict[str, Any]:
    """Build the context-selection spec for the next repair attempt."""
    compile_errors = tuple(str(e) for e in (compile_errors or ()) if str(e).strip())
    terms = _compile_terms(compile_errors, adapter_error)
    tags = _infer_focus_tags(
        layer_failed=layer_failed,
        synthesis_error=synthesis_error,
        compile_errors=compile_errors,
        adapter_error=adapter_error,
        consistency_report=consistency_report,
        coverage_report=coverage_report,
        probe_outcomes=probe_outcomes,
        probe_stimuli=probe_stimuli,
    )
    if not tags:
        return {}

    context = {
        "focus_tags": tags,
        "compile_terms": terms,
        "device_fields": _select_device_fields(_as_mapping(device_ir), tags),
        "rtos_slots": _select_rtos_slots(_as_mapping(rtos_contract), tags, terms),
        "expected_transactions": _select_transactions(expected_transactions, coverage_report, tags),
        "failing_stimuli": _failing_probe_names(probe_outcomes, probe_stimuli),
        "include_api_contract": True,
        "include_selected_test_stimuli": bool(
            set(tags) & {"test-stimuli", "fault-handling", "output-semantics", "conversion"}
        ),
        "plan_hash": _as_mapping(frozen_plan).get("plan_hash"),
    }
    return {key: value for key, value in context.items() if value not in (None, "", (), [], {})}


__all__ = ["build_repair_context"]
