"""Shared helpers for SPI register-protocol hint handling."""
from __future__ import annotations

import dataclasses
from typing import Any, Mapping, Optional


_REGISTER_PROTO_NAMES = {"register", "reg", "register_map", "register-protocol"}


@dataclasses.dataclass(frozen=True)
class SpiProtocolHints:
    proto: Optional[str]
    rw_mask: int
    mb_mask: int
    addr_mask: int
    read_when_set: bool
    explicit: bool = False

    @property
    def is_register(self) -> bool:
        return str(self.proto or "").strip().lower() in _REGISTER_PROTO_NAMES

    def read_command(self, register_addr: int, *, length: int = 1) -> int:
        cmd = int(register_addr) & self.addr_mask
        if self.read_when_set:
            cmd |= self.rw_mask
        if length > 1:
            cmd |= self.mb_mask
        return cmd & 0xFF

    def write_command(self, register_addr: int) -> int:
        cmd = int(register_addr) & self.addr_mask
        if not self.read_when_set:
            cmd |= self.rw_mask
        return cmd & 0xFF


def _as_mapping(value: Any) -> Optional[Mapping[str, Any]]:
    return value if isinstance(value, Mapping) else None


def _as_int(value: Any) -> Optional[int]:
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float) and value.is_integer():
        return int(value)
    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None
        try:
            return int(text, 0)
        except ValueError:
            return None
    return None


def _as_bool(value: Any) -> Optional[bool]:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        text = value.strip().lower()
        if text in {"true", "yes", "1", "set", "read_when_set"}:
            return True
        if text in {"false", "no", "0", "clear", "read_when_clear"}:
            return False
    if isinstance(value, int) and value in (0, 1):
        return bool(value)
    return None


def _first_text(*values: Any) -> Optional[str]:
    for value in values:
        if isinstance(value, str) and value.strip():
            return value.strip().lower()
    return None


def _iter_hint_sources(
    *,
    device_ir: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    overrides: Optional[Mapping[str, Any]] = None,
):
    # Prefer explicit overrides before extracted or task-provided hints.
    for root in (overrides, device_ir, task_package):
        mapping = _as_mapping(root)
        if mapping is None:
            continue
        for key in ("protocol_hints", "spi_protocol", "connection_binding",
                    "connection", "bus_binding"):
            nested = _as_mapping(mapping.get(key))
            if nested is not None:
                yield nested
        fixed = _as_mapping(mapping.get("fixed_task_context"))
        if fixed is not None:
            device = _as_mapping(fixed.get("device"))
            if device is not None:
                hints = _as_mapping(device.get("protocol_hints"))
                if hints is not None:
                    yield hints
                yield device
            connection = _as_mapping(fixed.get("connection"))
            if connection is not None:
                hints = _as_mapping(connection.get("protocol_hints"))
                if hints is not None:
                    yield hints
                yield connection
        yield mapping


def _pick_int_from_sources(sources, keys: tuple[str, ...]) -> tuple[Optional[int], bool]:
    for source in sources:
        for key in keys:
            if key in source:
                value = _as_int(source.get(key))
                if value is not None:
                    return value, True
    return None, False


def _pick_bool_from_sources(sources, keys: tuple[str, ...]) -> tuple[Optional[bool], bool]:
    for source in sources:
        for key in keys:
            if key in source:
                value = _as_bool(source.get(key))
                if value is not None:
                    return value, True
    return None, False


def spi_protocol_hints(
    *,
    device_ir: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    overrides: Optional[Mapping[str, Any]] = None,
    default_proto: Optional[str] = None,
    default_rw_mask: int = 0x80,
    default_mb_mask: int = 0x00,
    default_addr_mask: int = 0x7F,
    default_read_when_set: bool = True,
) -> SpiProtocolHints:
    sources = list(_iter_hint_sources(
        device_ir=device_ir,
        task_package=task_package,
        overrides=overrides,
    ))
    proto = None
    for source in sources:
        proto = _first_text(
            source.get("spi_proto"),
            source.get("spi_sub_mode"),
            source.get("protocol"),
        )
        if proto:
            break

    rw_mask, rw_explicit = _pick_int_from_sources(
        sources,
        ("spi_rw_mask", "register_read_mask", "read_mask", "rw_mask"),
    )
    mb_mask, mb_explicit = _pick_int_from_sources(
        sources,
        ("spi_mb_mask", "multi_byte_mask", "mb_mask"),
    )
    addr_mask, addr_explicit = _pick_int_from_sources(
        sources,
        ("spi_addr_mask", "address_mask", "addr_mask"),
    )
    read_when_set, read_explicit = _pick_bool_from_sources(
        sources,
        ("spi_read_when_set", "read_when_set"),
    )

    return SpiProtocolHints(
        proto=proto or default_proto,
        rw_mask=(default_rw_mask if rw_mask is None else rw_mask) & 0xFF,
        mb_mask=(default_mb_mask if mb_mask is None else mb_mask) & 0xFF,
        addr_mask=(default_addr_mask if addr_mask is None else addr_mask) & 0xFF,
        read_when_set=(
            default_read_when_set if read_when_set is None else read_when_set
        ),
        explicit=bool(
            proto or rw_explicit or mb_explicit or addr_explicit or read_explicit
        ),
    )


__all__ = ["SpiProtocolHints", "spi_protocol_hints"]
