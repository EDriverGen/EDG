"""classify a device into one of the 5 eval_class categories."""
from __future__ import annotations

import dataclasses
import re
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple


# Public eval_class constants

EVAL_CLASS_SINGLE_CHANNEL = "single_channel"
EVAL_CLASS_MULTI_CHANNEL = "multi_channel"
EVAL_CLASS_MEMORY = "memory"
EVAL_CLASS_DISPLAY = "display"
EVAL_CLASS_RTC = "rtc"

EVAL_CLASSES: Tuple[str, ...] = (
    EVAL_CLASS_SINGLE_CHANNEL,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_RTC,
)

# Buses supported by the evaluation runtime.
SUPPORTED_BUSES: Tuple[str, ...] = ("i2c", "spi", "uart", "gpio", "smbus")


# Keyword sets

# Time-field tokens used for RTC detection.
_RTC_TOKENS: frozenset = frozenset({
    "second", "minute", "hour", "day", "date",
    "month", "year", "weekday", "alarm",
})

# Generic display command and field tokens.
_DISPLAY_TOKENS: frozenset = frozenset({
    "oled", "lcd", "display", "screen", "gddram", "framebuffer",
    "page", "column", "contrast",
})

# Channel-root keywords used to recognize register-name stems.
_CHANNEL_KEYWORDS: frozenset = frozenset({
    "temp", "press", "humidity", "light",
    "accel", "gyro", "mag",
    "co2", "distance",
    "ch",
})

# Keywords considered safe to search in free-form text.
_TEXT_CHANNEL_KEYWORDS: frozenset = frozenset({
    "temperature", "pressure", "humidity", "light",
    "accelerometer", "gyroscope", "magnetometer",
    "co2", "distance",
})

# Canonicalize near-synonyms after a confirmed match.
_CANONICAL_ALIAS: Mapping[str, str] = {
    "temperature": "temp",
    "pressure": "press",
    "accelerometer": "accel",
    "gyroscope": "gyro",
    "magnetometer": "mag",
}

# Layout suffixes stripped before channel-root detection.
_LAYOUT_SUFFIXES: Tuple[str, ...] = (
    "_msb", "_lsb", "_h", "_l", "_hi", "_lo",
    "_high", "_low", "_out",
)

# Axis suffixes kept as part of the channel root.
_AXIS_SUFFIXES: Tuple[str, ...] = (
    "_x", "_y", "_z",
)

# Register names ignored when counting data channels.
_CONTROL_REG_TOKENS: frozenset = frozenset({
    "ctrl", "control", "config", "cfg", "cmd", "command",
    "status", "id", "reset", "reserved", "test",
    "interrupt", "fifo", "power", "sleep",
    "calibration", "mode", "offset", "trim",
    "bank", "enable",
})

# Writable threshold names filtered out of read channels.
_SETPOINT_TOKENS: frozenset = frozenset({
    "hyst", "threshold", "limit_high", "limit_low", "setpoint",
})


# Public dataclass

@dataclasses.dataclass(frozen=True)
class ClassifyResult:
    """Outcome of :func:`classify_device`."""
    eval_class: str
    bus_type: str
    confidence: float
    rule_applied: str
    warnings: Tuple[str, ...]
    channel_count: int
    channel_roots: Tuple[str, ...]


# Small utilities

def _as_list(x: Any) -> List[Any]:
    """Return ``x`` as a list. ``None`` becomes an empty list."""
    if x is None:
        return []
    if isinstance(x, list):
        return x
    return [x]


def _collect_register_names(ir: Mapping[str, Any]) -> List[str]:
    """Extract register / command names from a DeviceIR dict."""
    names: List[str] = []
    for entry in _as_list(ir.get("registers_or_commands")):
        if not isinstance(entry, dict):
            continue
        raw = entry.get("name")
        if not isinstance(raw, str):
            continue
        for part in raw.split("/"):
            part = part.strip()
            if part:
                names.append(part)
    return names


def _normalise_name(name: str) -> str:
    """Lowercase and collapse whitespace / hyphens to underscores."""
    s = name.lower().strip()
    s = re.sub(r"[\s\-]+", "_", s)
    s = re.sub(r"__+", "_", s)
    # Split axis suffixes from compact register names.
    s = re.sub(
        r"(?<![a-z])(data|out|accel|acc|gyro|gyr|mag)([xyz])(?![a-z])",
        r"\1_\2",
        s,
    )
    return s


