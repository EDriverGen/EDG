"""pipeline step - Transaction Template Translator."""

from __future__ import annotations

import logging
import re
from dataclasses import asdict, dataclass, field

from .types import (
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SlotPlan,
    SymbolBinding,
    TaskSpec,
)

logger = logging.getLogger(__name__)


@dataclass
class TransactionStep:
    """One step inside a :class:`TransactionTemplate`."""

    op: str
    args: dict = field(default_factory=dict)
    slot_id: str | None = None
    bound_symbol: str | None = None
    derivation: str = "structured"
    confidence: float = 0.5
    description: str = ""


@dataclass
class TransactionTemplate:
    """A full transaction sequence (init / read / write / etc.)."""

    template_id: str
    steps: list[TransactionStep] = field(default_factory=list)
    derivation: str = "structured"
    """``structured`` / ``rule`` / ``llm`` / ``shape`` — overall
    derivation path used.  When the template uses multiple paths the
    field reflects the *weakest* path (so callers can decide whether
    to trust it)."""

    confidence: float = 0.5
    requires_human: bool = False
    notes: str = ""


# Static maps


# Map (canonical_bus, transaction.kind) → preferred slot id when
# present in the slot_plan.  Multiple slot ids are tried in order.
_BUS_KIND_SLOT_PRIORITY: dict[tuple[str, str], list[str]] = {
    ("i2c", "write"): ["i2c.command_write", "i2c.write"],
    ("i2c", "read"): ["i2c.read"],
    ("i2c", "write_then_read"): [
        "i2c.write_then_read",
        "i2c.command_write_then_delay_then_read",
        "i2c.transfer",
    ],
    ("spi", "write"): ["spi.write", "spi.transfer"],
    ("spi", "read"): ["spi.read", "spi.transfer"],
    ("spi", "write_then_read"): ["spi.transfer"],
    ("uart", "write"): ["uart.write", "uart.send"],
    ("uart", "read"): ["uart.read", "uart.recv"],
}

# Substrings that suggest a description-only step actually maps to a
# specific GPIO / timing op.  Used in the ``rule`` derivation path.
_RULE_PATTERNS: list[tuple[re.Pattern, str, str]] = [
    (re.compile(r"\bdelay\b.*\b(?:us|microsecond)", re.IGNORECASE), "delay_us", "runtime.delay_us"),
    (re.compile(r"\bdelay\b.*\b(?:ms|millisecond)", re.IGNORECASE), "delay_ms", "runtime.delay_ms"),
    (re.compile(r"\bwait\b.*\b(?:conversion|measurement)\b", re.IGNORECASE), "delay_ms", "runtime.delay_ms"),
    (re.compile(r"\bdrive\b.*\bhigh\b", re.IGNORECASE), "gpio_drive_high", "gpio.write"),
    (re.compile(r"\bdrive\b.*\blow\b", re.IGNORECASE), "gpio_drive_low", "gpio.write"),
    (re.compile(r"\bset\b.*\bhigh\b", re.IGNORECASE), "gpio_drive_high", "gpio.write"),
    (re.compile(r"\bset\b.*\blow\b", re.IGNORECASE), "gpio_drive_low", "gpio.write"),
    (re.compile(r"\bread\b.*\bpin\b", re.IGNORECASE), "gpio_read", "gpio.read"),
    (re.compile(r"\b(measure|width|pulse)\b", re.IGNORECASE), "wait_pulse_high", "timing.measure_pulse_width"),
]


# Helpers


def _first_bound_slot(
    slot_ids: list[str], bindings: dict[str, SymbolBinding]
) -> tuple[str | None, str | None]:
    """Return ``(slot_id, symbol)`` for the first ``slot_ids[i]`` that
    has a binding; ``(None, None)`` if none of them are bound.
    """
    for sid in slot_ids:
        b = bindings.get(sid)
        if b and b.symbol:
            return sid, b.symbol
    return None, None


