"""Model-assisted public output semantics alignment for code generation."""
from __future__ import annotations

import json
import logging
import os
import re
import time
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from ..llm.providers import BaseProvider, ProviderError
from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_RTC,
    ClassifyResult,
)
from .route import RoutingResult

logger = logging.getLogger(__name__)

_SAFE_ID_RE = re.compile(r"[^a-z0-9]+")
_ALLOWED_KINDS = {"raw_count", "physical_scaled", "physical_base", "status_or_code"}
_PHYSICAL_KINDS = {"physical_scaled", "physical_base"}
_RAW_UNIT_TOKENS = ("raw", "count", "counts", "code", "codes", "lsb", "adc")


def output_semantics_schema() -> Dict[str, Any]:
    """Return the lightweight JSON object shape requested from the model."""
    return {
        "type": "object",
        "additionalProperties": True,
        "required": ["channels"],
        "properties": {
            "version": {"type": "integer"},
            "device_id": {"type": "string"},
            "channels": {
                "type": "array",
                "items": {
                    "type": "object",
                    "additionalProperties": True,
                    "required": ["source_channel", "semantic_kind", "public_unit"],
                    "properties": {
                        "public_id": {"type": "string", "minLength": 1},
                        "source_channel": {"type": "string", "minLength": 1},
                        "semantic_kind": {
                            "type": "string",
                            "enum": sorted(_ALLOWED_KINDS),
                        },
                        "public_unit": {"type": "string", "minLength": 1},
                        "c_type": {"type": "string", "minLength": 1},
                        "conversion_required": {"type": "boolean"},
                        "formula_id": {"type": "string"},
                        "confidence": {"type": "number"},
                        "evidence": {"type": "string"},
                    },
                },
            },
            "warnings": {
                "type": "array",
                "items": {"type": "string"},
            },
        },
    }


