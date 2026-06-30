"""Model-assisted channel canonicalisation for multi-channel code generation."""
from __future__ import annotations

import json
import logging
import os
import re
import time
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from ..llm.providers import BaseProvider, ProviderError
from .classify_device import EVAL_CLASS_MULTI_CHANNEL, ClassifyResult
from .route import RoutingResult

logger = logging.getLogger(__name__)

_SAFE_ID_RE = re.compile(r"[^a-z0-9]+")
_WS_RE = re.compile(r"\s+")


def channel_alias_schema() -> Dict[str, Any]:
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
                    "required": ["canonical_id", "source_channels"],
                    "properties": {
                        "canonical_id": {"type": "string", "minLength": 1},
                        "source_channels": {
                            "type": "array",
                            "items": {"type": "string"},
                        },
                        "aliases": {
                            "type": "array",
                            "items": {"type": "string"},
                        },
                        "quantity": {"type": "string"},
                        "location": {"type": "string"},
                        "axis": {"type": "string"},
                        "unit": {"type": "string"},
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


def should_build_channel_alias_map(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> bool:
    """Return whether alias inference should run for this task."""
    mode = os.getenv("DRIVERGEN_CHANNEL_ALIAS_MODE", "auto").strip().lower()
    if mode in {"0", "false", "no", "off", "disabled"}:
        return False
    channels = _read_channel_records(device_ir)
    if not channels:
        return False
    if mode in {"1", "true", "yes", "on", "force", "always"}:
        return True
    return (
        classify_result.eval_class == EVAL_CLASS_MULTI_CHANNEL
        and len(channels) >= 2
    )


def build_or_load_channel_alias_map(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    task_package: Optional[Mapping[str, Any]] = None,
    run_dir: Optional[Path] = None,
    prefer_json_mode: bool = True,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> Dict[str, Any]:
    """Generate or load a channel alias map for this run."""
    if not should_build_channel_alias_map(device_ir, classify_result):
        result = _empty_alias_map(device_ir, "disabled_or_not_applicable")
        _dump_alias_map(run_dir, result)
        return result

    cache_path = Path(run_dir) / "channel_alias_map.json" if run_dir else None
    refresh = os.getenv("DRIVERGEN_CHANNEL_ALIAS_REFRESH", "").strip().lower()
    if cache_path is not None and refresh not in {"1", "true", "yes", "on"}:
        try:
            if cache_path.is_file():
                cached = json.loads(cache_path.read_text(encoding="utf-8"))
                if isinstance(cached, Mapping):
                    cleaned = normalise_channel_alias_map(device_ir, cached)
                    cleaned["cache_status"] = "hit"
                    _dump_alias_map(run_dir, cleaned)
                    return cleaned
        except (OSError, json.JSONDecodeError, TypeError, ValueError) as exc:
            logger.warning("Could not load channel alias cache %s: %s", cache_path, exc)

    raw: Mapping[str, Any]
    raw_text = ""
    fallback_reason = ""
    system_prompt, user_prompt = build_channel_alias_prompt(
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        task_package=task_package,
    )
    metadata: Dict[str, Any] = {
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "rtos_id": str(rtos_contract.get("rtos") or "unknown"),
        "eval_class": classify_result.eval_class,
        "bus_kind": routing.bus_kind or classify_result.bus_type,
        "stage": "channel_alias",
    }
    if extra_metadata:
        for key, value in extra_metadata.items():
            metadata.setdefault(key, value)

    start = time.time()
    try:
        if prefer_json_mode:
            raw = provider.generate_json(
                "channel_alias_map",
                channel_alias_schema(),
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
        logger.warning("channel_alias model call failed; using fallback: %s", fallback_reason)
        raw = _fallback_alias_map(device_ir, fallback_reason)
        raw_text = ""
    elapsed = round(time.time() - start, 3)

    result = normalise_channel_alias_map(device_ir, raw)
    result["generation_time_s"] = elapsed
    result["model"] = getattr(provider, "model", "") or ""
    result["provider_name"] = getattr(provider, "name", "") or ""
    result["cache_status"] = "generated"
    if fallback_reason:
        result["fallback_reason"] = fallback_reason
    _dump_alias_map(run_dir, result)
    _dump_raw_alias_response(run_dir, raw_text)
    return result


def build_channel_alias_prompt(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    task_package: Optional[Mapping[str, Any]] = None,
) -> Tuple[str, str]:
    """Build the model prompt for channel-name alignment."""
    records = _read_channel_records(device_ir)
    flows = _flow_records(device_ir)
    formulae = _formula_records(device_ir)
    task_summary = _task_summary(task_package)
    device_id = str(device_ir.get("device_id") or "unknown")
    rtos_id = str(rtos_contract.get("rtos") or "unknown")
    system_prompt = (
        "You align sensor channel names for embedded-driver code generation. "
        "Return exactly one JSON object. Do not invent channels. Do not use "
        "reference answers. Prefer short, stable public IDs that describe "
        "quantity plus location/axis when needed."
    )
    user_prompt = "\n".join(
        [
            "# Channel Canonicalization Task",
            "",
            f"- device_id: {device_id}",
            f"- target_rtos: {rtos_id}",
            f"- eval_class: {classify_result.eval_class}",
            f"- bus: {routing.bus_kind or classify_result.bus_type}",
            f"- channel_roots: {list(classify_result.channel_roots)}",
            f"- task_summary: {task_summary}",
            "",
            "## Source read_channels from Device IR",
            json.dumps(records, ensure_ascii=False, indent=2),
            "",
            "## Operation flows that mention channels",
            json.dumps(flows, ensure_ascii=False, indent=2),
            "",
            "## Conversion formulae",
            json.dumps(formulae, ensure_ascii=False, indent=2),
            "",
            "## Output JSON shape",
            json.dumps(channel_alias_schema(), ensure_ascii=False, indent=2),
            "",
            "## Rules",
            "- Every source read_channel id must appear in exactly one channels[*].source_channels entry.",
            "- canonical_id must be lowercase snake_case and safe as a C/API key.",
            "- Use canonical_id for public API/test output names later; put old IR spellings in aliases.",
            "- Keep semantically distinct channels separate: axes, local vs remote, pressure vs temperature, etc.",
            "- For one generic temperature channel, prefer canonical_id=temp rather than temperature; for multiple temperature channels use temp_local/temp_ext1/temp_remote as appropriate.",
            "- If a datasheet exposes local/internal and remote/external temperature channels, prefer temp_local, temp_ext1, temp_ext2 when that is the clearest public vocabulary.",
            "- If a datasheet exposes axes, prefer accel_x/accel_y/accel_z, gyro_x/gyro_y/gyro_z, mag_x/mag_y/mag_z.",
            "- If the IR name is already clear and compact, canonical_id may equal the source id.",
            "- Return only JSON, with no Markdown fences.",
        ]
    )
    return system_prompt, user_prompt


def normalise_channel_alias_map(
    device_ir: Mapping[str, Any],
    payload: Mapping[str, Any],
) -> Dict[str, Any]:
    """Sanitise the model alias payload and fill missing source channels."""
    records = _read_channel_records(device_ir)
    source_ids = [str(row.get("id") or "").strip() for row in records]
    source_set = {sid for sid in source_ids if sid}
    raw_channels = payload.get("channels") if isinstance(payload, Mapping) else []
    if not isinstance(raw_channels, list):
        raw_channels = []

    channels: List[Dict[str, Any]] = []
    assigned: set[str] = set()
    used_ids: set[str] = set()
    warnings: List[str] = []

    for row in raw_channels:
        if not isinstance(row, Mapping):
            warnings.append("ignored non-object channel alias row")
            continue
        raw_sources = _string_list(row.get("source_channels"))
        valid_sources = [src for src in raw_sources if src in source_set and src not in assigned]
        if not valid_sources:
            invalid = [src for src in raw_sources if src not in source_set]
            if invalid:
                warnings.append(f"ignored unknown source_channels: {invalid}")
            continue
        raw_canonical = _safe_id(row.get("canonical_id") or "")
        aliases = _string_list(row.get("aliases"))
        if len(valid_sources) > 1:
            warnings.append(
                "split a multi-source alias row so each IR read_channel keeps "
                "one public channel"
            )
        for source in valid_sources:
            if len(valid_sources) == 1 and raw_canonical:
                canonical = _canonical_vocabulary_id(raw_canonical, source, device_ir)
            else:
                canonical = _fallback_canonical_id_for_sources([source], device_ir)
            canonical = _dedupe_id(canonical, used_ids)
            used_ids.add(canonical)
            assigned.add(source)
            row_aliases = list(aliases)
            row_aliases.append(source)
            channels.append(
                {
                    "canonical_id": canonical,
                    "source_channels": [source],
                    "aliases": sorted({a for a in row_aliases if a}),
                    "quantity": _safe_text(row.get("quantity")),
                    "location": _safe_text(row.get("location")),
                    "axis": _safe_text(row.get("axis")),
                    "unit": _safe_text(row.get("unit")),
                    "confidence": _coerce_confidence(row.get("confidence")),
                    "evidence": _safe_text(row.get("evidence")),
                }
            )

    for source in source_ids:
        if source and source not in assigned:
            canonical = _dedupe_id(
                _fallback_canonical_id_for_sources([source], device_ir),
                used_ids,
            )
            used_ids.add(canonical)
            assigned.add(source)
            channels.append(
                {
                    "canonical_id": canonical,
                    "source_channels": [source],
                    "aliases": [source],
                    "quantity": "",
                    "location": "",
                    "axis": "",
                    "unit": "",
                    "confidence": 0.5,
                    "evidence": "deterministic fallback for an unmapped IR read_channel",
                }
            )
            warnings.append(f"filled missing source_channel {source!r} with fallback {canonical!r}")

    payload_warnings = payload.get("warnings") if isinstance(payload, Mapping) else []
    warnings.extend(_string_list(payload_warnings))
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or payload.get("device_id") or "unknown"),
        "source": "llm_channel_alias",
        "channels": channels,
        "warnings": warnings,
    }


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
                }
            )
        elif row is not None:
            cid = str(row).strip()
            if cid:
                out.append({"id": cid})
    return out


