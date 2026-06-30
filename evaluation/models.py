"""Data models for the DriverGen evaluation layer."""
from __future__ import annotations

from dataclasses import dataclass, field, asdict
from typing import Any, Dict, List, Literal, Optional

# ---- trace primitives ------------------------------------------------------

@dataclass
class I2CTransaction:
    """One full START..STOP sequence on the I2C bus."""
    seq: int
    addr: Optional[int]            # 7-bit slave address (None if NACK)
    is_read: Optional[bool]        # direction of last address byte
    tx_bytes: List[int] = field(default_factory=list)
    rx_bytes: List[int] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "I2CTransaction":
        return cls(
            seq=int(d["seq"]),
            addr=(None if d.get("addr") is None else int(d["addr"])),
            is_read=(None if d.get("is_read") is None else bool(d["is_read"])),
            tx_bytes=[int(b) & 0xFF for b in d.get("tx_bytes", [])],
            rx_bytes=[int(b) & 0xFF for b in d.get("rx_bytes", [])],
        )


@dataclass
class I2CTrace:
    """Ordered sequence of transactions observed during one test run."""
    device: str
    source: str                    # "generated" | "golden" | "reference"
    transactions: List[I2CTransaction] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "device": self.device,
            "source": self.source,
            "transactions": [t.to_dict() for t in self.transactions],
        }

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "I2CTrace":
        return cls(
            device=str(d["device"]),
            source=str(d.get("source", "unknown")),
            transactions=[I2CTransaction.from_dict(t) for t in d.get("transactions", [])],
        )


# ---- verdicts --------------------------------------------------------------

# Claim strength labels.
# The evaluation layer MUST attach one of these labels to every verdict so
# downstream readers never confuse "compiled" with "physically correct".
ClaimStrength = Literal[
    "build-valid",            # L1 only
    "runtime-smoke-valid",    # L2 boot + no fault
    "protocol-valid-strict",  # L3 byte-exact match vs golden
    "protocol-valid-semantic",# L3 register-set match vs golden
    "protocol-valid-relaxed", # L3 required-writes present
    "semantic-valid",         # L4 physical reading within tolerance
    "robust-valid",           # L5 error injection survived
]

Level = Literal["L1", "L2", "L3", "L4", "L5"]


@dataclass
class LevelVerdict:
    """Verdict for one (device, level) pair."""
    device: str
    level: Level
    passed: bool
    claim: ClaimStrength
    detail: str = ""
    evidence: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class EvaluationReport:
    """Aggregate verdicts for one driver under test."""
    device: str
    combo: str                     # e.g. "bh1750_rtthread_stm32f103rb"
    verdicts: List[LevelVerdict] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "device": self.device,
            "combo": self.combo,
            "verdicts": [v.to_dict() for v in self.verdicts],
        }