def should_build_output_semantics_map(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> bool:
    """Return whether output-semantics inference should run for this task."""
    mode = os.getenv("DRIVERGEN_OUTPUT_SEMANTICS_MODE", "auto").strip().lower()
    if mode in {"0", "false", "no", "off", "disabled"}:
        return False
    records = _read_channel_records(device_ir)
    if not records:
        return False
    if mode in {"1", "true", "yes", "on", "force", "always"}:
        return True
    return classify_result.eval_class not in {
        EVAL_CLASS_DISPLAY,
        EVAL_CLASS_MEMORY,
        EVAL_CLASS_RTC,
    }


def build_or_load_output_semantics_map(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    run_dir: Optional[Path] = None,
    prefer_json_mode: bool = True,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> Dict[str, Any]:
    """Generate or load public output semantics for this run."""
    if not should_build_output_semantics_map(device_ir, classify_result):
        result = _empty_output_semantics_map(device_ir, "disabled_or_not_applicable")
        _dump_output_semantics_map(run_dir, result)
        return result

    cache_path = Path(run_dir) / "output_semantics_map.json" if run_dir else None
    refresh = os.getenv("DRIVERGEN_OUTPUT_SEMANTICS_REFRESH", "").strip().lower()
    if cache_path is not None and refresh not in {"1", "true", "yes", "on"}:
        try:
            if cache_path.is_file():
                cached = json.loads(cache_path.read_text(encoding="utf-8"))
                if isinstance(cached, Mapping):
                    cleaned = normalise_output_semantics_map(
                        device_ir,
                        cached,
                        channel_alias_map=channel_alias_map,
                    )
                    cleaned["cache_status"] = "hit"
                    _dump_output_semantics_map(run_dir, cleaned)
                    return cleaned
        except (OSError, json.JSONDecodeError, TypeError, ValueError) as exc:
            logger.warning(
                "Could not load output semantics cache %s: %s",
                cache_path, exc,
            )

    raw: Mapping[str, Any]
    raw_text = ""
    fallback_reason = ""
    system_prompt, user_prompt = build_output_semantics_prompt(
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        channel_alias_map=channel_alias_map,
        task_package=task_package,
    )
    metadata: Dict[str, Any] = {
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "rtos_id": str(rtos_contract.get("rtos") or "unknown"),
        "eval_class": classify_result.eval_class,
        "bus_kind": routing.bus_kind or classify_result.bus_type,
        "stage": "output_semantics_v1",
    }
    if extra_metadata:
        for key, value in extra_metadata.items():
            metadata.setdefault(key, value)

    start = time.time()
    try:
        if prefer_json_mode:
            raw = provider.generate_json(
                "output_semantics_map",
                output_semantics_schema(),
                system_prompt,
                user_prompt,
                metadata,
            )
            raw_text = json.dumps(raw, ensure_ascii=False)
        else:
            raw_text = provider.generate_text(system_prompt, user_prompt, metadata)
            raw = _extract_json_object(raw_text)
    except (NotImplementedError, ProviderError, ValueError, TypeError) as exc:
        fallback_reason = f"{type(exc).__name__}: {exc}"
        logger.warning(
            "output_semantics model call failed; using fallback: %s",
            fallback_reason,
        )
        raw = _fallback_output_semantics_map(device_ir, channel_alias_map, fallback_reason)
        raw_text = ""
    elapsed = round(time.time() - start, 3)

    result = normalise_output_semantics_map(
        device_ir,
        raw,
        channel_alias_map=channel_alias_map,
    )
    result["generation_time_s"] = elapsed
    result["model"] = getattr(provider, "model", "") or ""
    result["provider_name"] = getattr(provider, "name", "") or ""
    result["cache_status"] = "generated"
    if fallback_reason:
        result["fallback_reason"] = fallback_reason
    _dump_output_semantics_map(run_dir, result)
    _dump_raw_output_semantics_response(run_dir, raw_text)
    return result


def build_output_semantics_prompt(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
) -> Tuple[str, str]:
    """Build the model prompt for public output semantics selection."""
    records = _read_channel_records(device_ir)
    formulae = _formula_records(device_ir)
    flows = _flow_records(device_ir)
    raw_encoding = _compact_mapping(device_ir.get("raw_encoding"))
    alias_rows = _alias_rows(channel_alias_map)
    device_id = str(device_ir.get("device_id") or "unknown")
    rtos_id = str(rtos_contract.get("rtos") or "unknown")
    task_summary = _task_summary(task_package)

    system_prompt = (
        "You choose public output value semantics for embedded-driver code "
        "generation. Return exactly one JSON object. Use only the Device IR, "
        "platform contract summary, routing, and task context. Do not use "
        "reference-driver expected values. Do not "
        "invent channels."
    )
    user_prompt = "\n".join(
        [
            "# Public Output Semantics Task",
            "",
            f"- device_id: {device_id}",
            f"- target_rtos: {rtos_id}",
            f"- eval_class: {classify_result.eval_class}",
            f"- bus: {routing.bus_kind or classify_result.bus_type}",
            f"- channel_count: {classify_result.channel_count}",
            f"- channel_roots: {list(classify_result.channel_roots)}",
            f"- task_summary: {task_summary}",
            "",
            "## Source read_channels from Device IR",
            json.dumps(records, ensure_ascii=False, indent=2),
            "",
            "## Channel canonicalization map",
            json.dumps(alias_rows, ensure_ascii=False, indent=2),
            "",
            "## Raw encoding hints",
            json.dumps(raw_encoding, ensure_ascii=False, indent=2),
            "",
            "## Conversion formulae",
            json.dumps(formulae, ensure_ascii=False, indent=2),
            "",
            "## Operation flows that mention readable outputs",
            json.dumps(flows, ensure_ascii=False, indent=2),
            "",
            "## Output JSON shape",
            json.dumps(output_semantics_schema(), ensure_ascii=False, indent=2),
            "",
            "## Semantic kind definitions",
            "- raw_count: public API returns the raw register field, ADC code, LSB count, or unconverted sensor code.",
            "- physical_scaled: public API returns a converted physical integer in an explicit scaled unit such as milli_degC, milli_g, micro_T, Pa, lux, or milli_percent_rh.",
            "- physical_base: public API returns a converted physical value in a base unit when no scaled integer convention is present.",
            "- status_or_code: public API returns a status enum, device mode, ID, error code, or other non-measurement code.",
            "",
            "## Rules",
            "- Every Device IR read_channel id must appear exactly once as channels[*].source_channel.",
            "- If a channel canonicalization row exists, channels[*].public_id must equal that row's canonical_id for the source_channel.",
            "- Decide semantics for the generation-side api_contract, test_plan expected values, and driver_code outputs together; they must agree.",
            "- conversion_required must be false for semantic_kind=raw_count/status_or_code and true for semantic_kind=physical_scaled/physical_base.",
            "- Choose physical_scaled/physical_base only when Device IR gives enough source bytes, encoding, and conversion formula evidence to compute the final public value in generated C without inventing missing arithmetic.",
            "- Choose raw_count when conversion depends on missing runtime configuration, full-scale range, undocumented calibration, or a formula that the IR does not make executable.",
            "- Treat a formula as executable only when it has a non-null integer_approximation_expression, or it is a simple direct arithmetic formula whose inputs are all bound to source bytes/config values present in the IR.",
            "- A warning is not permission to choose physical output. If your evidence says the conversion may require extra scaling, careful compensation, floating point, missing implementation detail, or a missing integer_approximation_expression, the correct semantic_kind is raw_count for that channel.",
            "- Do not treat a prose-only compensation formula as executable. If integer_approximation_expression is null/missing for a calibrated pressure/humidity/IMU/environmental compensation formula, choose raw_count and add a warning. Do not choose physical_scaled merely because the prose formula names calibration coefficients.",
            "- If a physical conversion needs calibration coefficients, scale factors, oversampling/range settings, or mode-dependent constants, choose physical_scaled only when all required values and executable arithmetic are explicitly present in Device IR operation_flows/config/register/formula evidence.",
            "- For simple scalar sensors with explicit two's-complement/raw encoding and a clear physical formula, prefer the converted physical output unit.",
            "- For accelerometer, gyroscope, magnetometer, and similar configurable-axis sensors, prefer raw_count for axis channels unless the IR explicitly fixes full-scale/range configuration and formula.",
            "- Do not use reset/default sensitivity, datasheet default range, or an assumed full-scale setting as enough evidence for physical_scaled motion-axis outputs. The IR must show an init/config flow that writes the range/full-scale register, or an explicit fixed_range/fixed_scale fact that the generated driver will preserve.",
            "- If you choose physical_scaled for a motion axis, evidence must name the exact config register/write or fixed-scale IR fact. Evidence such as 'default full-scale assumed' is insufficient and should instead lead to raw_count plus a warning.",
            "- For calibrated environmental sensors, choose physical_scaled only if the IR contains the calibration reads and an executable compensation formula for that exact channel; otherwise choose raw_count and add a warning.",
            "- public_unit must be the exact unit later used by api_contract/test_plan/driver_code.",
            "- Use int32_t for raw_count and scaled physical integer outputs unless explicit bounds require a different C scalar type.",
            "- confidence should reflect IR evidence quality, not whether the choice would match any external evaluation.",
            "- Return only JSON, with no Markdown fences.",
        ]
    )
    return system_prompt, user_prompt


def normalise_output_semantics_map(
    device_ir: Mapping[str, Any],
    payload: Mapping[str, Any],
    *,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
) -> Dict[str, Any]:
    """Sanitise the model payload and fill missing source channels."""
    records = _read_channel_records(device_ir)
    source_ids = [str(row.get("id") or "").strip() for row in records]
    source_set = {sid for sid in source_ids if sid}
    alias_by_source = _alias_by_source(channel_alias_map)
    raw_channels = payload.get("channels") if isinstance(payload, Mapping) else []
    if not isinstance(raw_channels, list):
        raw_channels = []

    channels: List[Dict[str, Any]] = []
    assigned: set[str] = set()
    warnings: List[str] = []

    for row in raw_channels:
        if not isinstance(row, Mapping):
            warnings.append("ignored non-object output semantics row")
            continue
        source = str(row.get("source_channel") or "").strip()
        if source not in source_set:
            if source:
                warnings.append(f"ignored unknown source_channel: {source!r}")
            continue
        if source in assigned:
            warnings.append(f"ignored duplicate source_channel: {source!r}")
            continue
        record = next((r for r in records if r.get("id") == source), {})
        kind = _normalise_kind(row.get("semantic_kind"))
        if not kind:
            kind = _fallback_semantic_kind(record, device_ir)
            warnings.append(f"filled missing semantic_kind for {source!r} with {kind!r}")
        public_id = alias_by_source.get(source) or _safe_id(row.get("public_id") or source)
        if not public_id:
            public_id = _safe_id(source) or "channel"
        public_unit = _normalise_public_unit(row.get("public_unit"), kind, record)
        formula_id = str(row.get("formula_id") or record.get("formula_id") or "").strip()
        conversion_required = row.get("conversion_required")
        if not isinstance(conversion_required, bool):
            conversion_required = kind in _PHYSICAL_KINDS
        expected_conversion = kind in _PHYSICAL_KINDS
        if bool(conversion_required) != expected_conversion:
            warnings.append(
                f"corrected conversion_required for {source!r}: "
                f"semantic_kind={kind!r} implies {expected_conversion}"
            )
            conversion_required = expected_conversion
        channels.append(
            {
                "public_id": public_id,
                "source_channel": source,
                "semantic_kind": kind,
                "public_unit": public_unit,
                "c_type": _normalise_c_type(row.get("c_type"), kind),
                "conversion_required": bool(conversion_required),
                "formula_id": formula_id,
                "confidence": _coerce_confidence(row.get("confidence")),
                "evidence": _safe_text(row.get("evidence")),
            }
        )
        assigned.add(source)

    for source in source_ids:
        if source and source not in assigned:
            record = next((r for r in records if r.get("id") == source), {})
            fallback = _fallback_channel_semantics(record, device_ir, channel_alias_map)
            channels.append(fallback)
            assigned.add(source)
            warnings.append(
                f"filled missing source_channel {source!r} with fallback "
                f"{fallback['semantic_kind']!r}"
            )

    payload_warnings = payload.get("warnings") if isinstance(payload, Mapping) else []
    warnings.extend(_string_list(payload_warnings))
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or payload.get("device_id") or "unknown"),
        "source": "llm_output_semantics",
        "channels": channels,
        "warnings": warnings,
    }


