"""evaluation.oracle.loader - load typed OracleData from disk."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List, Optional

from evaluation.oracle import ORACLE_DATA_DIR
from evaluation.oracle.schema import (
    NackScenario,
    OracleData,
    OracleMeta,
    PhysicalRange,
    RequiredWrite,
    Stimulus,
    default_nack_scenarios,
)


class OracleLoadError(Exception):
    """Raised when an oracle file is missing required fields or invalid."""


def _read_json(p: Path) -> Any:
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except FileNotFoundError:
        raise
    except json.JSONDecodeError as e:
        raise OracleLoadError(f"{p}: invalid JSON ({e})") from e


def _wrap(p: Path, build_fn, payload: Any):
    try:
        return build_fn(payload)
    except (KeyError, ValueError, TypeError) as e:
        raise OracleLoadError(f"{p}: {e}") from e


def load_oracle(
    device_id: str,
    oracle_root: Optional[Path] = None,
) -> OracleData:
    """Load all oracle artifacts for ``device_id`` and return an OracleData."""
    root = Path(oracle_root) if oracle_root is not None else ORACLE_DATA_DIR
    dev_dir = root / device_id
    if not dev_dir.is_dir():
        raise FileNotFoundError(
            f"oracle data dir not found: {dev_dir} "
            f"(expected oracle_root/{device_id}/)"
        )

    meta_path = dev_dir / "meta.json"
    stim_path = dev_dir / "stimuli.json"
    if not meta_path.exists():
        raise FileNotFoundError(f"oracle meta missing: {meta_path}")
    if not stim_path.exists():
        raise FileNotFoundError(f"oracle stimuli missing: {stim_path}")

    meta = _wrap(meta_path, OracleMeta.from_json, _read_json(meta_path))

    stim_payload = _read_json(stim_path)
    stim_list_raw = _coerce_stim_list(stim_path, stim_payload)
    stimuli: List[Stimulus] = []
    for i, raw in enumerate(stim_list_raw):
        try:
            stimuli.append(Stimulus.from_json(raw))
        except (KeyError, ValueError, TypeError) as e:
            raise OracleLoadError(f"{stim_path} stimuli[{i}]: {e}") from e
    if not stimuli:
        raise OracleLoadError(f"{stim_path}: stimuli list is empty")

    rw_path = dev_dir / "required_writes.json"
    required_writes: List[RequiredWrite] = []
    if rw_path.exists():
        rw_payload = _read_json(rw_path)
        rw_list = _coerce_rw_list(rw_path, rw_payload)
        for i, raw in enumerate(rw_list):
            try:
                required_writes.append(RequiredWrite.from_json(raw))
            except (KeyError, ValueError, TypeError) as e:
                raise OracleLoadError(
                    f"{rw_path} required_writes[{i}]: {e}"
                ) from e

    pr_path = dev_dir / "physical_range.json"
    physical_range: Optional[PhysicalRange] = None
    if pr_path.exists():
        pr_payload = _read_json(pr_path)
        pr_default = pr_payload.get("default") if isinstance(pr_payload, dict) else None
        if pr_default:
            physical_range = _wrap(pr_path, PhysicalRange.from_json, pr_default)

    nack_path = dev_dir / "nack_scenarios.json"
    if nack_path.exists():
        np = _read_json(nack_path)
        n_list = _coerce_nack_list(nack_path, np)
        nack_scenarios = []
        for i, raw in enumerate(n_list):
            try:
                nack_scenarios.append(NackScenario.from_json(raw))
            except (KeyError, ValueError, TypeError) as e:
                raise OracleLoadError(
                    f"{nack_path} scenarios[{i}]: {e}"
                ) from e
    else:
        nack_scenarios = default_nack_scenarios()

    gt_path = dev_dir / "golden_trace.json"
    golden_trace: Optional[Dict[str, Any]] = None
    if gt_path.exists():
        gt_payload = _read_json(gt_path)
        if not isinstance(gt_payload, dict) or "transactions" not in gt_payload:
            raise OracleLoadError(
                f"{gt_path}: expected object with 'transactions' field"
            )
        golden_trace = gt_payload

    pe_path = dev_dir / "protocol_equivalence.json"
    protocol_equivalence: Optional[Dict[str, Any]] = None
    if pe_path.exists():
        pe_payload = _read_json(pe_path)
        if not isinstance(pe_payload, dict):
            raise OracleLoadError(
                f"{pe_path}: expected object with optional 'rules' field"
            )
        rules = pe_payload.get("rules")
        if rules is not None and not isinstance(rules, list):
            raise OracleLoadError(f"{pe_path}: 'rules' must be a list")
        protocol_equivalence = pe_payload

    if meta.device_id != device_id:
        raise OracleLoadError(
            f"{meta_path}: meta.device_id={meta.device_id!r} != "
            f"requested {device_id!r}"
        )

    return OracleData(
        meta            = meta,
        stimuli         = stimuli,
        required_writes = required_writes,
        golden_trace    = golden_trace,
        protocol_equivalence = protocol_equivalence,
        physical_range  = physical_range,
        nack_scenarios  = nack_scenarios,
    )


def _coerce_stim_list(path: Path, payload: Any) -> List[Dict[str, Any]]:
    """Accept either a top-level list or {"stimuli": [...]}."""
    if isinstance(payload, list):
        return payload
    if isinstance(payload, dict) and isinstance(payload.get("stimuli"), list):
        return payload["stimuli"]
    raise OracleLoadError(
        f"{path}: expected list or object with 'stimuli' key, got {type(payload).__name__}"
    )


def _coerce_rw_list(path: Path, payload: Any) -> List[Dict[str, Any]]:
    if isinstance(payload, list):
        return payload
    if isinstance(payload, dict) and isinstance(payload.get("required_writes"), list):
        return payload["required_writes"]
    raise OracleLoadError(
        f"{path}: expected list or object with 'required_writes' key"
    )


def _coerce_nack_list(path: Path, payload: Any) -> List[Dict[str, Any]]:
    if isinstance(payload, list):
        return payload
    if isinstance(payload, dict) and isinstance(payload.get("scenarios"), list):
        return payload["scenarios"]
    raise OracleLoadError(
        f"{path}: expected list or object with 'scenarios' key"
    )


__all__ = ["load_oracle", "OracleLoadError"]
