"""pipeline step - derive a complete :class:`SlotPlan` from a TaskSpec."""

from __future__ import annotations

import logging
from dataclasses import replace
from typing import Iterable, Mapping

from .slot_template_loader import load_slot_template
from .types import (
    SOURCE_KIND_MANIFEST_REPO,
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SlotGoal,
    SlotPlan,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Helpers


def _read_sequence_text(device_ir: Mapping) -> str:
    """Concatenate every readable text fragment of ``device_ir.read_sequence``
    into a single lowercased blob — used by every read-sequence pattern
    rule below.
    """
    rs = device_ir.get("read_sequence") or []
    if not isinstance(rs, list):
        return ""
    parts: list[str] = []
    for step in rs:
        if isinstance(step, dict):
            for key in ("operation", "op", "action", "step", "description"):
                v = step.get(key)
                if isinstance(v, str) and v:
                    parts.append(v.lower())
                    break
        elif isinstance(step, str):
            parts.append(step.lower())
    return " ".join(parts)


def _timing_names(device_ir: Mapping) -> set[str]:
    out: set[str] = set()
    for tc in device_ir.get("timing_constraints") or []:
        if isinstance(tc, dict) and isinstance(tc.get("name"), str):
            out.add(tc["name"].lower())
    return out


def _required_attachment(task_spec: TaskSpec) -> Mapping:
    da = task_spec.device_attachment or {}
    if not isinstance(da, Mapping):
        return {}
    ra = da.get("required_attachment") or {}
    return ra if isinstance(ra, Mapping) else {}


# Derived slot constructors


def _slot(
    slot_id: str,
    *,
    layer: str,
    required: bool,
    origin: str,
    canonical_bus: str | None = None,
    expected_kinds: Iterable[str] = (),
    query_intents: Iterable[str] = (),
    preferred_root_roles: Iterable[str] = (),
    source_kinds_allowed: Iterable[str] = (SOURCE_KIND_MANIFEST_REPO,),
) -> SlotGoal:
    return SlotGoal(
        slot_id=slot_id,
        layer=layer,
        required=required,
        canonical_bus=canonical_bus,
        query_intents=list(query_intents),
        expected_kinds=list(expected_kinds),
        preferred_root_roles=list(preferred_root_roles),
        negative_root_roles=[],
        min_evidence=1,
        origin=origin,
        source_kinds_allowed=list(source_kinds_allowed),
    )


# Read sequence patterns


def _rule_register_pointer_then_read(rs_text: str, bus: str) -> list[SlotGoal]:
    if bus not in {"i2c", "spi"}:
        return []
    has_pointer = (
        ("register" in rs_text or "pointer" in rs_text or "address" in rs_text)
        and "write" in rs_text
    )
    if not (has_pointer and "read" in rs_text):
        return []
    return [
        _slot(
            slot_id=f"{bus}.write_then_read",
            layer="bus",
            required=True,
            origin="read_sequence",
            canonical_bus=bus,
            expected_kinds=["function"],
            query_intents=[
                f"{bus} write read",
                "mem read",
                "register read",
                "write then read",
            ],
            preferred_root_roles=["driver_framework", "vendor_hal"],
        ),
    ]


def _rule_command_write_then_delay_then_read(
    rs_text: str, bus: str
) -> list[SlotGoal]:
    has_command = "command" in rs_text or "opcode" in rs_text or "mode" in rs_text
    has_delay = "delay" in rs_text or "wait" in rs_text
    has_read = "read" in rs_text
    if not (has_command and has_delay and has_read):
        return []
    if bus not in {"i2c", "spi", "uart"}:
        return []
    return [
        _slot(
            slot_id=f"{bus}.command_write_then_delay_then_read",
            layer="bus",
            required=True,
            origin="read_sequence",
            canonical_bus=bus,
            expected_kinds=["function"],
            query_intents=[
                f"{bus} command",
                "issue command",
                "wait conversion",
                "read result",
            ],
            preferred_root_roles=["driver_framework", "vendor_hal"],
        ),
    ]


def _rule_pulse_measurement(rs_text: str, bus: str) -> list[SlotGoal]:
    if bus != "gpio":
        return []
    has_trigger = "trig" in rs_text or "trigger" in rs_text
    has_pulse_or_echo = "pulse" in rs_text or "echo" in rs_text
    if not (has_trigger or has_pulse_or_echo):
        return []
    out = [
        _slot(
            slot_id="gpio.write",
            layer="bus",
            required=True,
            origin="read_sequence",
            canonical_bus="gpio",
            expected_kinds=["function", "macro"],
            query_intents=["gpio write", "set pin", "trigger pulse"],
            preferred_root_roles=["vendor_hal", "driver_framework"],
        ),
        _slot(
            slot_id="gpio.read",
            layer="bus",
            required=True,
            origin="read_sequence",
            canonical_bus="gpio",
            expected_kinds=["function", "macro"],
            query_intents=["gpio read", "echo pin level", "input level"],
            preferred_root_roles=["vendor_hal", "driver_framework"],
        ),
        _slot(
            slot_id="runtime.delay_us",
            layer="runtime",
            required=True,
            origin="read_sequence",
            expected_kinds=["function", "macro", "task_package_helper"],
            query_intents=[
                "microsecond delay",
                "udelay",
                "delay us",
            ],
            preferred_root_roles=["vendor_hal", "kernel", "runtime"],
            source_kinds_allowed=[
                SOURCE_KIND_MANIFEST_REPO,
                SOURCE_KIND_TASK_PACKAGE_HELPER,
            ],
        ),
        _slot(
            slot_id="timing.measure_pulse_width",
            layer="timing",
            required=True,
            origin="read_sequence",
            canonical_bus="gpio",
            expected_kinds=["function", "macro", "task_package_helper"],
            query_intents=[
                "pulse width",
                "elapsed us",
                "monotonic counter",
                "tick to us",
                "input capture",
            ],
            preferred_root_roles=["vendor_hal", "kernel", "runtime"],
            source_kinds_allowed=[
                SOURCE_KIND_MANIFEST_REPO,
                SOURCE_KIND_TASK_PACKAGE_HELPER,
            ],
        ),
    ]
    return out


def _rule_read_sequence_patterns(task_spec: TaskSpec, device_ir: Mapping) -> list[SlotGoal]:
    rs_text = _read_sequence_text(device_ir)
    if not rs_text:
        return []
    bus = task_spec.bus_intent.canonical_bus
    out: list[SlotGoal] = []
    out.extend(_rule_register_pointer_then_read(rs_text, bus))
    out.extend(_rule_command_write_then_delay_then_read(rs_text, bus))
    out.extend(_rule_pulse_measurement(rs_text, bus))
    return out


# Timing-constraint delay


_DELAY_TIMING_NAMES = {
    "measurement_cycle_min_ms",
    "measurement_max_time_ms",
    "conversion_time_ms",
    "conversion_max_time_ms",
    "startup_time_ms",
    "boot_time_ms",
    "ranging_max_time_ms",
}


def _rule_timing_to_delay_ms(device_ir: Mapping) -> list[SlotGoal]:
    names = _timing_names(device_ir)
    if not (names & _DELAY_TIMING_NAMES):
        return []
    return [
        _slot(
            slot_id="runtime.delay_ms",
            layer="runtime",
            required=True,
            origin="timing_constraints",
            expected_kinds=["function", "macro", "task_package_helper"],
            query_intents=[
                "millisecond delay",
                "ms delay",
                "sleep ms",
            ],
            preferred_root_roles=["kernel", "runtime", "vendor_hal"],
            source_kinds_allowed=[
                SOURCE_KIND_MANIFEST_REPO,
                SOURCE_KIND_TASK_PACKAGE_HELPER,
            ],
        ),
    ]


# Address rule


def _rule_addressing(device_ir: Mapping, bus: str) -> list[SlotGoal]:
    ar = device_ir.get("address_rule") or {}
    if not isinstance(ar, Mapping):
        return []
    addr_type = (ar.get("type") or "").strip().lower()
    if not addr_type or addr_type == "none":
        return []
    if bus not in {"i2c", "spi"}:
        return []
    address_intents = [f"{bus} address", "slave address"]
    if "10" in addr_type:
        address_intents.append("10bit address")
    elif "7" in addr_type or "fixed" in addr_type or "slave" in addr_type:
        address_intents.append("7bit address")
    else:
        address_intents.extend(["7bit address", "10bit address"])
    return [
        _slot(
            slot_id=f"{bus}.addressing",
            layer="bus",
            required=True,
            origin="address_rule",
            canonical_bus=bus,
            expected_kinds=["macro", "config"],
            query_intents=address_intents,
            preferred_root_roles=["driver_framework", "vendor_hal"],
        ),
    ]


# Required attachment pins


def _rule_attachment_pins(task_spec: TaskSpec) -> list[SlotGoal]:
    ra = _required_attachment(task_spec)
    out: list[SlotGoal] = []
    # Interrupt mode can require an IRQ slot without attachment details.
    if ra.get("reset_gpio"):
        out.append(
            _slot(
                slot_id="gpio.reset_control",
                layer="bus",
                required=True,
                origin="device_attachment",
                canonical_bus="gpio",
                expected_kinds=["function", "macro"],
                query_intents=[
                    "gpio reset",
                    "reset pin",
                    "device reset",
                    "deassert reset",
                ],
                preferred_root_roles=["vendor_hal", "board_integration"],
            )
        )
    if ra.get("power_gpio"):
        out.append(
            _slot(
                slot_id="gpio.power_control",
                layer="bus",
                required=True,
                origin="device_attachment",
                canonical_bus="gpio",
                expected_kinds=["function", "macro"],
                query_intents=[
                    "power enable",
                    "vdd enable",
                    "power pin",
                    "load switch",
                ],
                preferred_root_roles=["vendor_hal", "board_integration"],
            )
        )
    if ra.get("interrupt_gpio") or task_spec.bus_intent.mode == "interrupt":
        out.append(
            _slot(
                slot_id="gpio.interrupt_configure",
                layer="bus",
                required=True,
                origin="device_attachment",
                canonical_bus="gpio",
                expected_kinds=["function", "macro"],
                query_intents=[
                    "exti",
                    "interrupt pin",
                    "edge trigger",
                    "irq config",
                ],
                preferred_root_roles=["vendor_hal", "board_integration"],
            )
        )
        out.append(
            _slot(
                slot_id="runtime.irq_attach",
                layer="runtime",
                required=True,
                origin="device_attachment",
                expected_kinds=["function", "macro"],
                query_intents=[
                    "irq attach",
                    "register isr",
                    "NVIC EnableIRQ",
                    "interrupt handler",
                ],
                preferred_root_roles=["vendor_hal", "kernel", "runtime"],
            )
        )
    return out


# Fixed attachment pins


def _rule_fixed_attachment_pins(task_spec: TaskSpec) -> list[SlotGoal]:
    """Surface ``connection.fixed_attachment`` as origin metadata on ``integration.pin_binding``."""
    cb = task_spec.connection_binding or {}
    if not isinstance(cb, Mapping):
        return []
    fixed = cb.get("fixed_attachment") or {}
    if not isinstance(fixed, Mapping) or not fixed:
        return []
    return [
        _slot(
            slot_id="integration.pin_binding",
            layer="integration",
            required=True,
            origin="fixed_attachment",
            expected_kinds=["macro", "struct", "config"],
            query_intents=[str(v) for v in fixed.values() if isinstance(v, str)],
            preferred_root_roles=["board_integration", "vendor_hal"],
        ),
    ]


# Bus instance binding


def _rule_bus_instance(task_spec: TaskSpec) -> list[SlotGoal]:
    bi = task_spec.bus_intent.bus_instance
    if not bi:
        return []
    bus = task_spec.bus_intent.canonical_bus or "bus"
    return [
        _slot(
            slot_id="integration.bus_instance_binding",
            layer="integration",
            required=True,
            origin="bus_instance",
            canonical_bus=task_spec.bus_intent.canonical_bus,
            expected_kinds=["macro", "struct", "config"],
            query_intents=[
                str(bi),
                f"{bus} instance",
                "bus binding",
                f"board {bus}",
            ],
            preferred_root_roles=["board_integration", "vendor_hal"],
        ),
    ]


# Merge


def _dedup_preserve_order(seq: list[str]) -> list[str]:
    """Stable dedup: keep first occurrence of each str."""
    seen: set[str] = set()
    out: list[str] = []
    for item in seq:
        if item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def _inject_taxonomy_intents(
    slots: list[SlotGoal], default_intents: list[str], bus: str | None,
) -> list[SlotGoal]:
    """Union ``BusIntent.default_query_intents`` into every base slot whose ``canonical_bus`` matches the task's bus."""
    if not default_intents or not bus:
        return slots
    out: list[SlotGoal] = []
    for s in slots:
        if s.canonical_bus and s.canonical_bus.lower() == bus.lower():
            merged_intents = _dedup_preserve_order(
                list(s.query_intents) + list(default_intents)
            )
            out.append(replace(s, query_intents=merged_intents))
        else:
            out.append(s)
    return out


def _merge(
    base: list[SlotGoal], derived: list[SlotGoal]
) -> tuple[list[SlotGoal], dict[str, list[str]]]:
    """Merge derived slots into base, preserving origin tokens AND union-merging the keyword fields."""
    merged: dict[str, SlotGoal] = {g.slot_id: g for g in base}
    summary: dict[str, list[str]] = {g.slot_id: ["template"] for g in base}

    for d in derived:
        if d.slot_id in merged:
            existing = merged[d.slot_id]
            new_required = existing.required or d.required
            tokens = summary[d.slot_id]
            if d.origin not in tokens:
                tokens.append(d.origin)
            merged[d.slot_id] = replace(
                existing,
                required=new_required,
                query_intents=_dedup_preserve_order(
                    list(existing.query_intents) + list(d.query_intents)
                ),
                expected_kinds=_dedup_preserve_order(
                    list(existing.expected_kinds) + list(d.expected_kinds)
                ),
                preferred_root_roles=_dedup_preserve_order(
                    list(existing.preferred_root_roles)
                    + list(d.preferred_root_roles)
                ),
                negative_root_roles=_dedup_preserve_order(
                    list(existing.negative_root_roles)
                    + list(d.negative_root_roles)
                ),
                source_kinds_allowed=_dedup_preserve_order(
                    list(existing.source_kinds_allowed)
                    + list(d.source_kinds_allowed)
                ),
                min_evidence=max(existing.min_evidence, d.min_evidence),
                origin="+".join(tokens),
            )
        else:
            merged[d.slot_id] = d
            summary[d.slot_id] = [d.origin]

    return list(merged.values()), summary


# Public API


def build_slot_plan(
    *,
    task_spec: TaskSpec,
    device_ir: Mapping | None = None,
) -> SlotPlan:
    """Compose the SlotPlan for one task."""
    connection_type = task_spec.bus_intent.connection_type or ""
    base = load_slot_template(connection_type)

    # Try the canonical connection key when mode is encoded separately.
    if not base:
        canon_bus = task_spec.bus_intent.canonical_bus
        canon_mode = task_spec.bus_intent.mode
        synthesised = (
            f"{canon_bus}_{canon_mode}" if canon_bus and canon_mode else ""
        )
        if synthesised and synthesised != connection_type:
            base = load_slot_template(synthesised)
            if base:
                logger.info(
                    "Slot plan: substituted synthesised connection_type='%s' "
                    "for raw='%s' (bus + mode pair).",
                    synthesised,
                    connection_type,
                )

    if not base:
        logger.warning(
            "build_slot_plan: empty base template for connection_type='%s'; "
            "the resulting SlotPlan will only contain device-derived slots.",
            connection_type,
        )

    # Seed base slots with taxonomy-level query intents.
    base = _inject_taxonomy_intents(
        base,
        task_spec.bus_intent.default_query_intents,
        task_spec.bus_intent.canonical_bus,
    )

    ir = dict(device_ir or {})
    bus = task_spec.bus_intent.canonical_bus or "unknown"

    derived: list[SlotGoal] = []
    derived.extend(_rule_read_sequence_patterns(task_spec, ir))
    derived.extend(_rule_timing_to_delay_ms(ir))
    derived.extend(_rule_addressing(ir, bus))
    derived.extend(_rule_attachment_pins(task_spec))
    derived.extend(_rule_fixed_attachment_pins(task_spec))
    derived.extend(_rule_bus_instance(task_spec))

    slots, summary = _merge(base, derived)

    # Stable sort: required first, then by layer, then by slot_id.
    layer_order = {
        "bus": 0,
        "timing": 1,
        "runtime": 2,
        "integration": 3,
        "board": 4,
        "build": 5,
        "task_helper": 6,
        "exemplar": 7,
    }
    slots.sort(
        key=lambda g: (
            0 if g.required else 1,
            layer_order.get(g.layer, 99),
            g.slot_id,
        )
    )

    return SlotPlan(
        slots=slots,
        connection_type=connection_type,
        derivation_summary=summary,
    )


__all__ = ["build_slot_plan"]