def _resolve_bus(
    ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]],
    warnings_out: List[str],
) -> Tuple[str, float]:
    """Resolve the authoritative bus_type."""
    def _from(pkg: Optional[Mapping[str, Any]], key: str) -> Optional[str]:
        if pkg is None:
            return None
        val = pkg.get(key)
        if isinstance(val, str) and val.strip():
            return val.strip().lower()
        # Try nested connection_binding/connection bus fields.
        conn = pkg.get("connection_binding") or pkg.get("connection")
        if isinstance(conn, Mapping):
            sub = conn.get("bus_type") or conn.get("connection_type")
            if isinstance(sub, str) and sub.strip():
                return sub.strip().lower()
        # Prefer fixed task context before falling back to the Device IR.
        fixed = pkg.get("fixed_task_context")
        if isinstance(fixed, Mapping):
            dev = fixed.get("device")
            if isinstance(dev, Mapping):
                sub = dev.get("bus_type") or dev.get("required_bus_type")
                if isinstance(sub, str) and sub.strip():
                    return sub.strip().lower()
            conn = fixed.get("connection")
            if isinstance(conn, Mapping):
                sub = conn.get("bus_type") or conn.get("connection_type")
                if isinstance(sub, str) and sub.strip():
                    return sub.strip().lower()
        return None

    bus = _from(task_package, "bus_type")
    if bus is not None:
        return _normalise_bus(bus, warnings_out), 0.40

    bus = _from(ir, "bus_type")
    if bus is not None:
        return _normalise_bus(bus, warnings_out), 0.20

    warnings_out.append("bus_type missing in both device_ir and task_package; fell back to 'i2c'")
    return "i2c", 0.0


def _normalise_bus(bus: str, warnings_out: List[str]) -> str:
    """Normalise free-form bus names to the supported set."""
    b = bus.lower().strip()
    if b in SUPPORTED_BUSES:
        return b
    # Common aliases.
    if b in {
        "serial", "usart", "rs232", "rs485",
        "uart_polling", "uart_interrupt", "uart_dma",
        "serial_polling", "serial_interrupt", "serial_dma",
        "usart_polling", "usart_interrupt", "usart_dma",
    }:
        return "uart"
    if b in {"1-wire", "1wire", "one_wire", "onewire", "gpio_1wire"}:
        return "gpio"
    # Normalize GPIO variants to the bus family.
    if b in {
        "gpio_timing", "gpio_pulse", "gpio_discrete", "gpio_level",
        "gpio_input", "gpio_output", "gpio_inout",
    }:
        return "gpio"
    if b in {"spi-bus", "mspi", "qspi"}:
        return "spi"
    warnings_out.append(f"bus_type {bus!r} not in supported set; keeping as-is")
    return b


# Per-class detectors

def _detect_rtc(
    names_norm: Sequence[str],
    device_id: str,
    warnings_out: List[str],
) -> Optional[float]:
    """Trigger iff at least 3 distinct time-tokens appear in register names."""
    import re

    def _tokens_in(name: str) -> set:
        # Split on separators, case transitions, and digit boundaries.
        parts = re.split(r"[^a-zA-Z0-9]+", name)
        out: set = set()
        for part in parts:
            if not part:
                continue
            # Further split CamelCase and digit boundaries.
            for sub in re.findall(r"[a-z]+|[A-Z]+(?=[A-Z]|$)|[A-Z][a-z]*|[0-9]+", part):
                token = sub.lower()
                out.add(token)
                if len(token) > 3 and token.endswith("s"):
                    out.add(token[:-1])
        return out

    hits: set = set()
    for n in names_norm:
        toks = _tokens_in(n)
        for tok in _RTC_TOKENS:
            if tok in toks:
                hits.add(tok)
    if len(hits) >= 3:
        return 0.50
    did = device_id.lower()
    if "rtc" in did and len(hits) >= 1:
        return 0.35
    return None


def _detect_memory(
    names_norm: Sequence[str],
    ir: Mapping[str, Any],
    device_id: str,
    warnings_out: List[str],
) -> Optional[float]:
    """Memory devices: large flat address space + no channels."""
    # Addressed storage often has a wide address and a sparse register map.
    addr_rule = ir.get("address_rule")
    if isinstance(addr_rule, Mapping):
        addr_size = addr_rule.get("address_size_bytes") or addr_rule.get("word_addr_bytes")
        try:
            addr_size_int = int(addr_size) if addr_size is not None else 0
        except (TypeError, ValueError):
            addr_size_int = 0
        if addr_size_int >= 2 and len(names_norm) <= 2:
            return 0.40

    return None


