"""pipeline step - task-spec normalisation."""

from __future__ import annotations

import logging
import re
from typing import Mapping

from .aliases import canonicalize_rtos_id
from .config import load_bus_taxonomy
from .types import (
    BusIntent,
    SlotPlan,
    SourceRoot,
    TaskSpec,
)

logger = logging.getLogger(__name__)

_DEFAULT_CANONICAL_BUS = "unknown"


# BusIntent


def _allowed_canonical_buses(taxonomy: Mapping) -> set[str]:
    return set(taxonomy.get("canonical_buses") or []) | {_DEFAULT_CANONICAL_BUS}


def _allowed_modes(taxonomy: Mapping) -> set[str]:
    return set(taxonomy.get("modes") or [])


_CONNECTION_SPLIT_RE = re.compile(r"[_\-]")


def _heuristic_canonical_bus(connection_type: str, taxonomy: Mapping) -> str:
    """Split ``connection_type`` on ``_`` or ``-`` and take the first segment if it's a known canonical_bus."""
    if not connection_type:
        return _DEFAULT_CANONICAL_BUS
    parts = _CONNECTION_SPLIT_RE.split(connection_type, maxsplit=1)
    head = parts[0].strip().lower()
    allowed = _allowed_canonical_buses(taxonomy)
    if head in allowed:
        return head
    return _DEFAULT_CANONICAL_BUS


def _heuristic_mode(connection_type: str, taxonomy: Mapping) -> str | None:
    if not connection_type:
        return None
    parts = _CONNECTION_SPLIT_RE.split(connection_type)
    if len(parts) < 2:
        return None
    tail = parts[-1].strip().lower()
    if tail in _allowed_modes(taxonomy):
        return tail
    return None


def canonicalize_bus_and_connection(
    *,
    connection_binding: Mapping | None,
    device_ir: Mapping | None,
    taxonomy: Mapping | None = None,
) -> BusIntent:
    """Build a :class:`BusIntent` from one task package's context."""
    cb = dict(connection_binding or {})
    ir = dict(device_ir or {})

    if taxonomy is None:
        taxonomy = load_bus_taxonomy()

    connection_type = (cb.get("connection_type") or "").strip()

    # Prefer explicit mode, then taxonomy or heuristic inference.
    explicit_mode_raw = cb.get("mode")
    explicit_mode: str | None = None
    if isinstance(explicit_mode_raw, str):
        cleaned = explicit_mode_raw.strip().lower()
        if cleaned and cleaned in _allowed_modes(taxonomy):
            explicit_mode = cleaned

    entry = (taxonomy.get("connection_types") or {}).get(connection_type)
    if entry:
        canonical_bus = entry.get("canonical_bus") or _DEFAULT_CANONICAL_BUS
        mode = explicit_mode or entry.get("mode")
        # Pass taxonomy default intents through to slot derivation.
        default_intents = entry.get("default_query_intents") or []
    else:
        canonical_bus = _heuristic_canonical_bus(connection_type, taxonomy)
        mode = explicit_mode or _heuristic_mode(connection_type, taxonomy)
        default_intents = []
        # Warn only when the connection cannot be mapped to a known bus.
        if connection_type and canonical_bus == _DEFAULT_CANONICAL_BUS:
            logger.warning(
                "Unknown connection_type '%s' — using heuristic canonical_bus='%s' / mode=%s. "
                "Add it to bus_taxonomy.json/connection_types to silence this warning.",
                connection_type,
                canonical_bus,
                mode,
            )

    # Use device metadata as a fallback when binding remains unknown.
    if canonical_bus == _DEFAULT_CANONICAL_BUS:
        ir_bus = (ir.get("bus_type") or "").strip().lower()
        if ir_bus:
            head = _CONNECTION_SPLIT_RE.split(ir_bus, maxsplit=1)[0]
            if head in _allowed_canonical_buses(taxonomy):
                canonical_bus = head
    else:
        # Warn when task metadata disagrees with the resolved binding.
        ir_bus = (ir.get("bus_type") or "").strip().lower()
        if ir_bus:
            ir_head = _CONNECTION_SPLIT_RE.split(ir_bus, maxsplit=1)[0]
            allowed = _allowed_canonical_buses(taxonomy)
            if (
                ir_head
                and ir_head != canonical_bus
                and ir_head in allowed
            ):
                logger.warning(
                    "Bus mismatch: connection_type='%s' resolved canonical_bus='%s' "
                    "but device_ir.bus_type='%s' suggests '%s'. Connection binding "
                    "wins; verify the task package if this is unintentional.",
                    connection_type, canonical_bus, ir_bus, ir_head,
                )

    backend = cb.get("backend") or None
    address_mode = cb.get("address_mode") or None
    bus_instance = cb.get("bus_instance") or None

    # Normalize taxonomy intents before downstream merging.
    default_intents_clean: list[str] = []
    seen_intents: set[str] = set()
    for it in default_intents:
        if not isinstance(it, str):
            continue
        s = it.strip()
        if s and s not in seen_intents:
            seen_intents.add(s)
            default_intents_clean.append(s)

    return BusIntent(
        canonical_bus=canonical_bus,
        connection_type=connection_type,
        mode=mode,
        backend=backend,
        address_mode=address_mode,
        bus_instance=bus_instance,
        default_query_intents=default_intents_clean,
    )


