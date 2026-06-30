"""evaluation.oracle.schema - typed dataclasses for human-curated oracle data."""
from __future__ import annotations

import dataclasses
import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional


# Same set as drivergen_eval_adapter.h's DRIVERGEN_EVAL_CLASS_*.
KNOWN_EVAL_CLASSES = {
    "single_channel", "multi_channel", "memory", "display", "rtc",
}

# Same set as connection_binding.bus_type accepted by route().
KNOWN_BUS_TYPES = {"i2c", "smbus", "spi", "uart", "gpio"}
_I2C_ACCESS_MODELS = {"register", "command", "port"}
_SPI_PROTOS = {"register", "stream", "command", "memory"}
_UART_FRAMINGS = {"fixed", "delimiter"}


# ---------- helpers ----------

def _parse_int(v: Any) -> int:
    """Accept either plain int or "0x.." / decimal string forms."""
    if isinstance(v, bool):  # bool is a subclass of int, exclude
        raise TypeError(f"expected int, got bool: {v!r}")
    if isinstance(v, int):
        return v
    if isinstance(v, str):
        return int(v, 0)
    raise TypeError(f"expected int or numeric string, got {type(v).__name__}: {v!r}")


def _parse_bytes(v: Any) -> List[int]:
    """Convert ['0x01', 0x10, 32] → [1, 16, 32], asserting each byte is 0..255."""
    if not isinstance(v, list):
        raise TypeError(f"expected list of byte values, got {type(v).__name__}")
    out: List[int] = []
    for x in v:
        b = _parse_int(x) & 0xFF
        if not 0 <= b <= 0xFF:
            raise ValueError(f"byte out of range: {x!r}")
        out.append(b)
    return out


def _parse_preload_value(v: Any) -> Any:
    """Normalize one mock_preload value."""
    if isinstance(v, str):
        # GPIO payload hex-string form; renderer does its own parsing.
        return v
    if not isinstance(v, list):
        raise TypeError(
            f"expected list or hex string, got {type(v).__name__}: {v!r}"
        )
    # Empty list is still a valid byte list.
    if not v:
        return []
    # Schedule form: every entry is itself a list.
    if all(isinstance(x, list) for x in v):
        schedule: List[List[int]] = []
        for i, pair in enumerate(v):
            if len(pair) < 2:
                raise ValueError(
                    f"schedule[{i}] must have [level, duration_us], got {pair!r}"
                )
            level    = _parse_int(pair[0])
            duration = _parse_int(pair[1])
            schedule.append([level, duration])
        return schedule
    # Otherwise treat as flat bytes (may raise for out-of-range values).
    return _parse_bytes(v)


def _require_keys(d: Dict[str, Any], keys: List[str], where: str) -> None:
    missing = [k for k in keys if k not in d]
    if missing:
        raise ValueError(f"{where}: missing required keys: {missing}")


# ---------- meta.json ----------

@dataclass(frozen=True)
class ChannelDescriptor:
    """One entry in OracleMeta.channels (multi_channel devices)."""
    id: str
    physical_unit: str
    scale: int = 0    # 0 = identity / not applicable

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "ChannelDescriptor":
        _require_keys(d, ["id", "physical_unit"], "channel")
        return cls(
            id=str(d["id"]),
            physical_unit=str(d["physical_unit"]),
            scale=int(d.get("scale", 0)),
        )


@dataclass(frozen=True)
class PrimaryDescriptor:
    """OracleMeta.primary — used by single_channel; also as channels[0] hint."""
    id: str
    physical_unit: str
    scale: int = 0

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "PrimaryDescriptor":
        _require_keys(d, ["id", "physical_unit"], "primary")
        return cls(
            id=str(d["id"]),
            physical_unit=str(d["physical_unit"]),
            scale=int(d.get("scale", 0)),
        )