def primary_semantic_kind(output_semantics_map: Optional[Mapping[str, Any]]) -> str:
    """Return the first public output semantic kind, or an empty string."""
    if not isinstance(output_semantics_map, Mapping):
        return ""
    channels = output_semantics_map.get("channels")
    if not isinstance(channels, list) or not channels:
        return ""
    first = channels[0]
    if not isinstance(first, Mapping):
        return ""
    kind = str(first.get("semantic_kind") or "").strip().lower()
    return kind if kind in _ALLOWED_KINDS else ""


def semantic_kind_is_physical(kind: str) -> bool:
    return str(kind or "").strip().lower() in _PHYSICAL_KINDS


def _read_channel_records(device_ir: Mapping[str, Any]) -> List[Dict[str, Any]]:
    rows = device_ir.get("read_channels") if isinstance(device_ir, Mapping) else None
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows:
        if isinstance(row, Mapping):
            cid = str(row.get("id") or "").strip()
            if not cid:
                continue
            out.append(
                {
                    "id": cid,
                    "raw_type": row.get("raw_type") or "",
                    "physical_unit": row.get("physical_unit") or "",
                    "description": row.get("description") or "",
                    "source_bytes": row.get("source_bytes") or [],
                    "formula_id": row.get("formula_id") or "",
                    "notes": str(row.get("notes") or "")[:240],
                }
            )
        elif row is not None:
            cid = str(row).strip()
            if cid:
                out.append({"id": cid})
    return out