def _flow_records(device_ir: Mapping[str, Any]) -> List[Dict[str, Any]]:
    rows = device_ir.get("operation_flows") if isinstance(device_ir, Mapping) else None
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows[:12]:
        if not isinstance(row, Mapping):
            continue
        out.append(
            {
                "flow_id": row.get("flow_id") or "",
                "kind": row.get("kind") or "",
                "channels": row.get("channels") or [],
                "notes": str(row.get("notes") or "")[:240],
            }
        )
    return out


def _formula_records(device_ir: Mapping[str, Any]) -> List[Dict[str, Any]]:
    rows = device_ir.get("conversion_formulae") if isinstance(device_ir, Mapping) else None
    if not isinstance(rows, list):
        return []
    out: List[Dict[str, Any]] = []
    for row in rows[:16]:
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
                "name": row.get("name") or row.get("id") or "",
                "formula": row.get("formula") or "",
                "integer_approximation_expression": expr,
                "notes": str(row.get("notes") or "")[:240],
            }
        )
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
    raise ValueError("channel alias response contains no JSON object")


def _fallback_alias_map(device_ir: Mapping[str, Any], reason: str) -> Dict[str, Any]:
    channels: List[Dict[str, Any]] = []
    used_ids: set[str] = set()
    for row in _read_channel_records(device_ir):
        source = str(row.get("id") or "")
        canonical = _dedupe_id(_fallback_canonical_id(source), used_ids)
        used_ids.add(canonical)
        channels.append(
            {
                "canonical_id": canonical,
                "source_channels": [source],
                "aliases": [source],
                "confidence": 0.45,
                "evidence": f"fallback after model alias call failed: {reason}",
            }
        )
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "channels": channels,
        "warnings": [f"fallback map used: {reason}"],
    }