def _detect_display(
    names_norm: Sequence[str],
    ir: Mapping[str, Any],
    device_id: str,
    warnings_out: List[str],
) -> Optional[float]:
    """Display controllers: distinctive command tokens, write-heavy."""
    did = device_id.lower()
    for tok in _DISPLAY_TOKENS:
        if tok in did:
            return 0.55
        for n in names_norm:
            if tok in n:
                return 0.50

    # Write-heavy devices with no read path are display-like.
    read_seq = _as_list(ir.get("read_sequence"))
    init_seq = _as_list(ir.get("init_sequence"))
    if not read_seq and len(init_seq) >= 5:
        return 0.30

    return None


def _channel_root(name: str) -> Optional[str]:
    """Reduce a register name to its channel root, or None if control."""
    n = _normalise_name(name)

    # Strip chained layout suffixes while preserving axis suffixes.
    changed = True
    while changed:
        changed = False
        for suf in _LAYOUT_SUFFIXES:
            if n.endswith(suf):
                n = n[: -len(suf)]
                changed = True
                break

    # Drop trailing digit runs from non-channel names.
    n = re.sub(r"_?\d+$", "", n)

    if not n:
        return None

    # Reject names with pure control-token segments.
    segments = n.split("_")
    for seg in segments:
        if seg in _CONTROL_REG_TOKENS:
            return None

    # Keep axis-qualified roots distinct.
    for axis in _AXIS_SUFFIXES:
        if n.endswith(axis):
            return n  # preserve axis-qualified root

    # Match whole-segment or safe prefix channel keywords.
    for seg in segments:
        if seg in _CHANNEL_KEYWORDS:
            return n
        for kw in _CHANNEL_KEYWORDS:
            # Avoid matching short aliases inside unrelated words.
            if seg == kw or (len(seg) > len(kw) and seg.startswith(kw)
                             and not seg[len(kw)].isalpha()):
                return n

    return None


def _scan_text_channels(text: str) -> List[str]:
    """Extract channel-keyword hits from a free-form description string."""
    if not text:
        return []
    # Replace underscores before whole-word text scanning.
    lowered = re.sub(r"\s+", " ", text.lower().replace("_", " "))
    hits: List[str] = []
    seen: set = set()
    # Whole-word match per keyword to avoid natural-language false positives.
    for kw in _TEXT_CHANNEL_KEYWORDS:
        pattern = rf"\b{re.escape(kw)}\b"
        if re.search(pattern, lowered):
            canon = _CANONICAL_ALIAS.get(kw, kw)
            if canon not in seen:
                seen.add(canon)
                hits.append(canon)
    return hits


def _count_channels(
    names_norm: Sequence[str],
    read_seq: Sequence[Any],
    ir: Optional[Mapping[str, Any]] = None,
) -> Tuple[int, List[str]]:
    """Count distinct channels."""
    roots: set = set()

    # Authoritative read_channels
    if ir is not None:
        rc = ir.get("read_channels")
        if isinstance(rc, list):
            for entry in rc:
                if isinstance(entry, Mapping):
                    cid = entry.get("id") or entry.get("channel") or entry.get("name")
                    if isinstance(cid, str) and cid.strip():
                        normalised = _normalise_name(cid)
                        # Drop control and setpoint entries from channels.
                        segments = normalised.split("_")
                        if any(seg in _CONTROL_REG_TOKENS for seg in segments):
                            continue
                        if normalised in _SETPOINT_TOKENS:
                            continue
                        roots.add(normalised)
            # Trust explicit read_channels over derived roots.
            if roots:
                return len(roots), sorted(roots)

    for n in names_norm:
        r = _channel_root(n)
        if r is not None:
            roots.add(r)

    # Use read_sequence descriptors when register names are sparse.
    if not roots:
        for step in read_seq:
            if not isinstance(step, Mapping):
                continue
            for key in ("channel", "axis", "field"):
                v = step.get(key)
                if isinstance(v, str) and v.strip():
                    roots.add(_normalise_name(v))

    # Last fallback: scan free-form description text.
    if not roots and ir is not None:
        text_bag: List[str] = []
        for step in read_seq:
            if isinstance(step, Mapping):
                for k in ("action", "details", "description", "summary", "mode"):
                    v = step.get(k)
                    if isinstance(v, str):
                        text_bag.append(v)
                # Some IRs nest step strings.
                inner = step.get("steps")
                if isinstance(inner, list):
                    for item in inner:
                        if isinstance(item, str):
                            text_bag.append(item)
            elif isinstance(step, str):
                text_bag.append(step)
        for entry in _as_list(ir.get("conversion_formulae")):
            if isinstance(entry, Mapping):
                for k in ("name", "formula", "description", "notes",
                          "quantity", "channel", "field", "unit"):
                    v = entry.get(k)
                    if isinstance(v, str):
                        text_bag.append(v)
            elif isinstance(entry, str):
                text_bag.append(entry)
        # Also scan register-entry notes and descriptions.
        for entry in _as_list(ir.get("registers_or_commands")):
            if isinstance(entry, Mapping):
                for k in ("notes", "description", "summary", "details"):
                    v = entry.get(k)
                    if isinstance(v, str):
                        text_bag.append(v)
        combined = " ".join(text_bag)
        for hit in _scan_text_channels(combined):
            roots.add(hit)

    return len(roots), sorted(roots)


