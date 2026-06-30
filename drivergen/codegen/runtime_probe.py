"""Runtime probe wrapper for generated drivers."""
from __future__ import annotations

import ast
import dataclasses
import json
import re
from pathlib import Path
from typing import Any, Iterable, List, Mapping, Optional, Sequence, Tuple

# Keep imports limited to runtime runners.
from evaluation.runtime.gpio_pulse_runner import run_gpio_vector
from evaluation.runtime.i2c_runner import run_i2c_vector
from evaluation.runtime.spi_runner import run_spi_vector
from evaluation.runtime.uart_runner import run_uart_vector

from .classify_device import ClassifyResult
from .ir_to_expected_transactions import _primary_i2c_address
from .route import (
    RUNTIME_GPIO_DISCRETE,
    RUNTIME_GPIO_PULSE,
    RUNTIME_I2C,
    RUNTIME_SPI,
    RUNTIME_UART,
    RoutingResult,
)
from .spi_protocol import spi_protocol_hints


# Runtime defaults

_DEFAULT_SPI_RW_MASK       = 0x80
_DEFAULT_SPI_MB_MASK       = 0x00
_DEFAULT_SPI_ADDR_MASK     = 0x7F
_DEFAULT_SPI_READ_WHEN_SET = True

_DEFAULT_GPIO_PIN_NUMBER = 5
_DEFAULT_GPIO_TRIG_PIN_NUMBER = -1
_DEFAULT_GPIO_PORT_INDEX = 1
_DEFAULT_GPIO_TRIG_PORT_INDEX = -1
_DEFAULT_GPIO_IDLE_LEVEL = 1
_DEFAULT_GPIO_TICK_US    = 1

# Fallback display geometry.
_DEFAULT_DISPLAY_WIDTH  = 128
_DEFAULT_DISPLAY_HEIGHT = 64

# Common port-letter pin labels.
_PORT_PIN_RE = re.compile(
    r"^\s*P([A-E])\s*[,]?\s*(\d{1,2})\s*$", re.IGNORECASE,
)
_GPIO_PORT_LETTERS = {"A": 0, "B": 1, "C": 2, "D": 3, "E": 4}


# Exceptions

class ProbeError(Exception):
    """Raised by :func:`probe_stimulus` for pre-dispatch problems."""


# Dataclasses

@dataclasses.dataclass(frozen=True)
class ProbeMeta:
    """Runtime-only metadata passed to ``evaluation.runtime`` runners."""
    # Required core
    device_id: str
    eval_class: str
    bus_type: str

    description: str = ""

    # I2C / SMBus
    i2c_address_7bit: Optional[int] = None

    # SPI fields are harmless when bus_type != "spi".
    spi_proto: Optional[str] = None
    spi_rw_mask: int = _DEFAULT_SPI_RW_MASK
    spi_mb_mask: int = _DEFAULT_SPI_MB_MASK
    spi_addr_mask: int = _DEFAULT_SPI_ADDR_MASK
    spi_read_when_set: bool = _DEFAULT_SPI_READ_WHEN_SET

    # UART fields are harmless when bus_type != "uart".
    uart_proto: Optional[str] = None
    uart_packet_len: int = 0
    uart_delimiter: Tuple[int, ...] = ()

    # GPIO fields are harmless when bus_type != "gpio".
    gpio_protocol_hint: Optional[str] = None
    gpio_pin_number: int = _DEFAULT_GPIO_PIN_NUMBER
    gpio_trig_pin_number: int = _DEFAULT_GPIO_TRIG_PIN_NUMBER
    gpio_port_index: int = _DEFAULT_GPIO_PORT_INDEX
    gpio_trig_port_index: int = _DEFAULT_GPIO_TRIG_PORT_INDEX
    gpio_idle_level: int = _DEFAULT_GPIO_IDLE_LEVEL
    gpio_tick_us: int = _DEFAULT_GPIO_TICK_US

    # Memory
    memory_size_bytes: int = 0
    memory_page_bytes: int = 0
    i2c_address_size_bytes: int = 0

    # Display
    display_width: int = 0
    display_height: int = 0

    # Accepted for compatibility with existing metadata producers.
    primary: Optional[Any] = None
    channels: Tuple[Any, ...] = ()

    @property
    def bus_kind(self) -> str:
        """SMBus normalises to i2c for runner dispatch."""
        return "i2c" if self.bus_type == "smbus" else self.bus_type


@dataclasses.dataclass(frozen=True)
class ProbeStimulus:
    """One normalized stimulus vector for the runtime probe."""
    name: str
    mock_preload: Mapping[str, Any]

    # Expected fields used after runner execution.
    expected_read_raw: Optional[float] = None
    raw_tolerance: Optional[float] = None
    expected_channels: Optional[Mapping[str, Any]] = None
    channel_preload_bytes: Optional[Mapping[str, Any]] = None
    expected_mem_bytes: Optional[str] = None
    expected_time: Optional[Mapping[str, Any]] = None
    expected_frame_err: Optional[int] = None
    expected_err: Optional[int] = None
    expected_transactions: Tuple[Mapping[str, Any], ...] = ()
    derivation: str = ""
    timeout_s: int = 0

    # Dropped preload keys are reported back as diagnostics.
    dropped_preload_keys: Tuple[Tuple[str, str], ...] = ()


@dataclasses.dataclass(frozen=True)
class ProbeOutcome:
    """Unified outcome from running one stimulus vector."""
    stimulus_name: str

    # Booleans from the harness output parsing
    boot_detected: bool = False
    test_done: bool = False
    result_pass: bool = False
    result_err: bool = False
    read_err: Optional[int] = None

    # Single-channel (canonical)
    read_raw: Optional[float] = None

    # Multi-channel: {channel_id: read_value}
    read_channels: Mapping[str, float] = dataclasses.field(default_factory=dict)

    # Memory-class payload readback
    mem_bytes: Tuple[int, ...] = ()
    mem_probe_addr: Optional[int] = None
    mem_probe_len: Optional[int] = None
    memory_size_bytes: Optional[int] = None
    memory_page_bytes: Optional[int] = None

    # Display-class
    display_frame_len: Optional[int] = None
    display_frame_err: Optional[int] = None
    display_status_err: Optional[int] = None
    display_status: Optional[int] = None

    # RTC-class
    rtc_get_err: Optional[int] = None
    rtc_set_err: Optional[int] = None
    rtc_time: Mapping[str, int] = dataclasses.field(default_factory=dict)

    # Bus trace path (JSONL) for diagnostics
    # feedback on protocol mismatches.
    trace_path: Optional[Path] = None

    # Full harness stdout as a sequence of lines (already split)
    output_lines: Tuple[str, ...] = ()

    # Diagnostic fields
    error: str = ""
    duration_s: float = 0.0
    renode_exit: Optional[int] = None

    # Echo of stimulus expected_* values, so serialised outcomes are
    # self-contained for debugging and diagnosis without the stimulus map.
    expected_read_raw: Optional[float] = None
    expected_channels: Mapping[str, Any] = dataclasses.field(default_factory=dict)
    expected_mem_bytes: Optional[str] = None
    expected_time: Optional[Mapping[str, Any]] = None
    expected_err: Optional[int] = None

    # Routing echo for diagnostics
    runtime_path: str = ""
    slave_kind: str = ""
    spi_sub_mode: str = ""
    bus_kind: str = ""

    @property
    def any_error(self) -> bool:
        return bool(self.error)

    def to_dict(self) -> dict:
        """JSON-serialisable snapshot with simple container types."""
        d = dataclasses.asdict(self)
        if d.get("trace_path") is not None:
            d["trace_path"] = str(d["trace_path"])
        d["mem_bytes"] = list(self.mem_bytes)
        d["output_lines"] = list(self.output_lines)
        d["read_channels"] = dict(self.read_channels)
        d["rtc_time"] = dict(self.rtc_time)
        return d


@dataclasses.dataclass(frozen=True)
class ProbeExpectationCheck:
    """Comparison of one probe outcome with one declared stimulus."""

    ok: bool
    failures: Tuple[str, ...] = ()

    def to_dict(self) -> dict:
        return {"ok": self.ok, "failures": list(self.failures)}

    @property
    def summary(self) -> str:
        if self.ok:
            return "expectations matched"
        return "; ".join(self.failures)