def _empty_alias_map(device_ir: Mapping[str, Any], reason: str) -> Dict[str, Any]:
    return {
        "version": 1,
        "device_id": str(device_ir.get("device_id") or "unknown"),
        "source": "llm_channel_alias",
        "channels": [],
        "warnings": [reason],
        "cache_status": "skipped",
    }


def _fallback_canonical_id_for_sources(
    sources: Sequence[str],
    device_ir: Mapping[str, Any],
) -> str:
    if sources:
        source = sources[0]
        records = _read_channel_records(device_ir)
        row = next((r for r in records if r.get("id") == source), {})
        if _looks_like_temperature_channel(source, row):
            return _temperature_vocabulary_id(source, records)
        return _fallback_canonical_id(source)
    return "channel"


def _canonical_vocabulary_id(
    candidate: str,
    source: str,
    device_ir: Mapping[str, Any],
) -> str:
    """Apply generic public-channel vocabulary to a candidate id."""
    records = _read_channel_records(device_ir)
    row = next((r for r in records if r.get("id") == source), {})
    if _looks_like_temperature_channel(candidate, row) or _looks_like_temperature_channel(source, row):
        canonical = _temperature_vocabulary_id(source, records)
        if canonical:
            return canonical
    canonical = _motion_vocabulary_id(candidate, source, row)
    if canonical:
        return canonical
    canonical = _environment_vocabulary_id(candidate, source, row)
    if canonical:
        return canonical
    return candidate or _fallback_canonical_id_for_sources([source], device_ir)


