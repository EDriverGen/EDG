"""deterministic bus routing for the generation pipeline."""
from __future__ import annotations

import dataclasses
from typing import Any, List, Mapping, Optional, Tuple

from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
    ClassifyResult,
)


# Runtime constants

RUNTIME_I2C          = "renode_i2c"
RUNTIME_SPI          = "renode_spi"
RUNTIME_UART         = "renode_uart"
RUNTIME_GPIO_PULSE   = "renode_gpio_pulse"
RUNTIME_GPIO_DISCRETE = "renode_gpio_discrete"

SLAVE_I2C_REGISTER   = "i2c_register_slave"
SLAVE_I2C_MEMORY     = "i2c_memory_slave"
SLAVE_I2C_DISPLAY    = "i2c_display_slave"
SLAVE_SPI_GENERIC    = "spi_generic_slave"
SLAVE_UART_BOT       = "uart_request_response_bot"
SLAVE_GPIO_PULSE     = "gpio_pulse_injector"
SLAVE_GPIO_LEVEL     = "gpio_level_slave"   # reserved (not yet implemented)

SPI_SUB_REGISTER = "register"
SPI_SUB_STREAM   = "stream"
SPI_SUB_COMMAND  = "command"

_ALL_SPI_SUBS: Tuple[str, ...] = (SPI_SUB_REGISTER, SPI_SUB_STREAM, SPI_SUB_COMMAND)


# Public result model

@dataclasses.dataclass(frozen=True)
class RoutingResult:
    """Outcome of :func:`route`."""
    runtime_path: str
    slave_kind: str
    spi_sub_mode: str
    bus_kind: str
    confidence: float
    rule_applied: str
    warnings: Tuple[str, ...]


# Helpers

def _as_str(x: Any) -> Optional[str]:
    if isinstance(x, str):
        s = x.strip()
        if s:
            return s
    return None


def _ir_hint_spi_proto(device_ir: Mapping[str, Any]) -> Optional[str]:
    """Look for an explicit ``spi_proto`` in device_ir / task_package."""
    for key in ("spi_proto", "spi_sub_mode"):
        v = _as_str(device_ir.get(key))
        if v and v.lower() in _ALL_SPI_SUBS:
            return v.lower()
    # Also support nested connection metadata.
    for wrapper in ("connection_binding", "connection", "bus_binding"):
        nest = device_ir.get(wrapper)
        if isinstance(nest, Mapping):
            v = _as_str(nest.get("spi_proto"))
            if v and v.lower() in _ALL_SPI_SUBS:
                return v.lower()
    return None


def _task_hint_spi_proto(task_package: Optional[Mapping[str, Any]]) -> Optional[str]:
    if task_package is None:
        return None
    v = _as_str(task_package.get("spi_proto"))
    if v and v.lower() in _ALL_SPI_SUBS:
        return v.lower()
    return None


def _count_registers(device_ir: Mapping[str, Any]) -> int:
    rs = device_ir.get("registers_or_commands")
    if isinstance(rs, list):
        return sum(1 for r in rs if isinstance(r, Mapping))
    return 0


def _register_has_numeric_address(device_ir: Mapping[str, Any]) -> bool:
    """Does the register map expose distinct numeric addresses?."""
    rs = device_ir.get("registers_or_commands")
    if not isinstance(rs, list):
        return False
    count = 0
    for r in rs:
        if not isinstance(r, Mapping):
            continue
        addr = r.get("address") or r.get("register_address") or r.get("addr")
        if isinstance(addr, str) and addr.strip().startswith(("0x", "0X")):
            count += 1
        elif isinstance(addr, int):
            count += 1
    return count >= 2


def _register_has_command_opcodes(device_ir: Mapping[str, Any]) -> bool:
    """Return whether command opcodes appear without register addresses."""
    rs = device_ir.get("registers_or_commands")
    if not isinstance(rs, list):
        return False
    has_opcode_no_addr = 0
    for r in rs:
        if not isinstance(r, Mapping):
            continue
        op = r.get("opcode") or r.get("command") or r.get("cmd16") or r.get("cmd")
        addr = r.get("address") or r.get("register_address") or r.get("addr")
        if op is not None and addr is None:
            has_opcode_no_addr += 1
    return has_opcode_no_addr >= 2


def _read_sequence_text(device_ir: Mapping[str, Any]) -> str:
    """Concatenate free-form text from read_sequence entries."""
    parts = []
    for entry in device_ir.get("read_sequence") or []:
        if isinstance(entry, Mapping):
            for key in ("action", "details", "description", "summary", "mode"):
                v = entry.get(key)
                if isinstance(v, str):
                    parts.append(v)
            inner = entry.get("steps")
            if isinstance(inner, list):
                for item in inner:
                    if isinstance(item, str):
                        parts.append(item)
        elif isinstance(entry, str):
            parts.append(entry)
    return "\n".join(parts).lower()


