"""evaluation.runtime.slave_renderer - render per-stimulus slave .py files."""
from __future__ import annotations

import ast
from pathlib import Path
from typing import Any, Dict, Iterable, List, Tuple

from evaluation.infrastructure import SLAVES_DIR
from evaluation.oracle.schema import OracleMeta, Stimulus


# Tokens the renderer searches for. They MUST match the markers in the
# slave templates verbatim. Indentation in the marker text is tolerated.
DEVS_BLOCK_BEGIN = "# === DEVS_BLOCK_BEGIN ==="
DEVS_BLOCK_END = "# === DEVS_BLOCK_END ==="


class SlaveRenderError(Exception):
    """Raised when the slave template can't be rendered (missing markers,
    invalid mock_preload structure, etc.)."""


def _register_preload_target(
    default_addr: int,
    reg_key: object,
) -> Tuple[int, int] | None:
    """Map a preload key to ``(i2c_addr, register)``."""
    key_str = str(reg_key)
    if key_str == "read_bytes":
        # Direct-read or command-mode devices read from the default pointer.
        return default_addr, 0
    for prefix in ("reg_", "req_", "resp_"):
        if key_str.startswith(prefix):
            key_str = key_str[len(prefix):]
            break
    if ":" in key_str:
        addr_part, reg_part = key_str.split(":", 1)
        try:
            return int(addr_part, 0) & 0x7F, int(reg_part, 0) & 0xFF
        except ValueError:
            return None
    try:
        return default_addr, int(key_str, 0) & 0xFF
    except ValueError:
        return None


def _format_devs_block(
    addr: int,
    preload: Dict[str, List[int]],
    indent: str = "    ",
    *,
    i2c_access_model: str = "register",
) -> str:
    """Render the per-stimulus devs assignment block for I2C devices."""
    # Group preload entries by I2C address.
    #
    # Oracle-generated preloads usually use bare register keys or
    # ``addr:reg``. generated test_stimuli may also use schema-level
    # convenience keys such as ``read_bytes`` or ``reg_0x10``. Parse these
    # structural forms here so the Renode slave honors the same test-plan
    # contract that the generation-side normalizer accepts. Keys meant for
    # other slave kinds are skipped instead of crashing the whole probe.
    addr_regs: Dict[int, Dict[int, List[int]]] = {}
    direct_reads: Dict[int, List[int]] = {}
    skipped_keys: List[str] = []
    for reg_key, byte_list in preload.items():
        if str(reg_key) == "read_bytes":
            direct_reads[addr] = byte_list
        target = _register_preload_target(addr, reg_key)
        if target is None:
            skipped_keys.append(f"{reg_key!r} (not register preload key)")
            continue
        dev_addr, reg_int = target
        addr_regs.setdefault(dev_addr, {})[reg_int] = byte_list
    if skipped_keys:
        import logging
        logging.getLogger(__name__).debug(
            "render_i2c_register_slave: skipped %d non-register "
            "mock_preload key(s): %s",
            len(skipped_keys),
            ", ".join(skipped_keys),
        )

    # Ensure default address is always present
    if addr not in addr_regs:
        addr_regs[addr] = {}

    lines: list[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}devs = {{}}")
    lines.append(f"{indent}direct_read_bytes = {{}}")
    access_model = (i2c_access_model or "register").strip().lower()
    port_only = {addr} if access_model == "port" else set()
    command_mode = {addr} if access_model == "command" else set()
    if port_only:
        lines.append(
            f"{indent}port_only_devs = "
            + "{"
            + ", ".join(hex(a) for a in sorted(port_only))
            + "}"
        )
    else:
        lines.append(f"{indent}port_only_devs = set()")
    if command_mode:
        lines.append(
            f"{indent}command_mode_devs = "
            + "{"
            + ", ".join(hex(a) for a in sorted(command_mode))
            + "}"
        )
    else:
        lines.append(f"{indent}command_mode_devs = set()")
    for dev_addr in sorted(addr_regs):
        lines.append(f"{indent}devs[{hex(dev_addr)}] = {{}}")
        for reg_int, byte_list in sorted(addr_regs[dev_addr].items()):
            bytes_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in byte_list) + "]"
            lines.append(f"{indent}devs[{hex(dev_addr)}][{reg_int}] = {bytes_repr}")
    for dev_addr in sorted(direct_reads):
        byte_list = direct_reads[dev_addr]
        bytes_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in byte_list) + "]"
        lines.append(f"{indent}direct_read_bytes[{hex(dev_addr)}] = {bytes_repr}")
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def _replace_devs_block(template: str, new_block: str) -> str:
    """Locate BEGIN/END markers in `template` and substitute `new_block`."""
    begin_idx = template.find(DEVS_BLOCK_BEGIN)
    end_idx = template.find(DEVS_BLOCK_END)
    if begin_idx == -1 or end_idx == -1 or end_idx < begin_idx:
        raise SlaveRenderError(
            "DEVS_BLOCK_BEGIN/END markers not found (or out of order) "
            "in slave template"
        )
    line_begin = template.rfind("\n", 0, begin_idx) + 1
    line_end = template.find("\n", end_idx)
    if line_end == -1:
        line_end = len(template)
    line_end += 1  # consume the trailing newline of the END marker line
    return template[:line_begin] + new_block + template[line_end:]