@dataclass(frozen=True)
class OracleMeta:
    """Contents of meta.json — device-level evaluation metadata."""
    device_id: str
    eval_class: str          # one of KNOWN_EVAL_CLASSES
    bus_type: str            # one of KNOWN_BUS_TYPES
    description: str = ""

    # Bus-specific hints
    i2c_address_7bit: Optional[int] = None     # required when bus_type in {i2c, smbus}
    i2c_access_model: str = "register"         # register pointer, command read, or port I/O
    spi_proto: Optional[str] = None            # SPI sub-mode hint (register/stream/command)
    # SPI `register`-proto wiring (meaningful only when spi_proto == "register").
    # Defaults mirror ADXL345 (bit7=R/W, bit6=MB, bits5..0=addr, read_when_set).
    # BMP280-style (bit7 clear = read) sets read_when_set=False. Chips without
    # a multi-byte bit leave spi_mb_mask=0.
    spi_rw_mask: int = 0x80                    # spi (register proto)
    spi_mb_mask: int = 0                       # spi (register proto); 0 = no MB bit
    spi_addr_mask: int = 0x7F                  # spi (register proto)
    spi_read_when_set: bool = True             # spi (register proto)
    # UART framing: "fixed" (packet_len) or "delimiter" (delimiter bytes).
    uart_proto: Optional[str] = None           # UART framing hint
    uart_packet_len: int = 0                   # uart framing='fixed'
    uart_delimiter: List[int] = field(default_factory=list)  # uart framing='delimiter'
    gpio_protocol_hint: Optional[str] = None   # 1-wire / pulse-width / discrete
    gpio_pin_number: int = 5                   # GPIO pin index within bank (echo/data)
    gpio_trig_pin_number: int = -1             # trigger pin index; -1 = same as pin_number
    gpio_port_index: int = 1                   # GPIO bank index A=0/B=1/C=2 for echo/data
    gpio_trig_port_index: int = -1             # trigger bank index; -1 = same as echo bank
    gpio_idle_level: int = 1                   # pull-up (1) or pull-down (0)
    gpio_tick_us: int = 1                      # microseconds per IDR read

    # Class-specific descriptors
    primary: Optional[PrimaryDescriptor] = None     # single_channel
    channels: List[ChannelDescriptor] = field(default_factory=list)  # multi_channel
    memory_size_bytes: int = 0                      # memory
    memory_page_bytes: int = 0                      # memory
    jedec_id: List[int] = field(default_factory=list)  # SPI memory (W25Q64JV etc.)
    erase_unit: int = 0                             # SPI memory erase size in bytes
    # Bus-level address width for memory devices (only meaningful for
    # bus_type in {i2c, smbus}). When omitted it's derived from size:
    # 1 byte for <=256B, 2 bytes otherwise. Useful for atypical parts.
    i2c_address_size_bytes: int = 0                 # memory (optional override)
    # Display pixel geometry (only meaningful for eval_class='display').
    # Defaults to 128x64 if unset (most common OLED / mono-LCD).
    display_width: int = 0                          # display
    display_height: int = 0                         # display

    # Per-fault L5 applicability flags. False marks an undetectable fault as
    # not applicable for the matching judge.
    fault_detect: Dict[str, bool] = field(default_factory=dict)

    # True requires a byte-exact L3 trace; otherwise semantic comparison
    # accepts traces that cover the required golden transactions.
    require_byte_exact: bool = False

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "OracleMeta":
        _require_keys(d, ["device_id", "eval_class", "bus_type"], "meta")
        ev = d["eval_class"]
        if ev not in KNOWN_EVAL_CLASSES:
            raise ValueError(f"meta.eval_class={ev!r} not in {sorted(KNOWN_EVAL_CLASSES)}")
        bus = d["bus_type"]
        if bus not in KNOWN_BUS_TYPES:
            raise ValueError(f"meta.bus_type={bus!r} not in {sorted(KNOWN_BUS_TYPES)}")

        addr_raw = d.get("i2c_address_7bit")
        addr_int = _parse_int(addr_raw) if addr_raw is not None else None

        primary = None
        if d.get("primary") is not None:
            primary = PrimaryDescriptor.from_json(d["primary"])
        channels = [ChannelDescriptor.from_json(c) for c in d.get("channels", [])]

        meta = cls(
            device_id          = str(d["device_id"]),
            eval_class         = ev,
            bus_type           = bus,
            description        = str(d.get("description", "")),
            i2c_address_7bit   = addr_int,
            i2c_access_model   = str(d.get("i2c_access_model", "register")),
            spi_proto          = d.get("spi_proto"),
            spi_rw_mask        = _parse_int(d.get("spi_rw_mask", 0x80)),
            spi_mb_mask        = _parse_int(d.get("spi_mb_mask", 0)),
            spi_addr_mask      = _parse_int(d.get("spi_addr_mask", 0x7F)),
            spi_read_when_set  = bool(d.get("spi_read_when_set", True)),
            uart_proto         = d.get("uart_proto"),
            uart_packet_len    = int(d.get("uart_packet_len", 0)),
            uart_delimiter     = _parse_bytes(d["uart_delimiter"]) if d.get("uart_delimiter") else [],
            gpio_protocol_hint = d.get("gpio_protocol_hint"),
            gpio_pin_number    = int(d.get("gpio_pin_number", 5)),
            gpio_trig_pin_number = int(d.get("gpio_trig_pin_number", -1)),
            gpio_port_index    = int(d.get("gpio_port_index", 1)),
            gpio_trig_port_index = int(d.get("gpio_trig_port_index", -1)),
            gpio_idle_level    = int(d.get("gpio_idle_level", 1)),
            gpio_tick_us       = int(d.get("gpio_tick_us", 1)),
            primary            = primary,
            channels           = channels,
            memory_size_bytes  = int(d.get("memory_size_bytes", 0)),
            memory_page_bytes  = int(d.get("memory_page_bytes", 0)),
            jedec_id           = [int(b) for b in d.get("jedec_id", [])],
            erase_unit         = int(d.get("erase_unit", 0)),
            i2c_address_size_bytes = int(d.get("i2c_address_size_bytes", 0)),
            display_width      = int(d.get("display_width", 0)),
            display_height     = int(d.get("display_height", 0)),
            fault_detect       = {
                str(k): bool(v) for k, v in (d.get("fault_detect") or {}).items()
            },
            require_byte_exact = bool(d.get("require_byte_exact", False)),
        )
        meta._cross_check()
        return meta

    def _cross_check(self) -> None:
        if self.eval_class == "single_channel" and self.primary is None:
            raise ValueError("single_channel meta requires `primary` descriptor")
        if self.eval_class == "multi_channel" and not self.channels:
            raise ValueError("multi_channel meta requires non-empty `channels`")
        if self.eval_class == "memory" and self.memory_size_bytes <= 0:
            raise ValueError("memory meta requires positive memory_size_bytes")
        if self.bus_type in ("i2c", "smbus") and self.i2c_address_7bit is None:
            raise ValueError(
                f"meta.bus_type={self.bus_type!r} requires i2c_address_7bit"
            )
        if (
            self.bus_type in ("i2c", "smbus")
            and self.i2c_access_model not in _I2C_ACCESS_MODELS
        ):
            raise ValueError(
                f"meta.i2c_access_model={self.i2c_access_model!r} "
                f"not in {sorted(_I2C_ACCESS_MODELS)}"
            )
        if self.bus_type == "spi":
            if self.spi_proto is not None and self.spi_proto not in _SPI_PROTOS:
                raise ValueError(
                    f"meta.spi_proto={self.spi_proto!r} not in {sorted(_SPI_PROTOS)}"
                )
            # register proto needs a non-zero address mask to be meaningful
            if self.spi_proto == "register" and self.spi_addr_mask <= 0:
                raise ValueError(
                    "register spi_proto requires positive spi_addr_mask"
                )
        if self.bus_type == "uart":
            if self.uart_proto is not None and self.uart_proto not in _UART_FRAMINGS:
                raise ValueError(
                    f"meta.uart_proto={self.uart_proto!r} not in "
                    f"{sorted(_UART_FRAMINGS)}"
                )
            if self.uart_proto == "fixed" and self.uart_packet_len <= 0:
                raise ValueError(
                    "uart_proto='fixed' requires positive uart_packet_len"
                )
            if self.uart_proto == "delimiter" and not self.uart_delimiter:
                raise ValueError(
                    "uart_proto='delimiter' requires non-empty uart_delimiter"
                )
        if self.bus_type == "gpio":
            if not (0 <= self.gpio_pin_number <= 15):
                raise ValueError(
                    f"gpio_pin_number={self.gpio_pin_number} out of [0, 15]"
                )
            if not (0 <= self.gpio_port_index <= 4):
                raise ValueError(
                    f"gpio_port_index={self.gpio_port_index} out of [0, 4]"
                )
            if self.gpio_trig_port_index >= 0 and not (0 <= self.gpio_trig_port_index <= 4):
                raise ValueError(
                    f"gpio_trig_port_index={self.gpio_trig_port_index} out of [-1, 4]"
                )
            if self.gpio_idle_level not in (0, 1):
                raise ValueError(
                    f"gpio_idle_level={self.gpio_idle_level} must be 0 or 1"
                )
            if self.gpio_tick_us <= 0:
                raise ValueError(
                    f"gpio_tick_us={self.gpio_tick_us} must be positive"
                )