def _infer_spi_sub_mode(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
) -> Tuple[str, str, float]:
    """Return (sub_mode, reason, confidence)."""
    explicit = _task_hint_spi_proto(task_package) or _ir_hint_spi_proto(device_ir)
    if explicit is not None:
        return explicit, f"explicit spi_proto={explicit!r}", 1.0

    # Empty register maps often indicate a streaming SPI shape.
    if _count_registers(device_ir) == 0:
        txt = _read_sequence_text(device_ir)
        if "clock" in txt or "shift" in txt or "stream" in txt:
            return SPI_SUB_STREAM, "empty registers + streaming read_sequence", 0.80
        return SPI_SUB_STREAM, "empty registers (default stream)", 0.60

    # Register map with numeric addresses selects register mode.
    if _register_has_numeric_address(device_ir):
        return SPI_SUB_REGISTER, "registers_or_commands have numeric addresses", 0.85

    # Register map with opcodes only selects command mode.
    if _register_has_command_opcodes(device_ir):
        return SPI_SUB_COMMAND, "registers_or_commands are opcode-only", 0.80

    # Conservative default for small SPI devices.
    return SPI_SUB_REGISTER, "fallback (register is most common SPI shape)", 0.40


def _infer_gpio_pulse_vs_discrete(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
) -> Tuple[str, str, float]:
    """Return (runtime_path, reason, confidence)."""
    def _gpio_hint_sources(root: Optional[Mapping[str, Any]]) -> List[Mapping[str, Any]]:
        if not isinstance(root, Mapping):
            return []
        out: List[Mapping[str, Any]] = [root]
        conn = root.get("connection_binding") or root.get("connection") or root.get("bus_binding")
        if isinstance(conn, Mapping):
            out.append(conn)
            fixed = conn.get("fixed_attachment")
            if isinstance(fixed, Mapping):
                out.append(fixed)
        hints = root.get("protocol_hints")
        if isinstance(hints, Mapping):
            out.append(hints)
        fixed = root.get("fixed_task_context")
        if isinstance(fixed, Mapping):
            for key in ("device", "connection"):
                sub = fixed.get(key)
                if isinstance(sub, Mapping):
                    out.extend(_gpio_hint_sources(sub))
        return out

    for src in _gpio_hint_sources(task_package) + _gpio_hint_sources(device_ir):
        hint = _as_str(src.get("gpio_protocol_hint"))
        if hint is None:
            hint = _as_str(src.get("gpio_protocol"))
        if hint is not None:
            low = hint.lower()
            if low in {"1-wire", "1wire", "one_wire", "onewire",
                        "1-wire-bitslot", "bitslot-1wire",
                        "dallas_1wire_bitslot", "dallas-1wire-bitslot",
                        "single_wire_pulse_width", "single-wire-pulse-width",
                        "pulse-width", "pulse", "pulse_width", "bit-bang",
                        "timing", "echo", "echo_pulse", "ultrasonic"}:
                return RUNTIME_GPIO_PULSE, f"gpio_protocol_hint={hint!r}", 1.0
            if low in {"discrete", "level", "on_off", "static"}:
                return RUNTIME_GPIO_DISCRETE, f"gpio_protocol_hint={hint!r}", 1.0

    # bus_type may carry the original flavour (gpio_timing / gpio_pulse) even
    # though classify_device normalised it to "gpio". Use it as a strong hint.
    for src in _gpio_hint_sources(task_package) + _gpio_hint_sources(device_ir):
        if not isinstance(src, Mapping):
            continue
        bt = _as_str(src.get("bus_type") or src.get("required_bus_type") or src.get("connection_type"))
        if bt is None:
            continue
        low = bt.lower()
        if low in {"gpio_timing", "gpio_pulse", "gpio_1wire",
                   "gpio_onewire", "one_wire", "onewire", "1-wire"}:
            return RUNTIME_GPIO_PULSE, f"bus_type={bt!r}", 0.9
        if low in {"gpio_discrete", "gpio_level", "gpio_input",
                   "gpio_output", "gpio_inout"}:
            return RUNTIME_GPIO_DISCRETE, f"bus_type={bt!r}", 0.9

    # Fall back to keyword scan of read_sequence text.
    txt = _read_sequence_text(device_ir)
    for kw in ("1-wire", "one-wire", "pulse", "echo pulse",
               "echo-pulse", "timing", "pulse-width"):
        if kw in txt:
            return RUNTIME_GPIO_PULSE, f"read_sequence mentions {kw!r}", 0.65

    # Default to discrete when no pulse/timing evidence is present.
    return RUNTIME_GPIO_DISCRETE, "no protocol hint; default discrete", 0.30