# Main entry

def classify_device(
    device_ir: Mapping[str, Any],
    task_package: Optional[Mapping[str, Any]] = None,
) -> ClassifyResult:
    """Classify a device into an eval_class, bus, channel count."""
    warnings_out: List[str] = []

    if not isinstance(device_ir, Mapping):
        warnings_out.append("device_ir is not a mapping; using empty dict")
        device_ir = {}

    device_id_raw = device_ir.get("device_id") or ""
    if not isinstance(device_id_raw, str):
        warnings_out.append("device_ir.device_id is not a string; coerced")
        device_id_raw = str(device_id_raw)
    device_id = device_id_raw.strip()

    bus, bus_conf = _resolve_bus(device_ir, task_package, warnings_out)

    raw_names = _collect_register_names(device_ir)
    names_norm = [_normalise_name(n) for n in raw_names]

    # I2C / SMBus specific branches.
    rule_applied = "fallback_single_channel"
    ec: Optional[str] = None
    conf_class = 0.0
    ch_count = 0
    ch_roots: List[str] = []

    # Trust an explicit access_model class when present.
    am = device_ir.get("access_model")
    if isinstance(am, Mapping):
        am_ec = am.get("eval_class")
        if isinstance(am_ec, str) and am_ec.strip() in {
            EVAL_CLASS_SINGLE_CHANNEL,
            EVAL_CLASS_MULTI_CHANNEL,
            EVAL_CLASS_MEMORY,
            EVAL_CLASS_DISPLAY,
            EVAL_CLASS_RTC,
        }:
            ec = am_ec.strip()
            conf_class = 0.90
            rule_applied = f"access_model_eval_class={ec}"

    if ec is None and bus in {"i2c", "smbus"}:
        boost = _detect_rtc(names_norm, device_id, warnings_out)
        if boost is not None:
            ec = EVAL_CLASS_RTC
            conf_class = boost
            rule_applied = "rtc_time_tokens"

        if ec is None:
            boost = _detect_memory(names_norm, device_ir, device_id, warnings_out)
            if boost is not None:
                ec = EVAL_CLASS_MEMORY
                conf_class = boost
                rule_applied = "memory_addr_size"

        if ec is None:
            boost = _detect_display(names_norm, device_ir, device_id, warnings_out)
            if boost is not None:
                ec = EVAL_CLASS_DISPLAY
                conf_class = boost
                rule_applied = "display_tokens"

    # Fall-through channel-count rule.
    if ec is None:
        read_seq = _as_list(device_ir.get("read_sequence"))
        ch_count, ch_roots = _count_channels(names_norm, read_seq, device_ir)

        if ch_count >= 2:
            ec = EVAL_CLASS_MULTI_CHANNEL
            conf_class = 0.45 + min(ch_count, 8) * 0.025   # bias modestly with channel-count
            rule_applied = f"multi_channel_count={ch_count}"
        else:
            ec = EVAL_CLASS_SINGLE_CHANNEL
            # One explicit channel is stronger than no channel evidence.
            conf_class = 0.40 if ch_count == 1 else 0.25
            rule_applied = (
                f"single_channel_count={ch_count}"
                if ch_count == 1
                else "fallback_single_channel"
            )

    total_conf = max(0.0, min(1.0, bus_conf + conf_class))
    return ClassifyResult(
        eval_class=ec,
        bus_type=bus,
        confidence=round(total_conf, 3),
        rule_applied=rule_applied,
        warnings=tuple(warnings_out),
        channel_count=ch_count,
        channel_roots=tuple(ch_roots),
    )


__all__ = [
    "EVAL_CLASS_SINGLE_CHANNEL",
    "EVAL_CLASS_MULTI_CHANNEL",
    "EVAL_CLASS_MEMORY",
    "EVAL_CLASS_DISPLAY",
    "EVAL_CLASS_RTC",
    "EVAL_CLASSES",
    "SUPPORTED_BUSES",
    "ClassifyResult",
    "classify_device",
]