def _formula_records(device_ir: Mapping[str, Any]) -> List[Dict[str, Any]]:
    rows = device_ir.get("conversion_formulae") if isinstance(device_ir, Mapping) else None
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows[:24]:
        if not isinstance(row, Mapping):
            continue
        expr = row.get("integer_approximation_expression")
        if isinstance(expr, Mapping):
            expr = {
                "expression": expr.get("expression") or "",
                "inputs": expr.get("inputs") or [],
                "output": expr.get("output") or "",
            }
        out.append(
            {
                "id": row.get("id") or row.get("name") or "",
                "name": row.get("name") or row.get("id") or "",
                "formula": row.get("formula") or "",
                "integer_approximation_expression": expr,
                "notes": str(row.get("notes") or "")[:240],
            }
        )
    return out


def _flow_records(device_ir: Mapping[str, Any]) -> List[Dict[str, Any]]:
    rows = device_ir.get("operation_flows") if isinstance(device_ir, Mapping) else None
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows[:18]:
        if not isinstance(row, Mapping):
            continue
        out.append(
            {
                "flow_id": row.get("flow_id") or "",
                "kind": row.get("kind") or "",
                "channels": row.get("channels") or [],
                "register_sequence": row.get("register_sequence") or [],
                "notes": str(row.get("notes") or "")[:260],
            }
        )
    return out


def _alias_rows(channel_alias_map: Optional[Mapping[str, Any]]) -> List[Dict[str, Any]]:
    if not isinstance(channel_alias_map, Mapping):
        return []
    rows = channel_alias_map.get("channels")
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows:
        if not isinstance(row, Mapping):
            continue
        out.append(
            {
                "canonical_id": row.get("canonical_id") or "",
                "source_channels": row.get("source_channels") or [],
                "aliases": row.get("aliases") or [],
                "unit": row.get("unit") or "",
                "quantity": row.get("quantity") or "",
                "axis": row.get("axis") or "",
                "location": row.get("location") or "",
            }
        )
    return out