# ---------- stimuli.json ----------

@dataclass(frozen=True)
class ExpectedReading:
    """Per-stimulus expected value(s)."""
    raw: Optional[int] = None
    raw_alt: List[int] = field(default_factory=list)
    tolerance: int = 0
    channels: Dict[str, Dict[str, Any]] = field(default_factory=dict)
    mem_bytes: List[int] = field(default_factory=list)
    time: Optional[Dict[str, int]] = None
    err: Optional[int] = None

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "ExpectedReading":
        return cls(
            raw       = _parse_int(d["raw"]) if "raw" in d and d["raw"] is not None else None,
            raw_alt   = [_parse_int(x) for x in d.get("raw_alt", []) if x is not None],
            tolerance = int(d.get("tolerance", 0)),
            channels  = {str(k): dict(v) for k, v in d.get("channels", {}).items()},
            mem_bytes = _parse_bytes(d["mem_bytes"]) if d.get("mem_bytes") else [],
            time      = dict(d["time"]) if d.get("time") else None,
            err       = int(d["err"]) if "err" in d and d["err"] is not None else None,
        )


@dataclass(frozen=True)
class Stimulus:
    """One test vector."""
    name: str
    mock_preload: Dict[str, Any]          # register key (hex str or int-as-str) → bytes / schedule / hex
    expected: ExpectedReading
    note: str = ""
    timeout_s: int = 0                    # 0 = inherit default

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "Stimulus":
        _require_keys(d, ["name"], "stimulus")
        preload_raw = d.get("mock_preload", {})
        preload: Dict[str, Any] = {}
        for k, v in preload_raw.items():
            preload[str(k)] = _parse_preload_value(v)
        expected = ExpectedReading.from_json(d.get("expected", {}))
        return cls(
            name         = str(d["name"]),
            mock_preload = preload,
            expected     = expected,
            note         = str(d.get("note", "")),
            timeout_s    = int(d.get("timeout_s", 0)),
        )