# Public entry

def route(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
    classify_result: ClassifyResult,
) -> RoutingResult:
    """Decide ``(runtime_path, slave_kind, spi_sub_mode)`` for a device."""
    warnings_out = []
    bus = classify_result.bus_type
    ec = classify_result.eval_class

    # I2C / SMBus
    if bus in {"i2c", "smbus"}:
        if ec == EVAL_CLASS_MEMORY:
            return RoutingResult(
                runtime_path=RUNTIME_I2C, slave_kind=SLAVE_I2C_MEMORY,
                spi_sub_mode="", bus_kind="i2c", confidence=1.0,
                rule_applied="i2c+memory", warnings=tuple(warnings_out),
            )
        if ec == EVAL_CLASS_DISPLAY:
            return RoutingResult(
                runtime_path=RUNTIME_I2C, slave_kind=SLAVE_I2C_DISPLAY,
                spi_sub_mode="", bus_kind="i2c", confidence=1.0,
                rule_applied="i2c+display", warnings=tuple(warnings_out),
            )
        # rtc + single_channel + multi_channel all share register slave.
        return RoutingResult(
            runtime_path=RUNTIME_I2C, slave_kind=SLAVE_I2C_REGISTER,
            spi_sub_mode="", bus_kind="i2c", confidence=1.0,
            rule_applied=f"i2c+{ec}", warnings=tuple(warnings_out),
        )

    # SPI
    if bus == "spi":
        sub, reason, sub_conf = _infer_spi_sub_mode(device_ir, task_package)
        return RoutingResult(
            runtime_path=RUNTIME_SPI, slave_kind=SLAVE_SPI_GENERIC,
            spi_sub_mode=sub, bus_kind="spi",
            confidence=round(min(1.0, 0.6 + sub_conf * 0.4), 3),
            rule_applied=f"spi+{sub} ({reason})",
            warnings=tuple(warnings_out),
        )

    # UART
    if bus in {"uart", "uart_polling", "uart_interrupt", "uart_dma"}:
        return RoutingResult(
            runtime_path=RUNTIME_UART, slave_kind=SLAVE_UART_BOT,
            spi_sub_mode="", bus_kind="uart", confidence=1.0,
            rule_applied="uart", warnings=tuple(warnings_out),
        )

    # GPIO
    if bus == "gpio":
        rt, reason, conf = _infer_gpio_pulse_vs_discrete(
            device_ir, task_package,
        )
        slave = SLAVE_GPIO_PULSE if rt == RUNTIME_GPIO_PULSE else SLAVE_GPIO_LEVEL
        if slave == SLAVE_GPIO_LEVEL:
            warnings_out.append(
                f"{SLAVE_GPIO_LEVEL!r} is reserved but not yet implemented; "
                "routing selects it so future devices do not silently fall back "
                "to pulse_injector. Add the slave when the first discrete "
                "GPIO device lands."
            )
        return RoutingResult(
            runtime_path=rt, slave_kind=slave, spi_sub_mode="",
            bus_kind="gpio",
            confidence=round(conf, 3),
            rule_applied=f"gpio ({reason})",
            warnings=tuple(warnings_out),
        )

    # Unknown bus
    warnings_out.append(
        f"unknown bus_type {bus!r}; defaulting to renode_i2c + register slave"
    )
    return RoutingResult(
        runtime_path=RUNTIME_I2C, slave_kind=SLAVE_I2C_REGISTER,
        spi_sub_mode="", bus_kind=bus, confidence=0.1,
        rule_applied="unknown_bus_fallback",
        warnings=tuple(warnings_out),
    )


__all__ = [
    "RUNTIME_I2C", "RUNTIME_SPI", "RUNTIME_UART",
    "RUNTIME_GPIO_PULSE", "RUNTIME_GPIO_DISCRETE",
    "SLAVE_I2C_REGISTER", "SLAVE_I2C_MEMORY", "SLAVE_I2C_DISPLAY",
    "SLAVE_SPI_GENERIC", "SLAVE_UART_BOT",
    "SLAVE_GPIO_PULSE", "SLAVE_GPIO_LEVEL",
    "SPI_SUB_REGISTER", "SPI_SUB_STREAM", "SPI_SUB_COMMAND",
    "RoutingResult",
    "route",
]