def _structured_step(
    step_dict: dict,
    canonical_bus: str | None,
    bindings: dict[str, SymbolBinding],
) -> TransactionStep | None:
    """Translate one ``device_ir.read_sequence`` step into a :class:`TransactionStep` via the ``structured`` path."""
    tx = step_dict.get("transaction")
    if not isinstance(tx, dict):
        return None
    kind = (tx.get("kind") or "").strip().lower()
    if not kind:
        return None
    bus = (canonical_bus or "").lower()
    candidates = _BUS_KIND_SLOT_PRIORITY.get((bus, kind), [])
    if not candidates:
        return None
    slot_id, symbol = _first_bound_slot(candidates, bindings)
    op_map = {
        "write": f"{bus}_write",
        "read": f"{bus}_read",
        "write_then_read": f"{bus}_write_then_read",
    }
    return TransactionStep(
        op=op_map.get(kind, f"{bus}_{kind}"),
        args={
            "bytes": tx.get("bytes") or [],
            "length": tx.get("length"),
            "pointer_target": tx.get("pointer_target"),
            "notes": tx.get("notes"),
        },
        slot_id=slot_id,
        bound_symbol=symbol,
        derivation="structured",
        confidence=0.9 if symbol else 0.4,
        description=str(step_dict.get("step") or step_dict.get("description") or ""),
    )


def _rule_step(
    step_dict: dict, bindings: dict[str, SymbolBinding]
) -> TransactionStep | None:
    """Translate one description-only step via the ``rule`` path."""
    desc = str(step_dict.get("step") or step_dict.get("description") or "")
    if not desc.strip():
        return None
    for pat, op, slot_id in _RULE_PATTERNS:
        if pat.search(desc):
            b = bindings.get(slot_id)
            symbol = b.symbol if b else None
            return TransactionStep(
                op=op,
                args={"description": desc},
                slot_id=slot_id if symbol else None,
                bound_symbol=symbol,
                derivation="rule",
                confidence=0.65 if symbol else 0.3,
                description=desc,
            )
    return None


def _shape_step(step_dict: dict) -> TransactionStep:
    """Final fallback — keep the description, no slot."""
    desc = str(step_dict.get("step") or step_dict.get("description") or "")
    return TransactionStep(
        op="description",
        args={},
        slot_id=None,
        bound_symbol=None,
        derivation="shape",
        confidence=0.1,
        description=desc,
    )


# Public entry


_DERIVATION_ORDER = ("structured", "rule", "llm", "shape")


def _build_template_from_steps(
    *,
    template_id: str,
    sequence_label: str,
    raw_steps: list,
    bindings: dict[str, SymbolBinding],
    bus: str | None,
) -> TransactionTemplate | None:
    """Translate one raw sequence list (init / read / write) into a
    :class:`TransactionTemplate`.  Returns ``None`` when the input
    sequence has no usable steps.
    """
    steps: list[TransactionStep] = []
    derivations_used: set[str] = set()
    for step_dict in raw_steps:
        if not isinstance(step_dict, dict):
            continue
        step = _structured_step(step_dict, bus, bindings)
        if step is None:
            step = _rule_step(step_dict, bindings)
        if step is None:
            step = _shape_step(step_dict)
        steps.append(step)
        derivations_used.add(step.derivation)
    if not steps:
        return None

    weakest = "shape"
    for d in reversed(_DERIVATION_ORDER):
        if d in derivations_used:
            weakest = d
            break

    confidence = sum(s.confidence for s in steps) / max(len(steps), 1)
    requires_human = any(
        s.bound_symbol is None and s.op != "description"
        for s in steps
    )
    return TransactionTemplate(
        template_id=template_id,
        steps=steps,
        derivation=weakest,
        confidence=round(confidence, 3),
        requires_human=requires_human,
        notes=(
            f"Translated from device_ir.{sequence_label} "
            f"({len(raw_steps)} step(s)) via priority chain; "
            f"weakest path used: {weakest}."
        ),
    )


# Sequence keys to walk + their template_id suffix.  Order matters
# because downstream prompt rendering preserves it.
_SEQUENCE_KEYS: tuple[tuple[str, str], ...] = (
    ("init_sequence", "init_sequence"),
    ("write_sequence", "write_sequence"),
    ("read_sequence", "read_sequence"),
)


def build_transaction_templates(
    *,
    device_ir: dict,
    bindings: dict[str, SymbolBinding],
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
) -> list[dict]:
    """Build the ``transaction_templates`` block for one task."""
    if not isinstance(device_ir, dict):
        return []

    bus = task_spec.bus_intent.canonical_bus if task_spec.bus_intent else None
    device_id = task_spec.device_id or "device"

    out: list[dict] = []
    for ir_key, label in _SEQUENCE_KEYS:
        raw = device_ir.get(ir_key)
        if not isinstance(raw, list) or not raw:
            continue
        template = _build_template_from_steps(
            template_id=f"{device_id}_{label}",
            sequence_label=label,
            raw_steps=raw,
            bindings=bindings,
            bus=bus,
        )
        if template is not None:
            out.append(asdict(template))
    return out


__all__ = [
    "TransactionStep",
    "TransactionTemplate",
    "build_transaction_templates",
]