def render_i2c_register_slave(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render an i2c_register_slave.py for `(meta, stim)` to `out_path`."""
    if meta.bus_type not in ("i2c", "smbus"):
        raise SlaveRenderError(
            f"i2c_register_slave only supports bus_type in {{i2c, smbus}}; "
            f"got bus_type={meta.bus_type!r}"
        )
    if meta.i2c_address_7bit is None:
        raise SlaveRenderError("meta.i2c_address_7bit is required for I2C")

    src = template_path or (SLAVES_DIR / "i2c_register_slave.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_devs_block(
        meta.i2c_address_7bit,
        stim.mock_preload,
        i2c_access_model=meta.i2c_access_model,
    )
    rendered = _replace_devs_block(template, new_block)

    # Sanity: must still parse as Python. Renode will run it, but we'd
    # rather fail here with a clear error than upstream in renode_exec.
    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered slave is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


def _derive_address_size(meta: OracleMeta) -> int:
    """Return the on-bus address width in bytes for a memory device."""
    if meta.i2c_address_size_bytes > 0:
        if meta.i2c_address_size_bytes not in (1, 2):
            raise SlaveRenderError(
                f"i2c_address_size_bytes={meta.i2c_address_size_bytes} "
                "not supported (expected 1 or 2)"
            )
        return meta.i2c_address_size_bytes
    if meta.memory_size_bytes <= 256:
        return 1
    return 2


def _collapse_runs(
    preload: Dict[str, List[int]], size: int
) -> List[Tuple[int, List[int]]]:
    """Flatten `{addr_str: bytes_list}` into a sorted list of `(start_addr, bytes_list)` tuples, validating bounds and overlap."""
    parsed: List[Tuple[int, List[int]]] = []
    for addr_key, byte_list in preload.items():
        try:
            start = int(str(addr_key), 0)
        except ValueError as e:
            raise SlaveRenderError(
                f"mock_preload key {addr_key!r} not parseable as int"
            ) from e
        if start < 0 or start >= size:
            raise SlaveRenderError(
                f"mock_preload key 0x{start:X} outside memory range "
                f"[0, 0x{size:X})"
            )
        if start + len(byte_list) > size:
            raise SlaveRenderError(
                f"mock_preload at 0x{start:X} with {len(byte_list)} bytes "
                f"overruns memory size 0x{size:X}"
            )
        parsed.append((start, [b & 0xFF for b in byte_list]))
    parsed.sort(key=lambda p: p[0])
    for i in range(1, len(parsed)):
        prev_end = parsed[i - 1][0] + len(parsed[i - 1][1])
        if parsed[i][0] < prev_end:
            raise SlaveRenderError(
                f"mock_preload regions overlap at 0x{parsed[i][0]:X} "
                f"(previous ends at 0x{prev_end:X})"
            )
    return parsed


def _format_memory_devs_block(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str = "    ",
) -> str:
    """Render the memory-slave DEVS block: device params + flat preloads."""
    size = meta.memory_size_bytes
    page = meta.memory_page_bytes if meta.memory_page_bytes > 0 else 1
    addr_width = _derive_address_size(meta)
    runs = _collapse_runs(preload, size)

    lines: list[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}target_addr_7bit   = {hex(meta.i2c_address_7bit)}")
    lines.append(f"{indent}memory_size_bytes  = {size}")
    lines.append(f"{indent}memory_page_bytes  = {page}")
    lines.append(f"{indent}address_size_bytes = {addr_width}")
    lines.append(f"{indent}memory = [0xFF] * memory_size_bytes")
    for start, byte_list in runs:
        end = start + len(byte_list)
        bytes_repr = "[" + ", ".join(f"0x{b:02X}" for b in byte_list) + "]"
        lines.append(f"{indent}memory[{start}:{end}] = {bytes_repr}")
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_i2c_memory_slave(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render an i2c_memory_slave.py for `(meta, stim)` to `out_path`."""
    if meta.bus_type not in ("i2c", "smbus"):
        raise SlaveRenderError(
            f"i2c_memory_slave only supports bus_type in {{i2c, smbus}}; "
            f"got bus_type={meta.bus_type!r}"
        )
    if meta.eval_class != "memory":
        raise SlaveRenderError(
            f"i2c_memory_slave expects eval_class='memory'; "
            f"got eval_class={meta.eval_class!r}"
        )
    if meta.i2c_address_7bit is None:
        raise SlaveRenderError("meta.i2c_address_7bit is required for I2C")
    if meta.memory_size_bytes <= 0:
        raise SlaveRenderError("meta.memory_size_bytes must be positive")

    src = template_path or (SLAVES_DIR / "i2c_memory_slave.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_memory_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered memory slave is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


def _format_display_devs_block(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str = "    ",
) -> str:
    """Render the display-slave DEVS block."""
    width  = meta.display_width  if meta.display_width  > 0 else 128
    height = meta.display_height if meta.display_height > 0 else 64
    if height % 8 != 0:
        raise SlaveRenderError(
            f"display_height={height} must be a multiple of 8"
        )
    pages = height // 8

    read_bytes: List[int] = []
    for key in ("status", "read"):
        if key in preload:
            read_bytes = [b & 0xFF for b in preload[key]]
            break

    lines: list[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}target_addr_7bit = {hex(meta.i2c_address_7bit)}")
    lines.append(f"{indent}display_width    = {width}")
    lines.append(f"{indent}display_height   = {height}")
    lines.append(f"{indent}display_pages    = {pages}")
    if read_bytes:
        bytes_repr = "[" + ", ".join(f"0x{b:02X}" for b in read_bytes) + "]"
        lines.append(f"{indent}mock_read        = {bytes_repr}")
    else:
        lines.append(f"{indent}mock_read        = []")
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_i2c_display_slave(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render an i2c_display_slave.py for `(meta, stim)` to `out_path`."""
    if meta.bus_type not in ("i2c", "smbus"):
        raise SlaveRenderError(
            f"i2c_display_slave only supports bus_type in {{i2c, smbus}}; "
            f"got bus_type={meta.bus_type!r}"
        )
    if meta.eval_class != "display":
        raise SlaveRenderError(
            f"i2c_display_slave expects eval_class='display'; "
            f"got eval_class={meta.eval_class!r}"
        )
    if meta.i2c_address_7bit is None:
        raise SlaveRenderError("meta.i2c_address_7bit is required for I2C")

    src = template_path or (SLAVES_DIR / "i2c_display_slave.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_display_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered display slave is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


# ----------------------------------------------------------------------
# SPI generic slave (register / stream / command protos)
# ----------------------------------------------------------------------

_SPI_REG_KEY_PREFIX = ""      # register-mode keys: plain hex ("0x00", "5")
_SPI_CMD_KEY_PREFIX = "cmd_"  # command-mode keys: "cmd_0x9F"
_SPI_STREAM_KEY     = "stream"
_SPI_PROTOS         = {"register", "stream", "command", "memory"}


def _parse_hex_key(key: str, context: str) -> int:
    try:
        return int(str(key), 0) & 0xFF
    except ValueError as e:
        raise SlaveRenderError(
            f"{context} key {key!r} is not parseable as an integer"
        ) from e


def _format_spi_register_body(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str,
) -> List[str]:
    lines: List[str] = []
    lines.append(f"{indent}rw_mask        = {hex(meta.spi_rw_mask & 0xFF)}")
    lines.append(f"{indent}mb_mask        = {hex(meta.spi_mb_mask & 0xFF)}")
    lines.append(f"{indent}addr_mask      = {hex(meta.spi_addr_mask & 0xFF)}")
    lines.append(f"{indent}read_when_set  = {bool(meta.spi_read_when_set)}")
    lines.append(f"{indent}regs = {{}}")
    for key, byte_list in preload.items():
        if key.startswith(_SPI_CMD_KEY_PREFIX) or key == _SPI_STREAM_KEY:
            continue  # not for this proto
        reg = _parse_hex_key(key, "spi register preload")
        bytes_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in byte_list) + "]"
        lines.append(f"{indent}regs[{hex(reg)}] = {bytes_repr}")
    lines.append(f"{indent}rx_stream = []")
    lines.append(f"{indent}cmd_table = {{}}")
    return lines


def _format_spi_stream_body(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str,
) -> List[str]:
    stream = preload.get(_SPI_STREAM_KEY, [])
    if not stream and len(preload) == 1:
        # Tolerate a single-entry preload with any key.
        stream = next(iter(preload.values()))
    bytes_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in stream) + "]"
    return [
        f"{indent}rw_mask        = 0x80",
        f"{indent}mb_mask        = 0x00",
        f"{indent}addr_mask      = 0x7F",
        f"{indent}read_when_set  = True",
        f"{indent}regs = {{}}",
        f"{indent}rx_stream = {bytes_repr}",
        f"{indent}cmd_table = {{}}",
    ]


def _format_spi_command_body(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str,
) -> List[str]:
    lines: List[str] = [
        f"{indent}rw_mask        = 0x80",
        f"{indent}mb_mask        = 0x00",
        f"{indent}addr_mask      = 0x7F",
        f"{indent}read_when_set  = True",
        f"{indent}regs = {{}}",
        f"{indent}rx_stream = []",
        f"{indent}cmd_table = {{}}",
    ]
    for key, byte_list in preload.items():
        if not key.startswith(_SPI_CMD_KEY_PREFIX):
            continue
        tail = key[len(_SPI_CMD_KEY_PREFIX):]
        opcode = _parse_hex_key(tail, "spi command preload")
        bytes_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in byte_list) + "]"
        lines.append(f"{indent}cmd_table[{hex(opcode)}] = {bytes_repr}")
    return lines


def _format_spi_memory_body(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str,
) -> List[str]:
    """Render DEVS body for ``proto = "memory"`` (SPI NOR flash)."""
    lines: List[str] = []
    lines.append(f"{indent}jedec_id     = {meta.jedec_id}")
    lines.append(f"{indent}memory_size  = {meta.memory_size_bytes}")
    lines.append(f"{indent}page_size    = {meta.memory_page_bytes}")
    lines.append(f"{indent}sector_size  = {meta.erase_unit}")

    # Build memory_bytes dict from preload
    parts: List[str] = []
    for key, values in sorted(preload.items()):
        try:
            addr = int(str(key), 0)
        except (ValueError, TypeError):
            continue
        vals_repr = ", ".join(str(int(v) & 0xFF) for v in values)
        parts.append(f"{indent}memory_bytes[{addr}] = [{vals_repr}]")
    if parts:
        lines.append(f"{indent}memory_bytes = {{}}")
        lines.extend(parts)
    else:
        lines.append(f"{indent}memory_bytes = {{}}")
    return lines


def render_spi_memory_slave(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render a ``spi_memory_slave.py`` for ``(meta, stim)`` to ``out_path``."""
    if meta.bus_type != "spi":
        raise SlaveRenderError(
            f"spi_memory_slave only supports bus_type='spi'; "
            f"got bus_type={meta.bus_type!r}"
        )
    src = template_path or (SLAVES_DIR / "spi_memory_slave.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")
    template = src.read_text(encoding="utf-8")
    new_block = _format_spi_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)
    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(f"rendered SPI memory slave is not valid Python: {e}") from e
    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


def _format_spi_devs_block(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str = "    ",
) -> str:
    """Render the SPI-slave DEVS block."""
    proto = (meta.spi_proto or "register").lower()
    if proto not in _SPI_PROTOS:
        raise SlaveRenderError(
            f"unknown spi_proto {meta.spi_proto!r}; expected one of "
            f"{sorted(_SPI_PROTOS)}"
        )

    lines: List[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}target_proto   = {proto!r}")
    if proto == "register":
        lines.extend(_format_spi_register_body(meta, preload, indent))
    elif proto == "stream":
        lines.extend(_format_spi_stream_body(meta, preload, indent))
    elif proto == "memory":
        lines.extend(_format_spi_memory_body(meta, preload, indent))
    else:  # command
        lines.extend(_format_spi_command_body(meta, preload, indent))
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_spi_generic_slave(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render a `spi_generic_slave.py` for `(meta, stim)` to `out_path`."""
    if meta.bus_type != "spi":
        raise SlaveRenderError(
            f"spi_generic_slave only supports bus_type='spi'; "
            f"got bus_type={meta.bus_type!r}"
        )

    src = template_path or (SLAVES_DIR / "spi_generic_slave.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_spi_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered SPI slave is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


# UART request/response bot.

_UART_DEFAULT_KEY   = "default"
_UART_REQ_PREFIX    = "req_"
_UART_FRAMINGS      = {"fixed", "delimiter"}


def _parse_hex_bytes(hex_text: str, context: str) -> List[int]:
    """Accept either '0xAA' / 'AA' / 'AABBCC' / 'AA BB CC' → list[int]."""
    s = hex_text.strip().lower()
    if s.startswith("0x"):
        s = s[2:]
    s = s.replace(" ", "").replace("_", "").replace(",", "")
    if len(s) % 2 != 0:
        raise SlaveRenderError(
            f"{context}: hex string {hex_text!r} must have even digit count"
        )
    try:
        return [int(s[i : i + 2], 16) for i in range(0, len(s), 2)]
    except ValueError as e:
        raise SlaveRenderError(
            f"{context}: hex string {hex_text!r} not parseable"
        ) from e


def _format_uart_devs_block(
    meta: OracleMeta,
    preload: Dict[str, List[int]],
    indent: str = "    ",
) -> str:
    """Render the UART bot DEVS block."""
    framing = (meta.uart_proto or "fixed").lower()
    if framing not in _UART_FRAMINGS:
        raise SlaveRenderError(
            f"unknown uart_proto {meta.uart_proto!r}; expected one of "
            f"{sorted(_UART_FRAMINGS)}"
        )
    if framing == "fixed" and meta.uart_packet_len <= 0:
        raise SlaveRenderError(
            "uart_proto='fixed' requires positive meta.uart_packet_len"
        )
    if framing == "delimiter" and not meta.uart_delimiter:
        raise SlaveRenderError(
            "uart_proto='delimiter' requires non-empty meta.uart_delimiter"
        )

    default_resp: List[int] = []
    table_entries: List[Tuple[Tuple[int, ...], List[int]]] = []
    for key, byte_list in preload.items():
        if key == _UART_DEFAULT_KEY:
            default_resp = [b & 0xFF for b in byte_list]
            continue
        if not key.startswith(_UART_REQ_PREFIX):
            # Tolerate keys we don't understand — reserved for future use.
            continue
        req_hex = key[len(_UART_REQ_PREFIX):]
        req_bytes = _parse_hex_bytes(req_hex, context=f"uart req key {key!r}")
        resp_bytes = [b & 0xFF for b in byte_list]
        table_entries.append((tuple(req_bytes), resp_bytes))

    # Sort by key tuple for determinism.
    table_entries.sort(key=lambda kv: kv[0])

    lines: List[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}framing        = {framing!r}")
    lines.append(f"{indent}packet_len     = {int(meta.uart_packet_len)}")
    delim_repr = (
        "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in meta.uart_delimiter) + "]"
    )
    lines.append(f"{indent}delimiter      = {delim_repr}")
    lines.append(f"{indent}cmd_response_table = {{}}")
    for key_tuple, resp in table_entries:
        key_repr = (
            "(" + ", ".join(f"0x{b & 0xFF:02X}" for b in key_tuple)
            + (",)" if len(key_tuple) == 1 else ")")
        )
        resp_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in resp) + "]"
        lines.append(f"{indent}cmd_response_table[{key_repr}] = {resp_repr}")
    default_repr = "[" + ", ".join(f"0x{b & 0xFF:02X}" for b in default_resp) + "]"
    lines.append(f"{indent}default_response = {default_repr}")
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_uart_bot(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render a `uart_request_response_bot.py` for `(meta, stim)` to `out_path`."""
    if meta.bus_type != "uart":
        raise SlaveRenderError(
            f"uart_bot only supports bus_type='uart'; "
            f"got bus_type={meta.bus_type!r}"
        )

    src = template_path or (SLAVES_DIR / "uart_request_response_bot.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_uart_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered UART bot is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


# GPIO pulse injector.

_GPIO_SCHEDULE_KEY = "schedule"
_GPIO_PAYLOAD_KEY  = "payload"
_GPIO_PREAMBLE_KEY = "preamble"

# DHT-family default bit encoding (in microseconds)
_DHT_DEFAULT_BIT_LOW_US       = 50
_DHT_DEFAULT_BIT_HIGH_US_ZERO = 27
_DHT_DEFAULT_BIT_HIGH_US_ONE  = 70
# DHT22 sensor handshake
_DHT_DEFAULT_PREAMBLE = [(0, 80), (1, 80)]


def _as_pulse_schedule(
    raw: Any,
    *,
    context: str,
) -> List[Tuple[int, int]]:
    """Coerce a raw preload value into a list of (level, duration_us)."""
    if not isinstance(raw, list):
        raise SlaveRenderError(
            f"{context}: expected a list of (level, duration_us) pairs; "
            f"got {type(raw).__name__}"
        )
    out: List[Tuple[int, int]] = []
    for i, item in enumerate(raw):
        if (
            not isinstance(item, (list, tuple))
            or len(item) != 2
        ):
            raise SlaveRenderError(
                f"{context}[{i}]: expected [level, duration_us] pair, "
                f"got {item!r}"
            )
        lvl, dur = item
        try:
            lvl_i = int(lvl) & 1
            dur_i = int(dur)
        except (TypeError, ValueError) as e:
            raise SlaveRenderError(
                f"{context}[{i}]: non-integer entries ({item!r})"
            ) from e
        if dur_i <= 0:
            raise SlaveRenderError(
                f"{context}[{i}]: duration must be positive, got {dur_i}"
            )
        out.append((lvl_i, dur_i))
    return out


def _encode_payload_bits(
    payload_bytes: List[int],
    bit_low_us: int,
    bit_high_us_zero: int,
    bit_high_us_one: int,
) -> List[Tuple[int, int]]:
    """Encode a list of bytes into a DHT-family pulse schedule."""
    schedule: List[Tuple[int, int]] = []
    for byte in payload_bytes:
        for shift in range(7, -1, -1):
            bit = (byte >> shift) & 1
            schedule.append((0, bit_low_us))
            schedule.append((
                1,
                bit_high_us_one if bit else bit_high_us_zero,
            ))
    return schedule


def _build_gpio_schedule_from_preload(
    preload: Dict[str, Any],
    context: str,
) -> List[Tuple[int, int]]:
    """Build a pulse schedule from `mock_preload`."""
    if _GPIO_SCHEDULE_KEY in preload:
        return _as_pulse_schedule(
            preload[_GPIO_SCHEDULE_KEY],
            context=f"{context}.schedule",
        )

    if _GPIO_PAYLOAD_KEY not in preload:
        return []

    raw_payload = preload[_GPIO_PAYLOAD_KEY]
    if isinstance(raw_payload, str):
        payload_bytes = _parse_hex_bytes(raw_payload, context=f"{context}.payload")
    elif isinstance(raw_payload, list):
        payload_bytes = [int(b) & 0xFF for b in raw_payload]
    else:
        raise SlaveRenderError(
            f"{context}.payload: expected list[int] or hex string, "
            f"got {type(raw_payload).__name__}"
        )

    bit_low   = int(preload.get("bit_low_us", _DHT_DEFAULT_BIT_LOW_US))
    bit_hi_0  = int(preload.get("bit_high_us_zero", _DHT_DEFAULT_BIT_HIGH_US_ZERO))
    bit_hi_1  = int(preload.get("bit_high_us_one", _DHT_DEFAULT_BIT_HIGH_US_ONE))
    if bit_low <= 0 or bit_hi_0 <= 0 or bit_hi_1 <= 0:
        raise SlaveRenderError(
            f"{context}.bit_*_us: all bit timing values must be positive"
        )

    if _GPIO_PREAMBLE_KEY in preload:
        preamble = _as_pulse_schedule(
            preload[_GPIO_PREAMBLE_KEY],
            context=f"{context}.preamble",
        )
    else:
        preamble = list(_DHT_DEFAULT_PREAMBLE)

    bits = _encode_payload_bits(payload_bytes, bit_low, bit_hi_0, bit_hi_1)
    # Trailing LOW pulse: the sensor briefly pulls LOW after the last bit
    # before releasing the bus.  Without this, a polling driver's final
    # "wait-for-HIGH-end" will never see a LOW edge and times out.
    trailing_low_us = int(preload.get("trailing_low_us", bit_low))
    return preamble + bits + [(0, trailing_low_us)]


def _format_gpio_schedule_literal(
    schedule: List[Tuple[int, int]],
    indent: str,
    line_width_hint: int = 80,
) -> str:
    """Render a pulse schedule list into deterministic Python source."""
    if not schedule:
        return f"{indent}pulse_schedule  = []\n"

    lines: List[str] = []
    lines.append(f"{indent}pulse_schedule  = [")
    for level, dur in schedule:
        lines.append(f"{indent}    ({level}, {dur}),")
    lines.append(f"{indent}]")
    return "\n".join(lines) + "\n"


def _format_gpio_devs_block(
    meta: OracleMeta,
    preload: Dict[str, Any],
    indent: str = "    ",
) -> str:
    """Render the GPIO pulse injector DEVS block."""
    if meta.bus_type != "gpio":
        raise SlaveRenderError(
            f"gpio_pulse_injector only supports bus_type='gpio'; "
            f"got bus_type={meta.bus_type!r}"
        )

    schedule = _build_gpio_schedule_from_preload(preload, context="mock_preload")
    if not schedule:
        raise SlaveRenderError(
            "mock_preload must provide either 'schedule' (list of "
            "[level, duration_us]) or 'payload' (bytes) to drive the "
            "GPIO pulse injector"
        )

    if not (0 <= meta.gpio_pin_number <= 15):
        raise SlaveRenderError(
            f"meta.gpio_pin_number={meta.gpio_pin_number} must be in [0, 15]"
        )
    if meta.gpio_trig_pin_number >= 0 and not (0 <= meta.gpio_trig_pin_number <= 15):
        raise SlaveRenderError(
            f"meta.gpio_trig_pin_number={meta.gpio_trig_pin_number} must be in [-1, 15]"
        )
    if meta.gpio_idle_level not in (0, 1):
        raise SlaveRenderError(
            f"meta.gpio_idle_level={meta.gpio_idle_level} must be 0 or 1"
        )
    if meta.gpio_tick_us <= 0:
        raise SlaveRenderError(
            f"meta.gpio_tick_us={meta.gpio_tick_us} must be positive"
        )

    lines: List[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}pin_number      = {int(meta.gpio_pin_number)}")
    lines.append(f"{indent}trig_pin_number = {int(meta.gpio_trig_pin_number)}")
    lines.append(f"{indent}idle_level      = {int(meta.gpio_idle_level)}")
    lines.append(f"{indent}tick_us         = {int(meta.gpio_tick_us)}")
    lines.append("")
    lines.append(_format_gpio_schedule_literal(schedule, indent).rstrip("\n"))
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_gpio_pulse_injector(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render a `gpio_pulse_injector.py` for `(meta, stim)` to `out_path`."""
    if meta.bus_type != "gpio":
        raise SlaveRenderError(
            f"gpio_pulse_injector only supports bus_type='gpio'; "
            f"got bus_type={meta.bus_type!r}"
        )

    src = template_path or (SLAVES_DIR / "gpio_pulse_injector.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_gpio_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered GPIO pulse injector is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


# ----------------------------------------------------------------------
# 1-Wire (Dallas) bit-slot injector (DS18B20)
# ----------------------------------------------------------------------

# Default presence-poll count for a standard DS18B20 eval flow:
#   drivergen_eval_init -> ds18b20_init -> ow_reset          (1 presence)
#   drivergen_eval_read -> ds18b20_read_temp -> 2x ow_reset  (2 presences)
# = 3 presence polls total before the 72-bit scratchpad read.
_OW_DEFAULT_PRESENCE_POLLS = 3
_OW_PAYLOAD_KEY            = "payload"
_OW_PRESENCE_KEY           = "n_presence_polls"


def _pack_scratchpad_bits(payload_bytes: List[int]) -> List[int]:
    """Expand scratchpad bytes into 72 response bits, LSB-first per byte."""
    bits: List[int] = []
    for byte in payload_bytes:
        for i in range(8):
            bits.append((byte >> i) & 1)
    return bits


def _format_ow_bitslot_devs_block(
    meta: OracleMeta,
    preload: Dict[str, Any],
    indent: str = "    ",
) -> str:
    """Render the 1-Wire bit-slot injector DEVS block."""
    if meta.bus_type != "gpio":
        raise SlaveRenderError(
            f"ow_bitslot_injector only supports bus_type='gpio'; "
            f"got bus_type={meta.bus_type!r}"
        )

    if _OW_PAYLOAD_KEY not in preload:
        raise SlaveRenderError(
            "mock_preload must provide 'payload' (list of scratchpad bytes) "
            "for the 1-Wire bit-slot injector"
        )

    raw_payload = preload[_OW_PAYLOAD_KEY]
    if isinstance(raw_payload, str):
        payload_bytes = _parse_hex_bytes(
            raw_payload, context="mock_preload.payload"
        )
    elif isinstance(raw_payload, list):
        payload_bytes = [int(b) & 0xFF for b in raw_payload]
    else:
        raise SlaveRenderError(
            f"mock_preload.payload: expected list[int] or hex string, "
            f"got {type(raw_payload).__name__}"
        )

    n_presence = int(preload.get(_OW_PRESENCE_KEY, _OW_DEFAULT_PRESENCE_POLLS))
    if n_presence < 0:
        raise SlaveRenderError(
            f"mock_preload.n_presence_polls={n_presence} must be >= 0"
        )

    bits = _pack_scratchpad_bits(payload_bytes)

    if not (0 <= meta.gpio_pin_number <= 15):
        raise SlaveRenderError(
            f"meta.gpio_pin_number={meta.gpio_pin_number} must be in [0, 15]"
        )
    if meta.gpio_idle_level not in (0, 1):
        raise SlaveRenderError(
            f"meta.gpio_idle_level={meta.gpio_idle_level} must be 0 or 1"
        )

    # Build the response stream: [0]*n_presence + scratchpad_bits
    response_lines: List[str] = [f"{indent}response_bits    = ["]
    # Presence polls first (all zeros to signal "device present").
    presence_chunk = ", ".join("0" for _ in range(n_presence))
    if presence_chunk:
        response_lines.append(f"{indent}    {presence_chunk},  # presence polls")
    # Then 8 bits per scratchpad byte.
    for bi, byte in enumerate(payload_bytes):
        byte_bits = bits[bi * 8 : (bi + 1) * 8]
        bits_str = ", ".join(str(b) for b in byte_bits)
        response_lines.append(
            f"{indent}    {bits_str},  # byte {bi} = 0x{byte & 0xFF:02X}"
        )
    response_lines.append(f"{indent}]")

    lines: List[str] = []
    lines.append(f"{indent}{DEVS_BLOCK_BEGIN}")
    lines.append(f"{indent}# Per-stimulus preload (rendered by slave_renderer).")
    lines.append(f"{indent}pin_number       = {int(meta.gpio_pin_number)}")
    lines.append(f"{indent}idle_level       = {int(meta.gpio_idle_level)}")
    lines.append(f"{indent}n_presence_polls = {n_presence}")
    lines.extend(response_lines)
    lines.append(f"{indent}{DEVS_BLOCK_END}")
    return "\n".join(lines) + "\n"


def render_ow_bitslot_injector(
    meta: OracleMeta,
    stim: Stimulus,
    out_path: Path,
    *,
    template_path: Path | None = None,
) -> Path:
    """Render an `ow_bitslot_injector.py` for `(meta, stim)` to `out_path`."""
    if meta.bus_type != "gpio":
        raise SlaveRenderError(
            f"ow_bitslot_injector only supports bus_type='gpio'; "
            f"got bus_type={meta.bus_type!r}"
        )

    src = template_path or (SLAVES_DIR / "ow_bitslot_injector.py")
    if not src.is_file():
        raise SlaveRenderError(f"slave template not found: {src}")

    template = src.read_text(encoding="utf-8")
    new_block = _format_ow_bitslot_devs_block(meta, stim.mock_preload)
    rendered = _replace_devs_block(template, new_block)

    try:
        ast.parse(rendered)
    except SyntaxError as e:
        raise SlaveRenderError(
            f"rendered 1-Wire bitslot injector is not valid Python: {e}"
        ) from e

    out_path = Path(out_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(rendered, encoding="utf-8")
    return out_path


__all__ = [
    "render_i2c_register_slave",
    "render_i2c_memory_slave",
    "render_i2c_display_slave",
    "render_spi_generic_slave",
    "render_spi_memory_slave",
    "render_uart_bot",
    "render_gpio_pulse_injector",
    "render_ow_bitslot_injector",
    "SlaveRenderError",
]
