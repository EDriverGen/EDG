"""evaluation.runtime.trace_io - load/save I2C traces."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Union

from evaluation.models import I2CTrace, I2CTransaction

PathLike = Union[str, Path]


def load_jsonl_trace(path: PathLike, device: str, source: str = "generated") -> I2CTrace:
    """Parse a JSONL trace file produced by the I2C model into an I2CTrace."""
    p = Path(path)
    txns: list[I2CTransaction] = []
    if p.exists():
        for line in p.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            try:
                txns.append(I2CTransaction.from_dict(obj))
            except (KeyError, ValueError, TypeError):
                continue
    return I2CTrace(device=device, source=source, transactions=txns)


def save_trace_json(trace: I2CTrace, path: PathLike) -> None:
    """Save an I2CTrace as pretty JSON. Used for golden / reference traces."""
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(trace.to_dict(), indent=2), encoding="utf-8")


def load_trace_json(path: PathLike) -> I2CTrace:
    """Load a pretty-JSON trace saved by :func:`save_trace_json`."""
    p = Path(path)
    obj = json.loads(p.read_text(encoding="utf-8"))
    return I2CTrace.from_dict(obj)


__all__ = ["load_jsonl_trace", "save_trace_json", "load_trace_json"]