# Preload normalisation

def _coerce_byte(v: Any) -> int:
    """Accept int or numeric string and return an 8-bit value."""
    if isinstance(v, bool):
        raise ValueError(f"byte value cannot be bool: {v!r}")
    if isinstance(v, int):
        return v & 0xFF
    if isinstance(v, str):
        return int(v, 0) & 0xFF
    raise ValueError(f"byte value has unexpected type {type(v).__name__}: {v!r}")


def _parse_json_list_of_ints(s: str) -> Optional[List[int]]:
    """Try to parse a string like ``"[0x10, 0x20]"`` or ``"[16, 32]"``."""
    stripped = s.strip()
    if not (stripped.startswith("[") and stripped.endswith("]")):
        return None
    try:
        parsed = ast.literal_eval(stripped)
    except (ValueError, SyntaxError):
        return None
    if not isinstance(parsed, (list, tuple)):
        return None
    try:
        return [_coerce_byte(x) for x in parsed]
    except ValueError:
        return None


_RENDERER_SENTINEL_KEYS: frozenset = frozenset({
    # I2C / SPI / memory flat-byte response
    "read_bytes",
    # SPI stream-mode preloaded response frame
    "stream",
    # GPIO bit-bang schedule
    "schedule",
    # UART plaintext payload
    "payload",
    # Display test helpers
    "frame_ok", "frame_err", "status_err",
    # RTC pre-loaded time / error injection
    "rtc_time", "rtc_get_err", "rtc_set_err",
    # Bus-level fault injection
    "nack_on_read", "nack_on_write",
})


def _is_renderer_key(k: str) -> bool:
    """True when ``k`` is a key shape the slave renderer can consume."""
    if not k:
        return False
    if k in _RENDERER_SENTINEL_KEYS:
        return True
    for prefix in ("reg_", "req_", "resp_"):
        if k.startswith(prefix):
            tail = k[len(prefix):]
            try:
                int(tail, 0)
                return True
            except ValueError:
                if prefix in ("req_", "resp_"):
                    # Request/response keys may encode byte streams.
                    compact = re.sub(r"[\s_,]", "", tail)
                    return (
                        bool(compact)
                        and len(compact) % 2 == 0
                        and re.fullmatch(r"[0-9a-fA-F]+", compact) is not None
                    )
                return False
    # addr:reg form.
    if ":" in k:
        a, _, b = k.partition(":")
        try:
            int(a, 0)
            int(b, 0)
            return True
        except ValueError:
            return False
    # Plain integer (hex or dec).
    try:
        int(k, 0)
        return True
    except ValueError:
        return False


def _parse_preload_value(v: Any) -> Any:
    """Normalise one ``mock_preload`` value to runner-ready shape."""
    if isinstance(v, list):
        if not v:
            return []
        # Schedule form: every entry is itself a list or tuple of length >=2
        if all(isinstance(x, (list, tuple)) for x in v):
            schedule: List[List[int]] = []
            for i, pair in enumerate(v):
                if len(pair) < 2:
                    raise ValueError(
                        f"schedule pair [{i}] must be [level, duration_us], "
                        f"got {pair!r}"
                    )
                lvl = _coerce_byte(pair[0]) & 1
                dur = int(pair[1], 0) if isinstance(pair[1], str) else int(pair[1])
                schedule.append([lvl, dur])
            return schedule
        # Flat byte list.
        return [_coerce_byte(x) for x in v]

    if isinstance(v, str):
        parsed = _parse_json_list_of_ints(v)
        if parsed is not None:
            return parsed
        # Keep non-list strings as payloads or command keys.
        return v

    if isinstance(v, int):
        return [_coerce_byte(v)]

    raise ValueError(
        f"mock_preload value has unsupported type {type(v).__name__}: {v!r}"
    )


def _normalise_mock_preload(
    raw: Any,
    *,
    dropped_out: Optional[list] = None,
) -> Mapping[str, Any]:
    """Normalise the whole ``mock_preload`` dict, coercing keys to str."""
    if raw is None:
        return {}
    if not isinstance(raw, Mapping):
        raise ValueError(
            f"mock_preload must be a mapping; got {type(raw).__name__}"
        )
    out: dict = {}
    skipped: List[Tuple[str, str]] = []
    for k, v in raw.items():
        key = str(k)
        if isinstance(v, bool):
            skipped.append((key, f"bool value {v!r} ignored"))
            continue
        if isinstance(v, Mapping):
            skipped.append((key, "nested mapping ignored"))
            continue
        # Only keep keys understood by the slave renderer.
        if not _is_renderer_key(key):
            skipped.append((key, "non-renderer key dropped"))
            continue
        try:
            parsed = _parse_preload_value(v)
        except ValueError as ex:
            skipped.append((key, str(ex)))
            continue
        out[key] = parsed
    if skipped:
        # Keep diagnostics separate from the runner preload table.
        import logging
        logging.getLogger(__name__).debug(
            "mock_preload: skipped %d keys: %s",
            len(skipped),
            ", ".join(f"{k} ({reason})" for k, reason in skipped),
        )
        if dropped_out is not None:
            dropped_out.extend(skipped)
    return out


# Build probe meta (factory)

def _as_mapping(x: Any) -> Optional[Mapping[str, Any]]:
    return x if isinstance(x, Mapping) else None


def _pick_i2c_address(
    device_ir: Mapping[str, Any],
    overrides: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    api_contract: Optional[Mapping[str, Any]] = None,
    expected_transactions: Optional[Iterable[Mapping[str, Any]]] = None,
) -> Optional[int]:
    """Canonical 7-bit I2C address, preferring explicit override."""
    def _coerce(ov: Any) -> Optional[int]:
        if ov is None:
            return None
        if isinstance(ov, bool):
            return None
        if isinstance(ov, int):
            return ov & 0xFF
        if isinstance(ov, str):
            s = ov.strip()
            try:
                return int(s, 0) & 0xFF
            except ValueError:
                # Last-ditch: look for the first hex literal anywhere in
                # the string can include a human-readable suffix.
                import re as _re
                m = _re.search(r"0x[0-9a-fA-F]{1,2}", s)
                if m:
                    return int(m.group(0), 16) & 0xFF
        return None

    for src_name, src in (
        ("overrides", overrides),
        ("task_package", task_package),
        ("api_contract", api_contract),
    ):
        if src and "i2c_address_7bit" in src:
            got = _coerce(src["i2c_address_7bit"])
            if got is not None:
                return got
    canonical = _primary_i2c_address(device_ir)
    if canonical is not None:
        try:
            return int(canonical, 16) & 0xFF
        except ValueError:
            pass
    if expected_transactions:
        for tx in expected_transactions:
            if not isinstance(tx, Mapping):
                continue
            addr = tx.get("addr_or_pin") or tx.get("address") or tx.get("addr")
            got = _coerce(addr)
            if got is not None:
                return got
    return None


def _pick_int(
    candidates: Iterable[Any], default: int,
) -> int:
    """Pick the first int-parseable candidate; fall back to default."""
    for c in candidates:
        if isinstance(c, bool):
            continue
        if isinstance(c, int):
            return c
        if isinstance(c, str):
            try:
                return int(c, 0)
            except ValueError:
                continue
    return default


def _pick_str(candidates: Iterable[Any]) -> Optional[str]:
    for c in candidates:
        if isinstance(c, str) and c.strip():
            return c.strip()
    return None


def _parse_port_pin(s: str) -> Optional[int]:
    """Parse port-letter pin strings into a pin index."""
    m = _PORT_PIN_RE.match(s)
    if not m:
        return None
    try:
        return int(m.group(2))
    except ValueError:
        return None