# Transaction shape helpers


def _detect_transaction_shape(device_ir: Mapping | None) -> str | None:
    """Reproduces ``contract_builder._detect_transaction_shape``."""
    if not device_ir:
        return None
    read_sequence = device_ir.get("read_sequence") or []
    if not isinstance(read_sequence, list):
        return "unknown"

    step_texts: list[str] = []
    for step in read_sequence:
        if isinstance(step, dict):
            value = (
                step.get("operation")
                or step.get("op")
                or step.get("action")
                or step.get("step")
                or step.get("description")
                or ""
            )
        else:
            value = str(step)
        step_texts.append(str(value).lower())

    has_register_pointer = any(
        ("register" in t or "pointer" in t or "address" in t) and "write" in t
        for t in step_texts
    )
    has_command_write = any(
        "command" in t or "opcode" in t or "mode" in t for t in step_texts
    )
    has_delay = any("delay" in t or "wait" in t for t in step_texts)
    has_read = any("read" in t for t in step_texts)

    if has_register_pointer and has_read:
        return "register_pointer_then_read"
    if has_command_write and has_delay and has_read:
        return "command_write_then_delay_then_read"
    if has_command_write and has_read:
        return "command_write_then_read"
    if has_read:
        return "direct_read"
    return "unknown"


# TaskSpec


def _coerce_mapping(value) -> dict:
    return dict(value) if isinstance(value, Mapping) else {}


def _canonicalize_board(value) -> str | None:
    """Strip whitespace + drop empty."""
    if not isinstance(value, str):
        return None
    s = value.strip()
    return s or None


def _canonicalize_str(value) -> str | None:
    """Like :func:`_canonicalize_board` but generic — used for
    ``mcu_family / integration / integration_style`` fields.
    """
    if not isinstance(value, str):
        return None
    s = value.strip()
    return s or None


def build_task_spec(
    *,
    platform_base_context: Mapping | None,
    connection_binding: Mapping | None,
    device_attachment: Mapping | None,
    device_ir: Mapping | None,
    source_roots: list[SourceRoot],
    slot_plan: SlotPlan | None = None,
    taxonomy: Mapping | None = None,
) -> TaskSpec:
    """Assemble a :class:`TaskSpec` from the task package's four context fragments + the already-resolved source roots."""
    platform = _coerce_mapping(platform_base_context)
    cb = _coerce_mapping(connection_binding)
    da = _coerce_mapping(device_attachment)
    ir = _coerce_mapping(device_ir)

    if not source_roots:
        raise ValueError(
            "build_task_spec: source_roots must be non-empty; resolve_source_roots() "
            "should be called before build_task_spec()."
        )

    bus_intent = canonicalize_bus_and_connection(
        connection_binding=cb,
        device_ir=ir,
        taxonomy=taxonomy,
    )

    return TaskSpec(
        rtos_id=canonicalize_rtos_id(platform.get("rtos")),
        board=_canonicalize_board(platform.get("board")),
        mcu_family=_canonicalize_str(platform.get("mcu_family")),
        integration=_canonicalize_str(platform.get("integration")),
        integration_style=_canonicalize_str(platform.get("integration_style")),
        bus_intent=bus_intent,
        connection_binding=cb,
        device_attachment=da,
        device_id=da.get("device_id") or ir.get("device_id"),
        device_transaction_shape=_detect_transaction_shape(ir),
        source_roots=list(source_roots),
        slot_plan=slot_plan,
    )


__all__ = [
    "canonicalize_bus_and_connection",
    "build_task_spec",
]