def _alias_by_source(channel_alias_map: Optional[Mapping[str, Any]]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    for row in _alias_rows(channel_alias_map):
        canonical = _safe_id(row.get("canonical_id") or "")
        if not canonical:
            continue
        for source in row.get("source_channels") or []:
            source_id = str(source or "").strip()
            if source_id and source_id not in out:
                out[source_id] = canonical
    return out


def _compact_mapping(value: Any) -> Dict[str, Any]:
    if not isinstance(value, Mapping):
        return {}
    out: Dict[str, Any] = {}
    for key, item in value.items():
        if isinstance(item, (str, int, float, bool)) or item is None:
            out[str(key)] = item
        elif isinstance(item, Sequence) and not isinstance(item, (str, bytes)):
            out[str(key)] = list(item[:12])
    return out


def _task_summary(task_package: Optional[Mapping[str, Any]]) -> str:
    if not isinstance(task_package, Mapping):
        return ""
    parts: List[str] = []
    for key in ("package_id", "device_id", "bus_type", "device_role", "summary"):
        value = task_package.get(key)
        if value:
            parts.append(f"{key}={value}")
    return "; ".join(parts)


def _extract_json_object(text: str) -> Dict[str, Any]:
    raw = text.strip()
    try:
        parsed = json.loads(raw)
        if isinstance(parsed, Mapping):
            return dict(parsed)
    except json.JSONDecodeError:
        pass
    match = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", raw, re.DOTALL | re.IGNORECASE)
    if match:
        parsed = json.loads(match.group(1))
        if isinstance(parsed, Mapping):
            return dict(parsed)
    start = raw.find("{")
    end = raw.rfind("}")
    if start >= 0 and end > start:
        parsed = json.loads(raw[start : end + 1])
        if isinstance(parsed, Mapping):
            return dict(parsed)
    raise ValueError("output semantics response contains no JSON object")


def _fallback_output_semantics_map(
    device_ir: Mapping[str, Any],
    channel_alias_map: Optional[Mapping[str, Any]],
    reason: str,
) -> Dict[str, Any]:
    channels = [
        _fallback_channel_semantics(row, device_ir, channel_alias_map)
        for row in _read_channel_records(device_ir)
    ]
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "channels": channels,
        "warnings": [f"fallback map used: {reason}"],
    }


def _fallback_channel_semantics(
    record: Mapping[str, Any],
    device_ir: Mapping[str, Any],
    channel_alias_map: Optional[Mapping[str, Any]],
) -> Dict[str, Any]:
    source = str(record.get("id") or "").strip()
    kind = _fallback_semantic_kind(record, device_ir)
    unit = _normalise_public_unit("", kind, record)
    return {
        "public_id": _alias_by_source(channel_alias_map).get(source) or _safe_id(source) or "channel",
        "source_channel": source,
        "semantic_kind": kind,
        "public_unit": unit,
        "c_type": "int32_t",
        "conversion_required": kind in _PHYSICAL_KINDS,
        "formula_id": str(record.get("formula_id") or "").strip(),
        "confidence": 0.45,
        "evidence": "deterministic fallback after output-semantics model call was unavailable",
    }


def _empty_output_semantics_map(device_ir: Mapping[str, Any], reason: str) -> Dict[str, Any]:
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "source": "llm_output_semantics",
        "channels": [],
        "warnings": [reason],
        "cache_status": "skipped",
    }


def _fallback_semantic_kind(record: Mapping[str, Any], device_ir: Mapping[str, Any]) -> str:
    unit = str(record.get("physical_unit") or "")
    if _unit_looks_raw_like(unit):
        return "raw_count"
    if _looks_like_status_channel(record):
        return "status_or_code"
    if _looks_like_motion_axis(record):
        return "raw_count"
    if _channel_has_formula(record, device_ir) and unit:
        return "physical_scaled" if _unit_looks_scaled(unit) else "physical_base"
    return "raw_count"


def _channel_has_formula(record: Mapping[str, Any], device_ir: Mapping[str, Any]) -> bool:
    formula_id = str(record.get("formula_id") or "").strip().lower()
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, Sequence) or isinstance(formulae, (str, bytes)):
        return bool(formula_id)
    if formula_id:
        for formula in formulae:
            if not isinstance(formula, Mapping):
                continue
            names = {
                str(formula.get("id") or "").strip().lower(),
                str(formula.get("name") or "").strip().lower(),
            }
            if formula_id in names:
                return True
    if len(formulae) == 1:
        return True
    source = str(record.get("id") or "").strip().lower()
    if not source:
        return False
    for formula in formulae:
        text = json.dumps(formula, ensure_ascii=False).lower()
        if source in text:
            return True
    return False