def _looks_like_temperature_channel(name: str, row: Mapping[str, Any]) -> bool:
    text = " ".join(
        str(part or "")
        for part in (
            name,
            row.get("physical_unit"),
            row.get("description"),
            row.get("formula_id"),
            row.get("notes"),
        )
    ).lower()
    return any(tok in text for tok in ("temp", "thermal", "degc", "celsius", "diode"))


def _temperature_vocabulary_id(
    source: str,
    records: Sequence[Mapping[str, Any]],
) -> str:
    source_tokens = _safe_id(source).split("_")
    all_sources = [str(row.get("id") or "") for row in records]
    kinds = [_temperature_kind(src) for src in all_sources]
    remote_count = sum(1 for kind, _idx in kinds if kind == "remote")
    ext_count = sum(1 for kind, _idx in kinds if kind == "ext")
    temp_count = sum(
        1
        for row in records
        if _looks_like_temperature_channel(str(row.get("id") or ""), row)
    )
    kind, idx = _temperature_kind(source)
    if kind == "local":
        return "temp_local"
    if kind == "remote":
        if remote_count <= 1:
            return "temp_remote"
        return f"temp_remote{idx}" if idx else "temp_remote"
    if kind == "ext":
        if idx:
            return f"temp_ext{idx}"
        return "temp_ext" if ext_count <= 1 else "temp_ext_unknown"
    if any(tok in {"temperature", "temp", "thermal"} for tok in source_tokens):
        if temp_count <= 1:
            return "temp"
        return _fallback_canonical_id(source)
    return ""


def _temperature_kind(source: str) -> Tuple[str, str]:
    tokens = [tok for tok in _safe_id(source).split("_") if tok]
    token_set = set(tokens)
    if token_set & {"internal", "local", "ambient", "int", "onchip", "die"}:
        return "local", ""
    if (
        token_set & {"external", "ext"}
        or "diode" in token_set
        or any(tok.startswith("external") or tok.startswith("ext") for tok in tokens)
    ):
        return "ext", _first_index(tokens)
    if token_set & {"remote", "rem"} or any(tok.startswith("remote") or tok.startswith("rem") for tok in tokens):
        return "remote", _first_index(tokens)
    return "", ""


def _fallback_canonical_id(name: str) -> str:
    safe = _safe_id(name)
    compact = safe
    tokens = [tok for tok in compact.split("_") if tok]
    token_set = set(tokens)

    if token_set & {"temperature", "temp", "thermal", "diode"}:
        if token_set & {"internal", "local", "ambient", "int"}:
            return "temp_local"
        ext_index = _first_index(tokens)
        if ext_index:
            return f"temp_ext{ext_index}"
        if token_set & {"external", "remote", "ext", "rem"}:
            return "temp_ext"
        if compact.endswith("_temperature"):
            return compact[: -len("_temperature")] or "temperature"
    for family, prefix in (
        ({"accelerometer", "acceleration", "accel"}, "accel"),
        ({"gyroscope", "gyro"}, "gyro"),
        ({"magnetometer", "magnetic", "mag"}, "mag"),
    ):
        if token_set & family:
            axis = _axis_token(tokens)
            if axis:
                return f"{prefix}_{axis}"
    if token_set & {"pressure", "press"}:
        return "pressure"
    if token_set & {"humidity", "hum"}:
        return "humidity"
    if token_set & {"light", "lux", "illuminance"}:
        return "illuminance"
    return compact or "channel"


def _first_index(tokens: Sequence[str]) -> str:
    for i, tok in enumerate(tokens):
        if tok.isdigit():
            return tok
        m = re.fullmatch(r"(?:ch|channel|ext|external|remote|rem)(\d+)", tok)
        if m:
            return m.group(1)
        if tok in {"external", "remote", "ext", "rem"} and i + 1 < len(tokens):
            nxt = tokens[i + 1]
            if nxt.isdigit():
                return nxt
    return ""