def _parse_gpio_port_index(value: Any) -> Optional[int]:
    """Parse a GPIO bank/port label into a zero-based port index."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value
    if not isinstance(value, str):
        return None
    text = value.strip()
    if not text:
        return None

    m = re.fullmatch(r"(?:GPIO|PORT|P)?([A-E])", text, flags=re.IGNORECASE)
    if m:
        return _GPIO_PORT_LETTERS.get(m.group(1).upper())
    try:
        return int(text, 0)
    except ValueError:
        return None


def _parse_port_pin_ref(s: str) -> Optional[Tuple[int, int]]:
    m = _PORT_PIN_RE.match(s)
    if not m:
        return None
    port = _GPIO_PORT_LETTERS.get(m.group(1).upper())
    if port is None:
        return None
    try:
        return port, int(m.group(2))
    except ValueError:
        return None


def _parse_gpio_pin_ref(value: Any) -> Optional[Tuple[Optional[int], int]]:
    """Parse common board GPIO labels into ``(port_index, pin_index)``."""
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return None, value
    if not isinstance(value, str):
        return None
    text = value.strip()
    if not text:
        return None

    parsed_port_pin = _parse_port_pin_ref(text)
    if parsed_port_pin is not None:
        return parsed_port_pin

    explicit_patterns = (
        r"\bGPIO([A-E])\b\s*:?\s*GPIO_PIN_(\d{1,2})\b",
        r"\bPAL_LINE\(\s*GPIO([A-E])\s*,\s*(\d{1,2})\s*\)",
        r"\bGET_PIN\(\s*(?:GPIO)?([A-E])\s*,\s*(\d{1,2})\s*\)",
    )
    for pattern in explicit_patterns:
        m = re.search(pattern, text, flags=re.IGNORECASE)
        if not m:
            continue
        port = _GPIO_PORT_LETTERS.get(m.group(1).upper())
        if port is None:
            return None
        try:
            return port, int(m.group(2), 0)
        except ValueError:
            return None

    m = re.search(r"\bGPIO_PIN\(\s*(\d{1,2})\s*,\s*(\d{1,2})\s*\)", text, flags=re.IGNORECASE)
    if m:
        try:
            return int(m.group(1), 0), int(m.group(2), 0)
        except ValueError:
            return None

    pin_only_patterns = (
        r"GPIO_PIN_(\d{1,2})\b",
        r"/dev/gpio(\d{1,2})\b",
        r"\bgpio[_-]?(\d{1,2})\b",
    )
    for pattern in pin_only_patterns:
        m = re.search(pattern, text, flags=re.IGNORECASE)
        if m:
            try:
                return None, int(m.group(1), 0)
            except ValueError:
                return None
    try:
        return None, int(text, 0)
    except ValueError:
        return None


def _fixed_attachment_sources(
    task_package: Optional[Mapping[str, Any]],
    device_ir: Mapping[str, Any],
) -> List[Mapping[str, Any]]:
    sources: List[Mapping[str, Any]] = []
    for root in (task_package, device_ir):
        if not isinstance(root, Mapping):
            continue
        fixed = _as_mapping(root.get("fixed_attachment"))
        if fixed:
            sources.append(fixed)
        conn = _as_mapping(root.get("connection_binding"))
        if conn:
            sources.append(conn)
            conn_fixed = _as_mapping(conn.get("fixed_attachment"))
            if conn_fixed:
                sources.append(conn_fixed)
        fixed_task = _as_mapping(root.get("fixed_task_context"))
        if fixed_task:
            for key in ("connection", "device"):
                sub = _as_mapping(fixed_task.get(key))
                if not sub:
                    continue
                sources.append(sub)
                sub_fixed = _as_mapping(sub.get("fixed_attachment"))
                if sub_fixed:
                    sources.append(sub_fixed)
    return sources


def _task_protocol_sources(
    task_package: Optional[Mapping[str, Any]],
    device_ir: Mapping[str, Any],
) -> List[Mapping[str, Any]]:
    """Return mappings that may carry bus/protocol hints."""
    sources: List[Mapping[str, Any]] = []
    for root in (task_package, device_ir):
        if not isinstance(root, Mapping):
            continue
        sources.append(root)
        hints = _as_mapping(root.get("protocol_hints"))
        if hints:
            sources.append(hints)
        for key in ("connection_binding", "connection", "bus_binding"):
            conn = _as_mapping(root.get(key))
            if conn:
                sources.append(conn)
                conn_hints = _as_mapping(conn.get("protocol_hints"))
                if conn_hints:
                    sources.append(conn_hints)
                conn_fixed = _as_mapping(conn.get("fixed_attachment"))
                if conn_fixed:
                    sources.append(conn_fixed)
        fixed_task = _as_mapping(root.get("fixed_task_context"))
        if fixed_task:
            for key in ("device", "connection"):
                sub = _as_mapping(fixed_task.get(key))
                if not sub:
                    continue
                sources.append(sub)
                sub_hints = _as_mapping(sub.get("protocol_hints"))
                if sub_hints:
                    sources.append(sub_hints)
                sub_fixed = _as_mapping(sub.get("fixed_attachment"))
                if sub_fixed:
                    sources.append(sub_fixed)
    return sources


def _first_gpio_port(
    candidates: Iterable[Any],
    default: int,
) -> int:
    for value in candidates:
        parsed = _parse_gpio_port_index(value)
        if parsed is not None:
            return parsed
    return default


def _extract_gpio_pin_ref(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
    overrides: Optional[Mapping[str, Any]],
) -> Tuple[int, int]:
    """Derive the echo/data GPIO ``(port, pin)`` from available artifacts."""
    sources: List[Any] = []
    port_sources: List[Any] = []
    if overrides:
        sources.append(overrides.get("gpio_pin_number"))
        sources.append(overrides.get("gpio_pin"))
        sources.append(overrides.get("gpio_echo_pin_number"))
        sources.append(overrides.get("gpio_echo_pin"))
        sources.append(overrides.get("echo_pin"))
        sources.append(overrides.get("echo_line"))
        port_sources.append(overrides.get("gpio_port_index"))
        port_sources.append(overrides.get("gpio_port"))
        port_sources.append(overrides.get("gpio_echo_port_index"))
        port_sources.append(overrides.get("gpio_echo_port"))
        port_sources.append(overrides.get("echo_port"))
    if task_package:
        sources.append(task_package.get("gpio_pin_number"))
        sources.append(task_package.get("gpio_pin"))
        sources.append(task_package.get("gpio_echo_pin_number"))
        sources.append(task_package.get("gpio_echo_pin"))
        sources.append(task_package.get("echo_pin"))
        sources.append(task_package.get("echo_line"))
        sources.append(task_package.get("echo_path"))
        port_sources.append(task_package.get("gpio_port_index"))
        port_sources.append(task_package.get("gpio_port"))
        port_sources.append(task_package.get("gpio_echo_port_index"))
        port_sources.append(task_package.get("gpio_echo_port"))
        port_sources.append(task_package.get("echo_port"))
        conn = _as_mapping(task_package.get("connection_binding")) or {}
        sources.append(conn.get("gpio_pin_number"))
        sources.append(conn.get("gpio_pin"))
        sources.append(conn.get("echo_pin"))
        sources.append(conn.get("echo_line"))
        sources.append(conn.get("echo_path"))
        port_sources.append(conn.get("gpio_port_index"))
        port_sources.append(conn.get("gpio_port"))
        port_sources.append(conn.get("gpio_echo_port_index"))
        port_sources.append(conn.get("gpio_echo_port"))
        port_sources.append(conn.get("echo_port"))
    sources.append(device_ir.get("gpio_pin_number"))
    sources.append(device_ir.get("gpio_pin"))
    sources.append(device_ir.get("gpio_echo_pin_number"))
    sources.append(device_ir.get("gpio_echo_pin"))
    sources.append(device_ir.get("echo_pin"))
    sources.append(device_ir.get("echo_line"))
    port_sources.append(device_ir.get("gpio_port_index"))
    port_sources.append(device_ir.get("gpio_port"))
    port_sources.append(device_ir.get("gpio_echo_port_index"))
    port_sources.append(device_ir.get("gpio_echo_port"))
    port_sources.append(device_ir.get("echo_port"))
    conn_ir = _as_mapping(device_ir.get("connection_binding")) or {}
    sources.append(conn_ir.get("gpio_pin_number"))
    sources.append(conn_ir.get("gpio_pin"))
    sources.append(conn_ir.get("echo_pin"))
    sources.append(conn_ir.get("echo_line"))
    port_sources.append(conn_ir.get("gpio_port_index"))
    port_sources.append(conn_ir.get("gpio_port"))
    port_sources.append(conn_ir.get("gpio_echo_port_index"))
    port_sources.append(conn_ir.get("gpio_echo_port"))
    port_sources.append(conn_ir.get("echo_port"))
    for fixed in _fixed_attachment_sources(task_package, device_ir):
        sources.append(fixed.get("gpio_pin_number"))
        sources.append(fixed.get("gpio_pin"))
        sources.append(fixed.get("data_pin"))
        sources.append(fixed.get("echo_pin"))
        sources.append(fixed.get("echo_line"))
        sources.append(fixed.get("echo_path"))
        port_sources.append(fixed.get("gpio_port_index"))
        port_sources.append(fixed.get("gpio_port"))
        port_sources.append(fixed.get("gpio_echo_port_index"))
        port_sources.append(fixed.get("gpio_echo_port"))
        port_sources.append(fixed.get("echo_port"))

    for v in sources:
        parsed = _parse_gpio_pin_ref(v)
        if parsed is not None:
            port, pin = parsed
            if port is None:
                port = _first_gpio_port(port_sources, _DEFAULT_GPIO_PORT_INDEX)
            return port, pin
    return _DEFAULT_GPIO_PORT_INDEX, _DEFAULT_GPIO_PIN_NUMBER


def _extract_gpio_trig_pin_ref(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
    overrides: Optional[Mapping[str, Any]],
    *,
    default_port: int,
) -> Tuple[int, int]:
    sources: List[Any] = []
    port_sources: List[Any] = []
    if overrides:
        sources.append(overrides.get("gpio_trig_pin_number"))
        sources.append(overrides.get("gpio_trig_pin"))
        sources.append(overrides.get("trig_pin"))
        sources.append(overrides.get("trig_line"))
        port_sources.append(overrides.get("gpio_trig_port_index"))
        port_sources.append(overrides.get("gpio_trig_port"))
        port_sources.append(overrides.get("trig_port"))
    if task_package:
        sources.append(task_package.get("gpio_trig_pin_number"))
        sources.append(task_package.get("gpio_trig_pin"))
        sources.append(task_package.get("trig_pin"))
        sources.append(task_package.get("trig_line"))
        sources.append(task_package.get("trig_path"))
        port_sources.append(task_package.get("gpio_trig_port_index"))
        port_sources.append(task_package.get("gpio_trig_port"))
        port_sources.append(task_package.get("trig_port"))
        conn = _as_mapping(task_package.get("connection_binding")) or {}
        sources.append(conn.get("trig_pin"))
        sources.append(conn.get("trig_line"))
        sources.append(conn.get("trig_path"))
        port_sources.append(conn.get("gpio_trig_port_index"))
        port_sources.append(conn.get("gpio_trig_port"))
        port_sources.append(conn.get("trig_port"))
    sources.append(device_ir.get("gpio_trig_pin_number"))
    sources.append(device_ir.get("gpio_trig_pin"))
    sources.append(device_ir.get("trig_pin"))
    sources.append(device_ir.get("trig_line"))
    port_sources.append(device_ir.get("gpio_trig_port_index"))
    port_sources.append(device_ir.get("gpio_trig_port"))
    port_sources.append(device_ir.get("trig_port"))
    conn_ir = _as_mapping(device_ir.get("connection_binding")) or {}
    sources.append(conn_ir.get("trig_pin"))
    sources.append(conn_ir.get("trig_line"))
    port_sources.append(conn_ir.get("gpio_trig_port_index"))
    port_sources.append(conn_ir.get("gpio_trig_port"))
    port_sources.append(conn_ir.get("trig_port"))
    for fixed in _fixed_attachment_sources(task_package, device_ir):
        sources.append(fixed.get("gpio_trig_pin_number"))
        sources.append(fixed.get("gpio_trig_pin"))
        sources.append(fixed.get("trig_pin"))
        sources.append(fixed.get("trig_line"))
        sources.append(fixed.get("trig_path"))
        port_sources.append(fixed.get("gpio_trig_port_index"))
        port_sources.append(fixed.get("gpio_trig_port"))
        port_sources.append(fixed.get("trig_port"))

    for value in sources:
        parsed = _parse_gpio_pin_ref(value)
        if parsed is not None:
            port, pin = parsed
            if port is None:
                port = _first_gpio_port(port_sources, default_port)
            return port, pin
    return _DEFAULT_GPIO_TRIG_PORT_INDEX, _DEFAULT_GPIO_TRIG_PIN_NUMBER


def _extract_uart_fields(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
    overrides: Optional[Mapping[str, Any]],
) -> Tuple[Optional[str], int, Tuple[int, ...]]:
    """Return ``(uart_proto, uart_packet_len, uart_delimiter)`` from signals."""
    sources: List[Mapping[str, Any]] = []
    if overrides:
        sources.append(overrides)
    sources.extend(_task_protocol_sources(task_package, device_ir))

    proto = None
    for src in sources:
        v = src.get("uart_proto")
        if isinstance(v, str) and v.strip().lower() in {"fixed", "delimiter"}:
            proto = v.strip().lower()
            break

    packet_len = 0
    delim: List[int] = []
    for src in sources:
        pl = src.get("uart_packet_len") or src.get("frame_len")
        if isinstance(pl, int) and pl > 0 and packet_len == 0:
            packet_len = pl
        raw_delim = src.get("uart_delimiter")
        if isinstance(raw_delim, (list, tuple)) and not delim:
            try:
                delim = [_coerce_byte(x) for x in raw_delim]
            except ValueError:
                delim = []
    return proto, packet_len, tuple(delim)


def _extract_memory_fields(
    device_ir: Mapping[str, Any],
    api_contract: Optional[Mapping[str, Any]],
    overrides: Optional[Mapping[str, Any]],
) -> Tuple[int, int, int]:
    """Return ``(memory_size_bytes, memory_page_bytes, i2c_address_size_bytes)``."""
    srcs: List[Mapping[str, Any]] = []
    if overrides:
        srcs.append(overrides)
    if api_contract:
        srcs.append(api_contract)
    srcs.append(device_ir)

    size = _pick_int(
        (s.get("memory_size_bytes") for s in srcs), default=0,
    )
    page = _pick_int(
        (s.get("memory_page_bytes") for s in srcs), default=0,
    )
    addr_size = _pick_int(
        (s.get("i2c_address_size_bytes") for s in srcs), default=0,
    )
    return size, page, addr_size


def _extract_display_fields(
    device_ir: Mapping[str, Any],
    api_contract: Optional[Mapping[str, Any]],
    overrides: Optional[Mapping[str, Any]],
) -> Tuple[int, int]:
    """Return ``(display_width, display_height)``."""
    srcs: List[Mapping[str, Any]] = []
    if overrides:
        srcs.append(overrides)
    if api_contract:
        srcs.append(api_contract)
    srcs.append(device_ir)
    w = _pick_int((s.get("display_width") for s in srcs), default=0)
    h = _pick_int((s.get("display_height") for s in srcs), default=0)
    return w, h


def build_probe_meta(
    *,
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
    routing_result: RoutingResult,
    api_contract: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    overrides: Optional[Mapping[str, Any]] = None,
    expected_transactions: Optional[Iterable[Mapping[str, Any]]] = None,
) -> ProbeMeta:
    """Assemble a :class:`ProbeMeta` from pipeline artifacts."""
    device_id = _pick_str((
        (overrides or {}).get("device_id"),
        device_ir.get("device_id"),
        classify_result.rule_applied,  # fallback, rarely useful
    )) or "unknown_device"

    # bus_type: routing wins over classify (route normalises "smbus" etc)
    bus_type = routing_result.bus_kind or classify_result.bus_type or "i2c"

    # I2C address
    i2c_addr = None
    if bus_type in {"i2c", "smbus"}:
        i2c_addr = _pick_i2c_address(
            device_ir,
            overrides=overrides,
            task_package=task_package,
            api_contract=api_contract,
            expected_transactions=expected_transactions,
        )

    # SPI fields
    spi_proto = routing_result.spi_sub_mode or None
    spi_hints = spi_protocol_hints(
        device_ir=device_ir,
        task_package=task_package,
        overrides=overrides,
        default_proto=spi_proto,
        default_rw_mask=_DEFAULT_SPI_RW_MASK,
        default_mb_mask=_DEFAULT_SPI_MB_MASK,
        default_addr_mask=_DEFAULT_SPI_ADDR_MASK,
        default_read_when_set=_DEFAULT_SPI_READ_WHEN_SET,
    )
    spi_rw_mask = spi_hints.rw_mask
    spi_mb_mask = spi_hints.mb_mask
    spi_addr_mask = spi_hints.addr_mask
    spi_read_when_set = spi_hints.read_when_set

    # UART fields
    uart_proto, uart_packet_len, uart_delim = _extract_uart_fields(
        device_ir, task_package, overrides,
    )

    # GPIO fields
    protocol_sources = _task_protocol_sources(task_package, device_ir)
    gpio_hint = _pick_str(
        [(overrides or {}).get("gpio_protocol_hint")]
        + [
            src.get("gpio_protocol_hint") or src.get("gpio_protocol")
            for src in protocol_sources
        ]
    )
    gpio_port, gpio_pin = _extract_gpio_pin_ref(
        device_ir, task_package, overrides,
    )
    gpio_trig_port, gpio_trig_pin = _extract_gpio_trig_pin_ref(
        device_ir,
        task_package,
        overrides,
        default_port=gpio_port,
    )
    gpio_idle = _pick_int(
        [(overrides or {}).get("gpio_idle_level")]
        + [
            src.get("gpio_idle_level")
            if src.get("gpio_idle_level") is not None
            else src.get("idle_level")
            for src in protocol_sources
        ],
        default=_DEFAULT_GPIO_IDLE_LEVEL,
    )
    gpio_tick = _pick_int(
        [(overrides or {}).get("gpio_tick_us")]
        + [src.get("gpio_tick_us") for src in protocol_sources],
        default=_DEFAULT_GPIO_TICK_US,
    )
    # Keep the runner metadata valid.
    if gpio_tick <= 0:
        gpio_tick = _DEFAULT_GPIO_TICK_US

    # Memory fields
    mem_size, mem_page, i2c_addr_size = _extract_memory_fields(
        device_ir, api_contract, overrides,
    )

    # Display fields
    disp_w, disp_h = _extract_display_fields(
        device_ir, api_contract, overrides,
    )

    description = _pick_str((
        (overrides or {}).get("description"),
        (task_package or {}).get("description"),
        device_ir.get("description"),
    )) or ""

    return ProbeMeta(
        device_id=device_id,
        eval_class=classify_result.eval_class,
        bus_type=bus_type,
        description=description,
        i2c_address_7bit=i2c_addr,
        spi_proto=spi_proto,
        spi_rw_mask=spi_rw_mask,
        spi_mb_mask=spi_mb_mask,
        spi_addr_mask=spi_addr_mask,
        spi_read_when_set=spi_read_when_set,
        uart_proto=uart_proto,
        uart_packet_len=uart_packet_len,
        uart_delimiter=uart_delim,
        gpio_protocol_hint=gpio_hint,
        gpio_pin_number=gpio_pin,
        gpio_trig_pin_number=gpio_trig_pin,
        gpio_port_index=gpio_port,
        gpio_trig_port_index=gpio_trig_port,
        gpio_idle_level=gpio_idle,
        gpio_tick_us=gpio_tick,
        memory_size_bytes=mem_size,
        memory_page_bytes=mem_page,
        i2c_address_size_bytes=i2c_addr_size,
        display_width=disp_w,
        display_height=disp_h,
    )


# Build probe stimulus (factory)

def _to_float(v: Any) -> Optional[float]:
    if v is None:
        return None
    if isinstance(v, bool):
        return None
    if isinstance(v, (int, float)):
        return float(v)
    if isinstance(v, str):
        try:
            return float(v)
        except ValueError:
            return None
    return None


def build_probe_stimulus(
    stim: Mapping[str, Any],
    *,
    expected_transactions: Sequence[Mapping[str, Any]] = (),
) -> ProbeStimulus:
    """Normalise one test_plan.test_stimuli entry into a ProbeStimulus."""
    if not isinstance(stim, Mapping):
        raise ValueError(
            f"test_stimuli entry must be a mapping; got {type(stim).__name__}"
        )
    name = stim.get("name")
    if not isinstance(name, str) or not name.strip():
        raise ValueError("test_stimuli entry missing non-empty 'name'")
    preload_raw = stim.get("mock_preload")
    # Empty preload is valid when no setup data is required.
    dropped: List[Tuple[str, str]] = []
    preload = _normalise_mock_preload(preload_raw, dropped_out=dropped)

    expected_channels_raw = stim.get("expected_channels")
    exp_channels = expected_channels_raw if isinstance(expected_channels_raw, Mapping) else None
    channel_preload_raw = stim.get("channel_preload_bytes")
    channel_preload = channel_preload_raw if isinstance(channel_preload_raw, Mapping) else None
    expected_time_raw = stim.get("expected_time")
    exp_time = expected_time_raw if isinstance(expected_time_raw, Mapping) else None

    derivation_raw = stim.get("derivation")
    derivation = derivation_raw.strip() if isinstance(derivation_raw, str) else ""

    timeout_raw = stim.get("timeout_s")
    timeout_s = int(timeout_raw) if isinstance(timeout_raw, int) and not isinstance(
        timeout_raw, bool
    ) and timeout_raw > 0 else 0

    return ProbeStimulus(
        name=name.strip(),
        mock_preload=preload,
        expected_read_raw=_to_float(stim.get("expected_read_raw")),
        raw_tolerance=_to_float(stim.get("raw_tolerance")),
        expected_channels=exp_channels,
        channel_preload_bytes=channel_preload,
        expected_mem_bytes=stim.get("expected_mem_bytes")
            if isinstance(stim.get("expected_mem_bytes"), str) else None,
        expected_time=exp_time,
        expected_frame_err=int(stim["expected_frame_err"])
            if isinstance(stim.get("expected_frame_err"), int)
            and not isinstance(stim.get("expected_frame_err"), bool) else None,
        expected_err=int(stim["expected_err"])
            if isinstance(stim.get("expected_err"), int)
            and not isinstance(stim.get("expected_err"), bool) else None,
        expected_transactions=tuple(
            tx for tx in expected_transactions if isinstance(tx, Mapping)
        ),
        derivation=derivation,
        timeout_s=timeout_s,
        dropped_preload_keys=tuple(dropped),
    )


def build_probe_stimuli(test_plan: Mapping[str, Any]) -> List[ProbeStimulus]:
    """Convenience: parse every entry in ``test_plan.test_stimuli``."""
    if not isinstance(test_plan, Mapping):
        raise ValueError(
            f"test_plan must be a mapping; got {type(test_plan).__name__}"
        )
    raw = test_plan.get("test_stimuli")
    if not isinstance(raw, list):
        raise ValueError("test_plan.test_stimuli must be a list")
    txs_raw = test_plan.get("expected_transactions") or []
    expected_transactions = (
        tuple(tx for tx in txs_raw if isinstance(tx, Mapping))
        if isinstance(txs_raw, Sequence) and not isinstance(txs_raw, (str, bytes))
        else tuple()
    )
    return [
        build_probe_stimulus(s, expected_transactions=expected_transactions)
        for s in raw
    ]


# Probe expectation validation

def _coerce_number(v: Any) -> Optional[float]:
    if v is None or isinstance(v, bool):
        return None
    if isinstance(v, (int, float)):
        return float(v)
    if isinstance(v, str):
        try:
            return float(v.strip())
        except ValueError:
            return None
    return None


def _normalise_hex_string(s: str) -> str:
    return re.sub(r"[^0-9a-fA-F]", "", s or "").upper()


def _mem_bytes_hex(values: Sequence[int]) -> str:
    return "".join(f"{int(b) & 0xFF:02X}" for b in values)


def _coerce_expected_byte(value: Any) -> Optional[int]:
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value & 0xFF
    text = str(value).strip()
    if not text:
        return None
    try:
        return int(text, 0) & 0xFF
    except ValueError:
        return None


def _coerce_expected_addr(value: Any) -> Optional[int]:
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value & 0x7F
    text = str(value).strip()
    if not text:
        return None
    try:
        return int(text, 0) & 0x7F
    except ValueError:
        return None


def _expected_prefix_options(raw: Any) -> Tuple[Tuple[int, ...], ...]:
    if not isinstance(raw, Sequence) or isinstance(raw, (str, bytes)):
        return tuple()
    out: List[Tuple[int, ...]] = []
    for option in raw:
        if isinstance(option, Sequence) and not isinstance(option, (str, bytes)):
            bs = tuple(
                b for b in (_coerce_expected_byte(item) for item in option)
                if b is not None
            )
            if bs:
                out.append(bs)
        else:
            b = _coerce_expected_byte(option)
            if b is not None:
                out.append((b,))
    return tuple(out)


def _load_i2c_trace(trace_path: Optional[Path]) -> Tuple[Mapping[str, Any], ...]:
    if trace_path is None:
        return tuple()
    path = Path(trace_path)
    if not path.exists():
        return tuple()
    rows: List[Mapping[str, Any]] = []
    try:
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            obj = json.loads(line)
            if isinstance(obj, Mapping):
                rows.append(obj)
    except (OSError, ValueError):
        return tuple()
    return tuple(rows)


def _trace_has_prefix(
    trace_rows: Sequence[Mapping[str, Any]],
    *,
    addr: Optional[int],
    prefix: Sequence[int],
    allow_leading_i2c_control: bool = False,
) -> bool:
    for row in trace_rows:
        if addr is not None:
            row_addr = _coerce_expected_addr(row.get("addr"))
            if row_addr != addr:
                continue
        tx_raw = row.get("tx_bytes") or []
        if not isinstance(tx_raw, Sequence) or isinstance(tx_raw, (str, bytes)):
            continue
        tx = tuple(
            b for b in (_coerce_expected_byte(item) for item in tx_raw)
            if b is not None
        )
        wanted = tuple(prefix)
        if len(tx) >= len(wanted) and tuple(tx[:len(wanted)]) == wanted:
            return True
        if (
            allow_leading_i2c_control
            and tx
            and tx[0] in (0x00, 0x40)
            and len(tx) - 1 >= len(wanted)
            and tuple(tx[1:1 + len(wanted)]) == wanted
        ):
            return True
    return False


def _trace_prefix_addrs(
    trace_rows: Sequence[Mapping[str, Any]],
    *,
    prefix: Sequence[int],
    allow_leading_i2c_control: bool = False,
) -> Tuple[int, ...]:
    addrs: List[int] = []
    for row in trace_rows:
        row_addr = _coerce_expected_addr(row.get("addr"))
        if row_addr is None:
            continue
        tx_raw = row.get("tx_bytes") or []
        if not isinstance(tx_raw, Sequence) or isinstance(tx_raw, (str, bytes)):
            continue
        tx = tuple(
            b for b in (_coerce_expected_byte(item) for item in tx_raw)
            if b is not None
        )
        wanted = tuple(prefix)
        matched = len(tx) >= len(wanted) and tuple(tx[:len(wanted)]) == wanted
        if (
            not matched
            and allow_leading_i2c_control
            and tx
            and tx[0] in (0x00, 0x40)
            and len(tx) - 1 >= len(wanted)
            and tuple(tx[1:1 + len(wanted)]) == wanted
        ):
            matched = True
        if matched and row_addr not in addrs:
            addrs.append(row_addr)
    return tuple(addrs)


def _trace_i2c_addrs(
    trace_rows: Sequence[Mapping[str, Any]],
    *,
    reads_only: bool = False,
) -> Tuple[int, ...]:
    addrs: List[int] = []
    for row in trace_rows:
        if reads_only and not row.get("is_read"):
            continue
        row_addr = _coerce_expected_addr(row.get("addr"))
        if row_addr is not None and row_addr not in addrs:
            addrs.append(row_addr)
    return tuple(addrs)


def _format_i2c_addrs(addrs: Sequence[int]) -> str:
    if not addrs:
        return "[]"
    return "[" + ",".join(f"0x{addr:02X}" for addr in addrs) + "]"


def _trace_has_read(
    trace_rows: Sequence[Mapping[str, Any]],
    *,
    addr: Optional[int],
) -> bool:
    for row in trace_rows:
        if not row.get("is_read"):
            continue
        if addr is None:
            return True
        row_addr = _coerce_expected_addr(row.get("addr"))
        if row_addr == addr:
            return True
    return False


def _check_expected_transactions_against_trace(
    outcome: ProbeOutcome,
    expected_transactions: Sequence[Mapping[str, Any]],
) -> Tuple[str, ...]:
    if not expected_transactions:
        return tuple()
    if outcome.bus_kind not in ("i2c", "smbus"):
        return tuple()
    trace_rows = _load_i2c_trace(outcome.trace_path)
    if not trace_rows:
        return ("expected_transactions declared but no I2C trace was captured",)

    failures: List[str] = []
    for index, tx in enumerate(expected_transactions):
        addr = _coerce_expected_addr(tx.get("addr_or_pin"))
        options = _expected_prefix_options(tx.get("write_prefix_any_of"))
        if options:
            allow_control = str(outcome.slave_kind or "") == "i2c_display_slave"
            if any(
                _trace_has_prefix(trace_rows, addr=addr, prefix=option)
                or _trace_has_prefix(
                    trace_rows,
                    addr=addr,
                    prefix=option,
                    allow_leading_i2c_control=allow_control,
                )
                for option in options
            ):
                continue
            rendered = [
                "[" + ",".join(f"0x{b:02X}" for b in option) + "]"
                for option in options
            ]
            wrong_addr_matches = []
            for option in options:
                wrong_addr_matches.extend(
                    candidate
                    for candidate in _trace_prefix_addrs(
                        trace_rows,
                        prefix=option,
                        allow_leading_i2c_control=allow_control,
                    )
                    if addr is None or candidate != addr
                )
            wrong_addr_matches = list(dict.fromkeys(wrong_addr_matches))
            addr_hint = ""
            if wrong_addr_matches and addr is not None:
                addr_hint = (
                    "; matching prefix was observed only at I2C addr(s)="
                    f"{_format_i2c_addrs(wrong_addr_matches)}, expected "
                    f"0x{addr:02X}. Configure/select the bus slave address "
                    "before issuing the transfer."
                )
            else:
                observed_addrs = _trace_i2c_addrs(trace_rows)
                if observed_addrs:
                    addr_hint = (
                        "; observed I2C addr(s)="
                        f"{_format_i2c_addrs(observed_addrs)}"
                    )
            failures.append(
                "expected_transactions[%d] phase=%s addr=%s missing write "
                "prefix any_of=%s in actual I2C trace%s"
                % (
                    index,
                    tx.get("phase", "?"),
                    tx.get("addr_or_pin", "?"),
                    rendered,
                    addr_hint,
                )
            )
            continue
        if tx.get("read_any") and not _trace_has_read(trace_rows, addr=addr):
            read_addrs = _trace_i2c_addrs(trace_rows, reads_only=True)
            all_addrs = _trace_i2c_addrs(trace_rows)
            addr_hint = ""
            if read_addrs:
                addr_hint = (
                    "; reads were observed only at I2C addr(s)="
                    f"{_format_i2c_addrs(read_addrs)}"
                )
            elif all_addrs:
                addr_hint = (
                    "; trace contains no reads; observed I2C addr(s)="
                    f"{_format_i2c_addrs(all_addrs)}"
                )
            failures.append(
                "expected_transactions[%d] phase=%s addr=%s requires an I2C "
                "read but trace has none%s"
                % (index, tx.get("phase", "?"), tx.get("addr_or_pin", "?"), addr_hint)
            )
    return tuple(failures)


def _channel_preload_hint(stim: ProbeStimulus, channel: Any) -> str:
    preload = stim.channel_preload_bytes or {}
    if not isinstance(preload, Mapping):
        return ""
    raw = preload.get(channel)
    if raw is None:
        raw = preload.get(str(channel))
    if not isinstance(raw, Sequence) or isinstance(raw, (str, bytes)):
        return ""
    rendered = []
    for item in raw:
        try:
            byte = _coerce_byte(item)
        except ValueError:
            continue
        rendered.append(f"0x{byte:02X}")
    return f" source_bytes=[{','.join(rendered)}]" if rendered else ""


def _first_i2c_read_rx_hint(outcome: ProbeOutcome) -> str:
    if outcome.bus_kind not in ("i2c", "smbus"):
        return ""
    for row in _load_i2c_trace(outcome.trace_path):
        if not row.get("is_read"):
            continue
        rx_raw = row.get("rx_bytes") or []
        if not isinstance(rx_raw, Sequence) or isinstance(rx_raw, (str, bytes)):
            continue
        rendered = []
        for item in rx_raw[:12]:
            byte = _coerce_expected_byte(item)
            if byte is not None:
                rendered.append(f"0x{byte:02X}")
        if rendered:
            suffix = ",..." if len(rx_raw) > len(rendered) else ""
            return f" first_i2c_read_rx=[{','.join(rendered)}{suffix}]"
    return ""


def _stimulus_semantic_hint(stim: Optional[ProbeStimulus]) -> str:
    if stim is None:
        return ""
    derivation = str(stim.derivation or "").lower()
    if "signed two's-complement sanity" in derivation:
        return (
            "; stimulus derivation says signed two's-complement sanity applies: "
            "decode the encoded source field as a signed raw value before "
            "applying the positive-path scale"
        )
    return ""


def check_probe_expectations(
    outcome: ProbeOutcome,
    stim: Optional[ProbeStimulus],
) -> ProbeExpectationCheck:
    """Check observed Renode readback against stimulus ``expected_*`` fields."""
    if stim is None:
        return ProbeExpectationCheck(ok=True)

    failures: List[str] = []
    has_declared_expectation = any((
        stim.expected_read_raw is not None,
        bool(stim.expected_channels),
        bool(stim.expected_mem_bytes),
        bool(stim.expected_time),
        stim.expected_frame_err is not None,
        stim.expected_err is not None,
    ))
    if not has_declared_expectation:
        failures.append(
            "no expected_* field declared; runtime probe would only check "
            "that the driver returned success"
        )
    semantic_hint = _stimulus_semantic_hint(stim)

    tol = _coerce_number(stim.raw_tolerance)
    if tol is None:
        tol = 0.0

    if stim.expected_read_raw is not None:
        exp = _coerce_number(stim.expected_read_raw)
        got = _coerce_number(outcome.read_raw)
        if exp is None:
            failures.append(
                f"expected_read_raw is not numeric: {stim.expected_read_raw!r}"
            )
        elif got is None:
            failures.append(
                f"expected_read_raw={exp:g} but read_raw was not emitted"
            )
        elif abs(got - exp) > tol:
            failures.append(
                f"read_raw expected={exp:g} got={got:g} tol={tol:g}"
                f"{semantic_hint}"
            )

    if stim.expected_channels:
        rx_hint = _first_i2c_read_rx_hint(outcome)
        for ch, expected in stim.expected_channels.items():
            exp = _coerce_number(expected)
            got = _coerce_number((outcome.read_channels or {}).get(ch))
            if exp is None:
                failures.append(
                    f"expected_channels[{ch!r}] is not numeric: {expected!r}"
                )
            elif got is None:
                failures.append(
                    f"channel {ch} expected={exp:g} but was not emitted"
                    f"{_channel_preload_hint(stim, ch)}"
                    f"{rx_hint}"
                )
            elif abs(got - exp) > tol:
                failures.append(
                    f"channel {ch} expected={exp:g} got={got:g} tol={tol:g}"
                    f"{_channel_preload_hint(stim, ch)}"
                    f"{rx_hint}"
                    f"{semantic_hint}"
                )

    if stim.expected_mem_bytes:
        exp_hex = _normalise_hex_string(str(stim.expected_mem_bytes))
        got_hex = _mem_bytes_hex(outcome.mem_bytes or ())
        if not exp_hex:
            failures.append("expected_mem_bytes is empty after normalisation")
        elif got_hex.upper() != exp_hex:
            failures.append(
                f"memory bytes expected={exp_hex} got={got_hex or '<empty>'}"
            )

    if stim.expected_time:
        for field, expected in stim.expected_time.items():
            exp = _coerce_number(expected)
            got = _coerce_number((outcome.rtc_time or {}).get(field))
            if exp is None:
                failures.append(
                    f"expected_time[{field!r}] is not numeric: {expected!r}"
                )
            elif got is None:
                failures.append(
                    f"RTC field {field} expected={exp:g} but was not emitted"
                )
            elif int(got) != int(exp):
                failures.append(
                    f"RTC field {field} expected={int(exp)} got={int(got)}"
                )

    if stim.expected_frame_err is not None:
        exp = _coerce_number(stim.expected_frame_err)
        got = _coerce_number(outcome.display_frame_err)
        if exp is None:
            failures.append(
                f"expected_frame_err is not numeric: {stim.expected_frame_err!r}"
            )
        elif got is None:
            failures.append(
                f"display frame_err expected={int(exp)} but was not emitted"
            )
        elif int(got) != int(exp):
            failures.append(
                f"display frame_err expected={int(exp)} got={int(got)}"
            )

    if stim.expected_err is not None:
        exp = _coerce_number(stim.expected_err)
        got = _coerce_number(outcome.read_err)
        observed_error = bool(outcome.result_err) or (
            got is not None and int(got) != 0
        )
        if exp is None:
            failures.append(
                f"expected_err is not numeric: {stim.expected_err!r}"
            )
        elif int(exp) == 0:
            if observed_error:
                detail = f" read_err={int(got)}" if got is not None else ""
                failures.append(f"expected success but driver returned error{detail}")
        elif not observed_error:
            failures.append(
                f"expected driver error expected_err={int(exp)} but runtime "
                "reported success/no read_err"
            )

    failures.extend(
        _check_expected_transactions_against_trace(
            outcome,
            stim.expected_transactions,
        )
    )

    return ProbeExpectationCheck(ok=not failures, failures=tuple(failures))


# Runner dispatch

# Runtime path to per-runner dispatch.
_DISPATCH_TABLE = {
    RUNTIME_I2C:          ("i2c",  run_i2c_vector),
    RUNTIME_SPI:          ("spi",  run_spi_vector),
    RUNTIME_UART:         ("uart", run_uart_vector),
    RUNTIME_GPIO_PULSE:   ("gpio", run_gpio_vector),
}


_UART_RESPONSE_PRELOAD_KEYS: Tuple[str, ...] = (
    "read_bytes",
    "response",
    "resp",
    "payload",
    "stream",
)


def _coerce_flat_bytes(value: Any) -> Optional[Tuple[int, ...]]:
    """Best-effort coercion for runner bridge byte arrays."""
    if value is None or isinstance(value, bool):
        return None
    if isinstance(value, int):
        return (_coerce_byte(value),)
    if isinstance(value, (list, tuple)):
        if any(isinstance(x, (list, tuple, Mapping)) for x in value):
            return None
        try:
            return tuple(_coerce_byte(x) for x in value)
        except ValueError:
            return None
    if isinstance(value, str):
        parsed = _parse_json_list_of_ints(value)
        if parsed is not None:
            return tuple(parsed)
        tokens = re.findall(r"0[xX][0-9a-fA-F]{1,2}|[0-9a-fA-F]{2}", value)
        residue = re.sub(
            r"0[xX][0-9a-fA-F]{1,2}|[0-9a-fA-F]{2}",
            "",
            value,
        )
        if tokens and re.fullmatch(r"[\s,;:_\-\[\]]*", residue or ""):
            out: List[int] = []
            for token in tokens:
                base = 0 if token.lower().startswith("0x") else 16
                out.append(int(token, base) & 0xFF)
            return tuple(out)
        try:
            return tuple(ord(ch) & 0xFF for ch in value)
        except TypeError:
            return None
    return None


def _uart_request_options(raw: Any) -> Tuple[Tuple[int, ...], ...]:
    if not isinstance(raw, Sequence) or isinstance(raw, (str, bytes)):
        return tuple()
    if not raw:
        return tuple()
    if all(not isinstance(item, (list, tuple)) for item in raw):
        frame = _coerce_flat_bytes(raw)
        return (frame,) if frame else tuple()
    return _expected_prefix_options(raw)


def _uart_expected_request_frames(
    stim: ProbeStimulus,
    meta: ProbeMeta,
) -> Tuple[Tuple[int, ...], ...]:
    frames: List[Tuple[int, ...]] = []
    seen: set = set()
    option_keys = (
        "write_prefix_any_of",
        "request_prefix_any_of",
        "request_bytes_any_of",
        "tx_prefix_any_of",
    )
    scalar_keys = (
        "write_prefix",
        "request_prefix",
        "write_bytes",
        "request_bytes",
        "tx_bytes",
    )
    for tx in stim.expected_transactions:
        if not isinstance(tx, Mapping):
            continue
        candidates: List[Tuple[int, ...]] = []
        for key in option_keys:
            candidates.extend(_uart_request_options(tx.get(key)))
        for key in scalar_keys:
            frame = _coerce_flat_bytes(tx.get(key))
            if frame:
                candidates.append(frame)
        for frame in candidates:
            if not frame:
                continue
            if meta.uart_proto == "fixed" and meta.uart_packet_len > 0:
                if len(frame) != int(meta.uart_packet_len):
                    continue
            if frame in seen:
                continue
            seen.add(frame)
            frames.append(frame)
    return tuple(frames)


def _uart_preload_response(preload: Mapping[str, Any]) -> Optional[Tuple[int, ...]]:
    for key in _UART_RESPONSE_PRELOAD_KEYS:
        if key not in preload:
            continue
        response = _coerce_flat_bytes(preload.get(key))
        if response:
            return response
    return None


def _uart_req_key(frame: Sequence[int]) -> str:
    return "req_" + "".join(f"{int(b) & 0xFF:02X}" for b in frame)


def _prepare_uart_probe_stimulus(
    stim: ProbeStimulus,
    meta: ProbeMeta,
) -> ProbeStimulus:
    """Bridge UART plan data to the runner preload shape."""
    if meta.bus_kind != "uart" or not isinstance(stim.mock_preload, Mapping):
        return stim
    response = _uart_preload_response(stim.mock_preload)
    if not response:
        return stim
    request_frames = _uart_expected_request_frames(stim, meta)
    if not request_frames:
        return stim

    preload = dict(stim.mock_preload)
    changed = False
    for frame in request_frames:
        key = _uart_req_key(frame)
        if key not in preload:
            preload[key] = list(response)
            changed = True
    if not changed:
        return stim
    return dataclasses.replace(stim, mock_preload=preload)


def _outcome_from_runner(
    stim_name: str,
    run_outcome: Any,
    routing: RoutingResult,
) -> ProbeOutcome:
    """Convert a per-runner ``*VectorOutcome`` into a unified ProbeOutcome."""
    # Missing runner fields fall back to ProbeOutcome defaults.
    def g(name: str, default: Any = None) -> Any:
        return getattr(run_outcome, name, default)

    return ProbeOutcome(
        stimulus_name=stim_name,
        boot_detected=bool(g("boot_detected", False)),
        test_done=bool(g("test_done", False)),
        result_pass=bool(g("result_pass", False)),
        result_err=bool(g("result_err", False)),
        read_err=g("read_err"),
        read_raw=g("read_raw"),
        read_channels=dict(g("read_channels", {}) or {}),
        mem_bytes=tuple(g("mem_bytes", []) or []),
        mem_probe_addr=g("mem_probe_addr"),
        mem_probe_len=g("mem_probe_len"),
        memory_size_bytes=g("memory_size_bytes"),
        memory_page_bytes=g("memory_page_bytes"),
        display_frame_len=g("display_frame_len"),
        display_frame_err=g("display_frame_err"),
        display_status_err=g("display_status_err"),
        display_status=g("display_status"),
        rtc_get_err=g("rtc_get_err"),
        rtc_set_err=g("rtc_set_err"),
        rtc_time=dict(g("rtc_time", {}) or {}),
        trace_path=g("trace_path"),
        output_lines=tuple(g("output_lines", []) or []),
        error=str(g("error", "") or ""),
        duration_s=float(g("duration_s", 0.0) or 0.0),
        renode_exit=g("renode_exit"),
        runtime_path=routing.runtime_path,
        slave_kind=routing.slave_kind,
        spi_sub_mode=routing.spi_sub_mode,
        bus_kind=routing.bus_kind,
        # Echo stimulus expected_* for self-contained debugging
        expected_read_raw=None,
        expected_channels={},
        expected_mem_bytes=None,
        expected_time=None,
        expected_err=None,
    )


def probe_stimulus(
    elf_path: Path,
    meta: ProbeMeta,
    stim: ProbeStimulus,
    work_dir: Path,
    *,
    routing: RoutingResult,
    timeout: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
) -> ProbeOutcome:
    """Run one stimulus vector via the appropriate evaluation runner."""
    if routing.runtime_path == RUNTIME_GPIO_DISCRETE:
        raise ProbeError(
            f"runtime_path={routing.runtime_path!r} is reserved but not "
            "yet implemented; routing uses it only as a marker for "
            "future discrete-GPIO devices"
        )
    dispatch = _DISPATCH_TABLE.get(routing.runtime_path)
    if dispatch is None:
        raise ProbeError(
            f"unknown runtime_path: {routing.runtime_path!r}. "
            f"Supported: {sorted(_DISPATCH_TABLE.keys())}"
    )
    expected_bus, runner = dispatch
    # Ensure routing and runner selection agree before dispatch.
    if routing.bus_kind != expected_bus and not (
        routing.bus_kind == "smbus" and expected_bus == "i2c"
    ):
        raise ProbeError(
            f"runtime_path={routing.runtime_path!r} expects "
            f"bus_kind={expected_bus!r}, got {routing.bus_kind!r}"
        )
    if meta.bus_kind != expected_bus:
        raise ProbeError(
            f"runtime_path={routing.runtime_path!r} expects "
            f"ProbeMeta.bus_kind={expected_bus!r}, got {meta.bus_kind!r}"
        )
    if not elf_path.exists():
        raise ProbeError(f"ELF not found: {elf_path}")

    work_dir = Path(work_dir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    # Individual stimuli may override the probe timeout.
    effective_timeout = stim.timeout_s if stim.timeout_s > 0 else timeout

    runner_stim = (
        _prepare_uart_probe_stimulus(stim, meta)
        if expected_bus == "uart" else stim
    )

    run_outcome = runner(
        elf_path,
        meta,
        runner_stim,
        work_dir,
        timeout=effective_timeout,
        sleep_s=sleep_s,
        base_repl_path=base_repl_path,
    )
    return _outcome_from_runner(stim.name, run_outcome, routing)


def probe_all_stimuli(
    elf_path: Path,
    meta: ProbeMeta,
    stimuli: Sequence[ProbeStimulus],
    work_dir: Path,
    *,
    routing: RoutingResult,
    timeout_per_stim: int = 60,
    sleep_s: int = 20,
    base_repl_path: Optional[Path] = None,
    stop_on_error: bool = False,
) -> List[ProbeOutcome]:
    """Probe every stimulus in order."""
    outcomes: List[ProbeOutcome] = []
    for stim in stimuli:
        outcome = probe_stimulus(
            elf_path,
            meta,
            stim,
            work_dir,
            routing=routing,
            timeout=timeout_per_stim,
            sleep_s=sleep_s,
            base_repl_path=base_repl_path,
        )
        outcomes.append(outcome)
        if stop_on_error and (outcome.error or not outcome.test_done):
            break
    return outcomes


__all__ = [
    "ProbeError",
    "ProbeMeta",
    "ProbeStimulus",
    "ProbeOutcome",
    "ProbeExpectationCheck",
    "build_probe_meta",
    "build_probe_stimulus",
    "build_probe_stimuli",
    "check_probe_expectations",
    "probe_stimulus",
    "probe_all_stimuli",
]