# ---------- required_writes.json ----------

@dataclass(frozen=True)
class RequiredWrite:
    """One entry in required_writes.json (L3 relaxed policy)."""
    addr: int                   # 7-bit I2C address (or arbitrary id for non-I2C)
    any_of: List[List[int]]     # acceptable byte prefixes
    description: str = ""

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "RequiredWrite":
        _require_keys(d, ["addr", "any_of"], "required_write")
        any_of = [_parse_bytes(p) for p in d["any_of"]]
        if not any_of:
            raise ValueError("required_write.any_of must be non-empty")
        return cls(
            addr        = _parse_int(d["addr"]),
            any_of      = any_of,
            description = str(d.get("description", "")),
        )


# ---------- nack_scenarios.json ----------

@dataclass(frozen=True)
class NackScenario:
    """One entry in nack_scenarios.json (L5 configuration)."""
    name: str
    nack_first_n: int
    description: str = ""

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "NackScenario":
        _require_keys(d, ["name", "nack_first_n"], "nack_scenario")
        return cls(
            name         = str(d["name"]),
            nack_first_n = int(d["nack_first_n"]),
            description  = str(d.get("description", "")),
        )


def default_nack_scenarios() -> List[NackScenario]:
    """Used when nack_scenarios.json is absent: transient + persistent."""
    return [
        NackScenario(name="transient_nack_1", nack_first_n=1,
                     description="single transient NACK at start of test"),
        NackScenario(name="persistent_nack_99", nack_first_n=99,
                     description="persistent NACK on every transaction"),
    ]


# ---------- physical_range.json ----------

@dataclass(frozen=True)
class PhysicalRange:
    """L4 fallback range when a stimulus has no explicit expected.raw."""
    minimum: float
    maximum: float
    raw_value_description: str = ""

    @classmethod
    def from_json(cls, d: Dict[str, Any]) -> "PhysicalRange":
        _require_keys(d, ["min", "max"], "physical_range.default")
        return cls(
            minimum               = float(d["min"]),
            maximum               = float(d["max"]),
            raw_value_description = str(d.get("raw_value_description", "")),
        )


# ---------- aggregate ----------

@dataclass(frozen=True)
class OracleData:
    """All oracle artifacts for one device, loaded eagerly."""
    meta: OracleMeta
    stimuli: List[Stimulus]
    required_writes: List[RequiredWrite] = field(default_factory=list)
    golden_trace: Optional[Dict[str, Any]] = None     # raw JSON (loaded lazily by L3-strict)
    protocol_equivalence: Optional[Dict[str, Any]] = None
    physical_range: Optional[PhysicalRange] = None
    nack_scenarios: List[NackScenario] = field(default_factory=list)


__all__ = [
    "KNOWN_EVAL_CLASSES",
    "KNOWN_BUS_TYPES",
    "ChannelDescriptor",
    "PrimaryDescriptor",
    "OracleMeta",
    "ExpectedReading",
    "Stimulus",
    "RequiredWrite",
    "NackScenario",
    "default_nack_scenarios",
    "PhysicalRange",
    "OracleData",
]