def _looks_like_motion_axis(record: Mapping[str, Any]) -> bool:
    text = " ".join(
        str(part or "")
        for part in (
            record.get("id"),
            record.get("description"),
            record.get("physical_unit"),
            record.get("formula_id"),
        )
    ).lower()
    compact = _safe_id(text)
    tokens = set(compact.split("_"))
    motion = {
        "accel",
        "accelerometer",
        "acceleration",
        "gyro",
        "gyroscope",
        "angular",
        "mag",
        "magnetometer",
        "magnetic",
    }
    axes = {"x", "y", "z", "axis"}
    return bool(tokens & motion) and bool(tokens & axes)


def _looks_like_status_channel(record: Mapping[str, Any]) -> bool:
    text = _safe_id(
        " ".join(
            str(part or "")
            for part in (
                record.get("id"),
                record.get("description"),
                record.get("physical_unit"),
            )
        )
    )
    return any(tok in text.split("_") for tok in ("status", "mode", "fault", "id", "code"))


def _unit_looks_raw_like(unit: Any) -> bool:
    compact = _safe_id(unit)
    if not compact:
        return False
    return any(token in compact.split("_") or token in compact for token in _RAW_UNIT_TOKENS)


def _unit_looks_scaled(unit: Any) -> bool:
    text = str(unit or "").strip().lower()
    compact = _safe_id(text)
    return (
        "milli" in compact
        or "micro" in compact
        or compact in {"pa", "lux", "percent_rh", "rh", "ppm"}
        or "%" in text
    )


def _normalise_kind(value: Any) -> str:
    kind = str(value or "").strip().lower()
    return kind if kind in _ALLOWED_KINDS else ""


def _normalise_public_unit(value: Any, kind: str, record: Mapping[str, Any]) -> str:
    unit = str(value or "").strip()
    if kind == "raw_count":
        return "raw_count" if not unit or not _unit_looks_raw_like(unit) else unit
    if kind == "status_or_code":
        return unit or "status_code"
    if unit:
        return unit
    physical = str(record.get("physical_unit") or "").strip()
    return physical or ("physical_value" if kind in _PHYSICAL_KINDS else "raw_count")


def _normalise_c_type(value: Any, kind: str) -> str:
    text = str(value or "").strip()
    if re.fullmatch(r"(?:u?int(?:8|16|32|64)_t|float|double|bool)", text):
        if kind in _PHYSICAL_KINDS and text in {"int8_t", "uint8_t", "int16_t", "uint16_t"}:
            return "int32_t"
        return text
    return "int32_t"


def _coerce_confidence(value: Any) -> float:
    try:
        f = float(value)
    except (TypeError, ValueError):
        return 0.5
    if f < 0:
        return 0.0
    if f > 1:
        return 1.0
    return round(f, 3)


def _string_list(value: Any) -> List[str]:
    if not isinstance(value, Sequence) or isinstance(value, (str, bytes)):
        return []
    return [str(item).strip() for item in value if str(item).strip()]


def _safe_id(value: Any) -> str:
    text = str(value or "").strip().lower()
    text = _SAFE_ID_RE.sub("_", text)
    return text.strip("_")


def _safe_text(value: Any) -> str:
    return str(value or "").strip()[:300]


def _dump_output_semantics_map(run_dir: Optional[Path], payload: Mapping[str, Any]) -> None:
    if run_dir is None:
        return
    path = Path(run_dir) / "output_semantics_map.json"
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    except OSError as exc:
        logger.warning("Could not write output semantics map %s: %s", path, exc)


def _dump_raw_output_semantics_response(run_dir: Optional[Path], raw_text: str) -> None:
    if run_dir is None or not raw_text:
        return
    path = Path(run_dir) / "output_semantics_raw_response.txt"
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(raw_text, encoding="utf-8")
    except OSError as exc:
        logger.warning("Could not write output semantics raw response %s: %s", path, exc)


__all__ = [
    "build_or_load_output_semantics_map",
    "build_output_semantics_prompt",
    "normalise_output_semantics_map",
    "output_semantics_schema",
    "primary_semantic_kind",
    "semantic_kind_is_physical",
    "should_build_output_semantics_map",
]