def _axis_token(tokens: Sequence[str]) -> str:
    for tok in tokens:
        if tok in {"x", "y", "z"}:
            return tok
        m = re.fullmatch(r".*_([xyz])", tok)
        if m:
            return m.group(1)
    return ""


def _motion_vocabulary_id(
    candidate: str,
    source: str,
    row: Mapping[str, Any],
) -> str:
    """Canonicalize generic 3-axis motion-sensor channel vocabulary."""
    text = _channel_text(candidate, source, row)
    tokens = [tok for tok in _safe_id(text).split("_") if tok]
    token_set = set(tokens)
    families = (
        ({"accelerometer", "acceleration", "accelerations", "accel", "acc"}, "accel"),
        ({"gyroscope", "gyroscopic", "gyro", "gyr"}, "gyro"),
        ({"magnetometer", "magnetic", "magnet", "mag"}, "mag"),
    )
    axis = _axis_token(tokens)
    for words, prefix in families:
        if token_set & words:
            return f"{prefix}_{axis}" if axis else prefix
    return ""


def _environment_vocabulary_id(
    candidate: str,
    source: str,
    row: Mapping[str, Any],
) -> str:
    """Canonicalize common environmental channel vocabulary."""
    text = _channel_text(candidate, source, row)
    tokens = [tok for tok in _safe_id(text).split("_") if tok]
    token_set = set(tokens)
    if token_set & {"pressure", "press", "barometric", "baro", "pa", "hpa"}:
        return "pressure"
    if token_set & {"humidity", "humid", "relative_humidity", "rh", "percent_rh"}:
        return "humidity"
    if token_set & {"illuminance", "illumination", "light", "lux"}:
        return "illuminance"
    if token_set & {"distance", "dist", "range", "ranging", "tof"}:
        return "distance"
    if "co2" in token_set:
        return "co2"
    if token_set & {"tvoc", "voc"}:
        return "tvoc"
    return ""


def _channel_text(candidate: str, source: str, row: Mapping[str, Any]) -> str:
    parts: List[str] = [candidate, source]
    for key in ("physical_unit", "description", "formula_id", "raw_type", "notes"):
        value = row.get(key)
        if value:
            parts.append(str(value))
    source_bytes = row.get("source_bytes")
    if isinstance(source_bytes, list):
        parts.extend(str(v) for v in source_bytes)
    return " ".join(parts)


def _safe_id(value: Any) -> str:
    text = str(value or "").strip().lower()
    text = _WS_RE.sub("_", text)
    text = _SAFE_ID_RE.sub("_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    if text and text[0].isdigit():
        text = f"ch_{text}"
    return text


def _safe_text(value: Any) -> str:
    return str(value or "").strip()


def _string_list(value: Any) -> List[str]:
    if isinstance(value, list):
        return [str(item).strip() for item in value if str(item).strip()]
    if isinstance(value, tuple):
        return [str(item).strip() for item in value if str(item).strip()]
    if isinstance(value, str) and value.strip():
        return [value.strip()]
    return []


def _coerce_confidence(value: Any) -> float:
    try:
        f = float(value)
    except (TypeError, ValueError):
        return 0.0
    return max(0.0, min(1.0, f))


def _dedupe_id(base: str, used_ids: set[str]) -> str:
    candidate = base or "channel"
    if candidate not in used_ids:
        return candidate
    idx = 2
    while f"{candidate}_{idx}" in used_ids:
        idx += 1
    return f"{candidate}_{idx}"


def _dump_alias_map(run_dir: Optional[Path], payload: Mapping[str, Any]) -> None:
    if run_dir is None:
        return
    try:
        path = Path(run_dir) / "channel_alias_map.json"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    except OSError:
        return


def _dump_raw_alias_response(run_dir: Optional[Path], text: str) -> None:
    if run_dir is None or not text:
        return
    try:
        path = Path(run_dir) / "channel_alias_raw_response.txt"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
    except OSError:
        return


__all__ = [
    "build_channel_alias_prompt",
    "build_or_load_channel_alias_map",
    "channel_alias_schema",
    "normalise_channel_alias_map",
    "should_build_channel_alias_map",
]
