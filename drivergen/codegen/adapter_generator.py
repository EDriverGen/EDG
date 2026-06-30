"""template-driven generator for `<device>_eval_adapter.c`."""
from __future__ import annotations

import dataclasses
import re
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
    ClassifyResult,
)


# Public result model

@dataclasses.dataclass(frozen=True)
class GeneratedAdapter:
    """Outcome of :func:`generate_adapter`."""
    device_id: str
    eval_class: str
    bus_kind: str
    source_c: str
    warnings: Tuple[str, ...]
    params: Mapping[str, Any]


class AdapterContractError(ValueError):
    """Raised when ``api_contract`` is malformed beyond recovery."""


# api_contract schemas

# Each schema is a tuple (required_top_level_keys, per_class_checker).
_SCHEMAS: Dict[str, List[str]] = {
    EVAL_CLASS_SINGLE_CHANNEL: [
        "init_call", "read_call",
        "primary_raw_type", "primary_raw_unit",
    ],
    EVAL_CLASS_MULTI_CHANNEL: [
        "init_call", "channels",
    ],
    EVAL_CLASS_MEMORY: [
        "init_call", "mem_read_call", "mem_write_call",
        "memory_size_bytes",
    ],
    EVAL_CLASS_DISPLAY: [
        "init_call", "output_frame_call",
    ],
    EVAL_CLASS_RTC: [
        "init_call", "get_time_call",
    ],
}

API_CONTRACT_SCHEMAS: Mapping[str, Sequence[str]] = _SCHEMAS
"""Public view of required-keys-per-class. Immutable."""


# Template building blocks

_C_IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def _is_valid_c_identifier(s: str) -> bool:
    return bool(_C_IDENT_RE.match(s))


def _sanitize_device_id_for_c(device_id: str) -> str:
    """Map arbitrary device id to a safe C identifier suffix."""
    s = re.sub(r"[^A-Za-z0-9_]", "_", device_id.strip().lower())
    if s and s[0].isdigit():
        s = "_" + s
    return s or "device"


def _normalise_call(call_str: str, *, what: str) -> str:
    """Strip trailing semicolon + whitespace from a call expression."""
    if not isinstance(call_str, str):
        raise AdapterContractError(f"{what} must be a string, got {type(call_str).__name__}")
    s = call_str.strip().rstrip(";").strip()
    if not s:
        raise AdapterContractError(f"{what} is empty")
    s = _rewrite_buffer_local(s, what=what)
    return s


_BUFFER_LOCAL_ALIASES: Dict[str, Tuple[str, str]] = {
    # contract key -> (canonical_local, alternate_local)
    "mem_read_call":     ("buf", "data"),
    "mem_write_call":    ("buf", "data"),
    "output_frame_call": ("data", "buf"),
}


def _rewrite_buffer_local(call_str: str, *, what: str) -> str:
    """Normalize buffer local names used in contract snippets."""
    pair = _BUFFER_LOCAL_ALIASES.get(what)
    if pair is None:
        return call_str
    canonical, mistake = pair
    mistake_pat = re.compile(rf"\b{re.escape(mistake)}\b")
    canon_pat   = re.compile(rf"\b{re.escape(canonical)}\b")
    if mistake_pat.search(call_str) and not canon_pat.search(call_str):
        return mistake_pat.sub(canonical, call_str)
    return call_str


def _validate_contract(
    eval_class: str,
    api_contract: Mapping[str, Any],
) -> List[str]:
    """Return a list of validation WARNINGS; raise on fatal errors."""
    if eval_class not in _SCHEMAS:
        raise AdapterContractError(f"unknown eval_class {eval_class!r}")
    required = _SCHEMAS[eval_class]
    missing = [k for k in required if k not in api_contract]
    if missing:
        raise AdapterContractError(
            f"api_contract for {eval_class!r} is missing required keys: {missing}"
        )
    # Cross-check: declared eval_class (if any) matches.
    declared = api_contract.get("eval_class")
    if declared is not None and declared != eval_class:
        raise AdapterContractError(
            f"api_contract.eval_class={declared!r} conflicts with "
            f"classifier result {eval_class!r}"
        )
    warnings_out: List[str] = []
    if eval_class == EVAL_CLASS_MULTI_CHANNEL:
        channels = api_contract.get("channels")
        if not isinstance(channels, list) or not channels:
            raise AdapterContractError(
                "multi_channel api_contract.channels must be a non-empty list"
            )
        for i, ch in enumerate(channels):
            if not isinstance(ch, Mapping):
                raise AdapterContractError(
                    f"multi_channel api_contract.channels[{i}] is not a mapping"
                )
            for k in ("id", "call", "out_arg", "out_type"):
                if k not in ch:
                    raise AdapterContractError(
                        f"multi_channel api_contract.channels[{i}] missing key {k!r}"
                    )
    if eval_class == EVAL_CLASS_MEMORY:
        if not isinstance(api_contract.get("memory_size_bytes"), int):
            raise AdapterContractError(
                "memory api_contract.memory_size_bytes must be int"
            )
    if eval_class == EVAL_CLASS_DISPLAY:
        if not api_contract.get("read_status_call"):
            warnings_out.append(
                "display api_contract has no read_status_call; adapter will "
                "return DRIVERGEN_EVAL_ERR_UNSUPPORTED (harness tolerates this)"
            )
    if eval_class == EVAL_CLASS_RTC:
        if not api_contract.get("set_time_call"):
            warnings_out.append(
                "rtc api_contract has no set_time_call; adapter will "
                "return DRIVERGEN_EVAL_ERR_UNSUPPORTED for set_time"
            )
    return warnings_out


# External bus-handle declarations

_HAL_HANDLE_PATTERNS: List[Tuple[str, str]] = [
    # (regex, handle type name)
    (r"\bhi2c[0-9]+\b",  "I2C_HandleTypeDef"),
    (r"\bhspi[0-9]+\b",  "SPI_HandleTypeDef"),
    (r"\bhuart[0-9]+\b", "UART_HandleTypeDef"),
    (r"\bhusart[0-9]+\b","USART_HandleTypeDef"),
    (r"\bhcan[0-9]+\b",  "CAN_HandleTypeDef"),
]


def _auto_extern_hal_handles(
    *, init_call: str, api_contract: Mapping[str, Any],
) -> str:
    """Inject missing external handle declarations referenced by init_call."""
    haystacks: List[str] = [
        init_call,
    ]
    for k in ("preamble_c", "init_extra_setup_c"):
        v = api_contract.get(k)
        if isinstance(v, str) and v:
            haystacks.append(v)
    blob = "\n".join(haystacks)

    declared: List[str] = []
    seen: set = set()
    for pat, type_name in _HAL_HANDLE_PATTERNS:
        for handle in re.findall(pat, init_call):
            if handle in seen:
                continue
            seen.add(handle)
            decl_pat = re.compile(
                r"\bextern\s+" + re.escape(type_name) + r"\s+" + re.escape(handle) + r"\b|"
                r"\b" + re.escape(type_name) + r"\s+" + re.escape(handle) + r"\b"
            )
            if decl_pat.search(blob):
                continue
            declared.append(
                f"    /* auto-injected external bus handle */\n"
                f"    extern {type_name} {handle};\n"
            )
    return "".join(declared)


_DEVICE_API_TOKEN_RE = re.compile(
    r"\b(?:rt_[A-Za-z0-9_]+|RT_[A-Z0-9_]+)\b|"
    r"\bstruct\s+rt_[A-Za-z0-9_]+\b"
)
_DEVICE_API_HEADER_INCLUDE_RE = re.compile(
    r'#\s*include\s*[<"](?:rtthread|rtdevice|drivers/dev_i2c)\.h[>"]'
)

_HAL_ADAPTER_TOKEN_RE = re.compile(
    r'\b(?:I2CDriver|I2CD\d|I2CConfig|i2cStart|i2cAcquireBus'
    r'|i2cReleaseBus|i2cMasterTransmit|i2cMasterReceive|SPIDriver'
    r'|SPIConfig|spiStart|spiAcquireBus)\b'
)


def _auto_adapter_preamble_includes(api_contract: Mapping[str, Any]) -> str:
    """Infer adapter-only includes needed by contract snippets."""
    preamble = api_contract.get("preamble_c") or ""
    if not isinstance(preamble, str):
        preamble = ""
    if _DEVICE_API_HEADER_INCLUDE_RE.search(preamble):
        return ""

    blobs: List[str] = []
    for key in ("init_call", "init_extra_setup_c", "preamble_c"):
        value = api_contract.get(key)
        if isinstance(value, str) and value:
            blobs.append(value)
    joined = "\n".join(blobs)

    if blobs and _DEVICE_API_TOKEN_RE.search(joined):
        return (
            "/* auto-included for contract support code */\n"
            "#include <rtthread.h>\n"
            "#include <rtdevice.h>\n"
        )

    if blobs and _HAL_ADAPTER_TOKEN_RE.search(joined):
        return (
            "/* auto-included for contract support code */\n"
            '#include "hal.h"\n'
        )

    return ""


# Per-class renderers

# Header preamble common to every class.
_HEADER_PREAMBLE = """\
/* {device_id}_eval_adapter.c
 *
 * Do NOT edit by hand; changes will be overwritten on the next
 *
 * eval_class : {eval_class}
 * bus_kind   : {bus_kind}
 * device_id  : {device_id}
 */
#include "drivergen_eval_adapter.h"
#include "{driver_header}"
"""

_META_TEMPLATE = """\

const drivergen_eval_meta_t drivergen_eval_meta = {{
    .device_id          = "{device_id}",
    .eval_class         = DRIVERGEN_EVAL_CLASS_{eval_class_macro},
    .channel_count      = {channel_count},
    .channels           = {channels_expr},
    .primary_id         = "{primary_id}",
    .primary_unit       = "{primary_unit}",
    .memory_size_bytes  = {memory_size_bytes},
    .memory_page_bytes  = {memory_page_bytes},
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
}};
"""

_CLEANUP_STUB = """\

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
"""

# The init wrapper treats zero as success and nonzero as I/O failure.
_INIT_TEMPLATE = """\

int drivergen_eval_init(const char *bus_name) {{
    if (bus_name == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
{init_extra_setup}\
    int _rc = (int)({init_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}}
"""


def _build_init_block(
    init_call: str,
    init_extra_setup: str = "",
) -> str:
    return _INIT_TEMPLATE.format(
        init_call=init_call,
        init_extra_setup=init_extra_setup,
    )


# ---- single_channel ----

_SINGLE_CHANNEL_TEMPLATE = """\

int drivergen_eval_read_raw_i32(int32_t *out) {{
    if (out == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    {primary_raw_type} raw = 0;
    int _rc = (int)({read_call});
    if (_rc != 0) {{
        return DRIVERGEN_EVAL_ERR_IO;
    }}
    *out = (int32_t)raw;
    return DRIVERGEN_EVAL_OK;
}}
"""


def _render_single_channel(
    params: Dict[str, Any], api_contract: Mapping[str, Any],
) -> str:
    primary_raw_type = api_contract["primary_raw_type"]
    read_call = _normalise_call(api_contract["read_call"], what="read_call")
    return _SINGLE_CHANNEL_TEMPLATE.format(
        primary_raw_type=primary_raw_type,
        read_call=read_call,
    )


# ---- multi_channel ----

_MULTI_CHANNEL_CHANNELS_TABLE = """\

static const drivergen_eval_channel_t g_channels[{count}] = {{
{entries}
}};
"""

_MULTI_CHANNEL_READ_TEMPLATE = """\

static int32_t g_cached[{count}];
static int     g_sample_valid = 0;

static int {sanitized}_eval_refresh_cache(void) {{
{refresh_body}
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {{
    if (out == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    if (channel_id < 0 || channel_id >= {count}) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    if (channel_id == 0 || !g_sample_valid) {{
        int _rc = {sanitized}_eval_refresh_cache();
        if (_rc != DRIVERGEN_EVAL_OK) {{
            return _rc;
        }}
    }}
    *out = g_cached[channel_id];
    return DRIVERGEN_EVAL_OK;
}}
"""


def _render_multi_channel(
    params: Dict[str, Any], api_contract: Mapping[str, Any],
) -> str:
    channels = api_contract["channels"]
    entries = ",\n".join(
        f'    {{"{ch["id"]}", "{ch.get("physical_unit", "raw")}", 0}}'
        for ch in channels
    )
    channels_table = _MULTI_CHANNEL_CHANNELS_TABLE.format(
        count=len(channels), entries=entries,
    )
    params["channel_count"] = len(channels)
    params["channels_expr"] = "g_channels"
    params["primary_id"] = channels[0]["id"]
    params["primary_unit"] = channels[0].get("physical_unit", "raw")

    # Group identical calls so one C call can update several cached slots.
    refresh_lines: List[str] = []
    declared_locals: set = set()
    call_to_indices: List[Tuple[str, List[Tuple[int, str, str]]]] = []
    for idx, ch in enumerate(channels):
        call = _normalise_call(ch["call"], what=f"channels[{idx}].call")
        out_arg = ch["out_arg"]
        out_type = ch["out_type"]
        if not _is_valid_c_identifier(out_arg):
            raise AdapterContractError(
                f"channels[{idx}].out_arg must be a C identifier, got {out_arg!r}"
            )
        # Group consecutive same calls.
        if call_to_indices and call_to_indices[-1][0] == call:
            call_to_indices[-1][1].append((idx, out_arg, out_type))
        else:
            call_to_indices.append((call, [(idx, out_arg, out_type)]))

    for call, slots in call_to_indices:
        # Declare locals (once per out_arg name).
        for (_idx, out_arg, out_type) in slots:
            if out_arg not in declared_locals:
                refresh_lines.append(f"    {out_type} {out_arg} = 0;")
                declared_locals.add(out_arg)
        refresh_lines.append(f"    {{")
        refresh_lines.append(f"        int _rc = (int)({call});")
        refresh_lines.append(f"        if (_rc != 0) {{")
        refresh_lines.append(f"            return DRIVERGEN_EVAL_ERR_IO;")
        refresh_lines.append(f"        }}")
        # Shared output locals are split into per-slot bits; distinct locals
        # are cached as whole values.
        _shared_arg = (
            slots[0][1]
            if len(slots) > 1 and all(oa == slots[0][1] for _, oa, _ in slots)
            else None
        )
        if _shared_arg is not None:
            _bit = 0
            for (i, oa, _) in slots:
                _expr = f"((int32_t)({oa}) >> {_bit}) & 1"
                refresh_lines.append(
                    f"        g_cached[{i}] = {_expr};"
                )
                _bit += 1
        else:
            for (idx, out_arg, _) in slots:
                refresh_lines.append(
                    f"        g_cached[{idx}] = (int32_t){out_arg};"
                )
        refresh_lines.append(f"    }}")

    refresh_body = "\n".join(refresh_lines)
    read_block = _MULTI_CHANNEL_READ_TEMPLATE.format(
        count=len(channels),
        sanitized=params["sanitized"],
        refresh_body=refresh_body,
    )
    return channels_table + read_block


# ---- memory ----

_MEMORY_TEMPLATE = """\

int drivergen_eval_mem_read(uint32_t addr, uint8_t *buf, uint16_t len) {{
    if (buf == NULL || len == 0) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    int _rc = (int)({mem_read_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}}

int drivergen_eval_mem_write(uint32_t addr, const uint8_t *buf, uint16_t len) {{
    if (buf == NULL || len == 0) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    int _rc = (int)({mem_write_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}}
"""


def _render_memory(
    params: Dict[str, Any], api_contract: Mapping[str, Any],
) -> str:
    params["memory_size_bytes"] = api_contract["memory_size_bytes"]
    params["memory_page_bytes"] = api_contract.get("memory_page_bytes", 0)
    return _MEMORY_TEMPLATE.format(
        mem_read_call=_normalise_call(
            api_contract["mem_read_call"], what="mem_read_call"),
        mem_write_call=_normalise_call(
            api_contract["mem_write_call"], what="mem_write_call"),
    )


# ---- display ----

_DISPLAY_TEMPLATE = """\

int drivergen_eval_output_frame(const uint8_t *data, uint16_t len) {{
    if (data == NULL || len == 0) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    int _rc = (int)({output_frame_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}}

int drivergen_eval_read_status(uint8_t *out) {{
{read_status_body}
}}
"""

_DISPLAY_STATUS_SUPPORTED = """\
    if (out == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    int _rc = (int)({read_status_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;\
"""

_DISPLAY_STATUS_UNSUPPORTED = """\
    (void)out;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;\
"""


def _render_display(
    params: Dict[str, Any], api_contract: Mapping[str, Any],
) -> str:
    rs_call = api_contract.get("read_status_call")
    if rs_call:
        read_status_body = _DISPLAY_STATUS_SUPPORTED.format(
            read_status_call=_normalise_call(rs_call, what="read_status_call"),
        )
    else:
        read_status_body = _DISPLAY_STATUS_UNSUPPORTED
    return _DISPLAY_TEMPLATE.format(
        output_frame_call=_normalise_call(
            api_contract["output_frame_call"], what="output_frame_call"),
        read_status_body=read_status_body,
    )


# ---- rtc ----

_RTC_TIME_FIELDS = ("year", "month", "day", "hour", "minute",
                     "second", "weekday")

_RTC_GET_TEMPLATE = """\

int drivergen_eval_get_time(drivergen_eval_time_t *out) {{
    if (out == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    {time_struct_decl}
    int _rc = (int)({get_time_call});
    if (_rc != 0) {{
        return DRIVERGEN_EVAL_ERR_IO;
    }}
{field_assignments}
    out->reserved = 0;
    return DRIVERGEN_EVAL_OK;
}}
"""

_RTC_SET_SUPPORTED = """\

int drivergen_eval_set_time(const drivergen_eval_time_t *in) {{
    if (in == NULL) {{
        return DRIVERGEN_EVAL_ERR_INVALID;
    }}
    {time_struct_from_in}
    int _rc = (int)({set_time_call});
    return (_rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}}
"""

_RTC_SET_UNSUPPORTED = """\

int drivergen_eval_set_time(const drivergen_eval_time_t *in) {{
    (void)in;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}}
"""


def _render_rtc(
    params: Dict[str, Any], api_contract: Mapping[str, Any],
) -> str:
    # Default to the ABI time struct when no native mapping is supplied.
    time_struct_decl = api_contract.get(
        "time_struct_decl", "drivergen_eval_time_t t;",
    )
    get_time_call = _normalise_call(
        api_contract["get_time_call"], what="get_time_call",
    )
    time_field_map = api_contract.get("time_fields")
    if isinstance(time_field_map, Mapping):
        assignments: List[str] = []
        for f in _RTC_TIME_FIELDS:
            expr = time_field_map.get(f)
            if expr is None:
                assignments.append(f"    out->{f} = 0;")
            else:
                assignments.append(f"    out->{f} = ({_rtc_field_c_type(f)})({expr});")
        field_assignments = "\n".join(assignments)
    else:
        # ABI-native default.
        field_assignments = "    *out = t;"

    get_block = _RTC_GET_TEMPLATE.format(
        time_struct_decl=time_struct_decl,
        get_time_call=get_time_call,
        field_assignments=field_assignments,
    )

    set_call = api_contract.get("set_time_call")
    if set_call:
        time_struct_from_in = api_contract.get(
            "time_struct_from_in", "drivergen_eval_time_t t = *in;",
        )
        set_call = _normalise_call(set_call, what="set_time_call")
        set_block = _RTC_SET_SUPPORTED.format(
            time_struct_from_in=time_struct_from_in,
            set_time_call=set_call,
        )
    else:
        set_block = _RTC_SET_UNSUPPORTED
    return get_block + set_block


def _rtc_field_c_type(field: str) -> str:
    """Return the ABI field's C type for explicit casts."""
    return "uint16_t" if field == "year" else "uint8_t"


# Public entry

_EVAL_CLASS_MACRO: Dict[str, str] = {
    EVAL_CLASS_SINGLE_CHANNEL: "SINGLE_CHANNEL",
    EVAL_CLASS_MULTI_CHANNEL:  "MULTI_CHANNEL",
    EVAL_CLASS_MEMORY:         "MEMORY",
    EVAL_CLASS_DISPLAY:        "DISPLAY",
    EVAL_CLASS_RTC:            "RTC",
}


def generate_adapter(
    classify_result: ClassifyResult,
    api_contract: Mapping[str, Any],
    *,
    device_id: Optional[str] = None,
    driver_header: Optional[str] = None,
) -> GeneratedAdapter:
    """Render ``<device>_eval_adapter.c`` from an api_contract."""
    eval_class = classify_result.eval_class
    warnings_out = _validate_contract(eval_class, api_contract)

    dev = device_id or api_contract.get("device_id")
    if not dev:
        raise AdapterContractError(
            "device_id not provided (neither as arg nor in api_contract)"
        )
    sanitized = _sanitize_device_id_for_c(dev)
    hdr = driver_header or api_contract.get("driver_header") or f"{dev}.h"

    params: Dict[str, Any] = {
        "device_id":         dev,
        "sanitized":         sanitized,
        "eval_class":        eval_class,
        "eval_class_macro":  _EVAL_CLASS_MACRO[eval_class],
        "bus_kind":          classify_result.bus_type,
        "driver_header":     hdr,
        # Defaults filled in per branch; overrides below.
        "channel_count":     0,
        "channels_expr":     "NULL",
        "primary_id":        api_contract.get("primary_id", dev),
        "primary_unit":      api_contract.get("primary_raw_unit") or "raw",
        "memory_size_bytes": 0,
        "memory_page_bytes": 0,
    }

    init_call = _normalise_call(api_contract["init_call"], what="init_call")
    init_extra_setup = api_contract.get("init_extra_setup_c", "") or ""

    # Add missing extern declarations for referenced bus handles.
    _hal_handle_decls = _auto_extern_hal_handles(
        init_call=init_call,
        api_contract=api_contract,
    )
    if _hal_handle_decls:
        init_extra_setup = _hal_handle_decls + init_extra_setup

    # Replace string-pointer bus handles with a typed UART handle when needed.
    if classify_result.bus_type == "uart":
        _llm_preamble = api_contract.get("preamble_c") or ""
        _hal_indicators = (
            "UART_HandleTypeDef", "HAL_UART",
        )
        _uses_hal = any(
            ind in _llm_preamble or ind in init_extra_setup
            for ind in _hal_indicators
        )
        if _uses_hal:
            # Use a valid typed handle instead of a string-address cast.
            init_extra_setup = "static UART_HandleTypeDef g_huart;\n"
            init_call = re.sub(
                r'\(void\s*\*\s*\)\s*bus_name\b',
                "&g_huart",
                init_call,
            )

    if init_extra_setup and not init_extra_setup.endswith("\n"):
        init_extra_setup += "\n"
    init_block = _build_init_block(
        init_call=init_call, init_extra_setup=init_extra_setup,
    )

    if eval_class == EVAL_CLASS_SINGLE_CHANNEL:
        class_block = _render_single_channel(params, api_contract)
    elif eval_class == EVAL_CLASS_MULTI_CHANNEL:
        class_block = _render_multi_channel(params, api_contract)
    elif eval_class == EVAL_CLASS_MEMORY:
        class_block = _render_memory(params, api_contract)
    elif eval_class == EVAL_CLASS_DISPLAY:
        class_block = _render_display(params, api_contract)
    elif eval_class == EVAL_CLASS_RTC:
        class_block = _render_rtc(params, api_contract)
    else:  # defensive; _validate_contract already raised
        raise AdapterContractError(f"unknown eval_class {eval_class!r}")

    # Add the canonical device object unless the preamble already defines it.
    preamble_c = api_contract.get("preamble_c") or ""
    auto_preamble = _auto_adapter_preamble_includes(api_contract)
    if auto_preamble:
        preamble_c = auto_preamble + preamble_c
    dev_struct_type = api_contract.get("dev_struct_type")
    # Trust an existing g_eval_dev declaration in the preamble.
    already_declares_dev = bool(
        re.search(r"\bg_eval_dev\b", preamble_c)
    )
    if dev_struct_type and not already_declares_dev:
        dst = dev_struct_type.strip()
        # Emit our own storage-class keyword.
        if dst.startswith("static "):
            dst = dst[len("static "):].strip()
        is_pointer = dst.endswith("*")
        if is_pointer:
            auto_decl = (
                f"\n/* auto-declared adapter state */\n"
                f"static {dst} g_eval_dev = NULL;\n"
            )
        else:
            auto_decl = (
                f"\n/* auto-declared adapter state */\n"
                f"static {dst} g_eval_dev;\n"
            )
        preamble_c = preamble_c + auto_decl
    if preamble_c and not preamble_c.startswith("\n"):
        preamble_c = "\n" + preamble_c
    if preamble_c and not preamble_c.endswith("\n"):
        preamble_c = preamble_c + "\n"

    parts: List[str] = [
        _HEADER_PREAMBLE.format(**params),
        preamble_c,
    ]
    # Put the channel table before metadata when metadata references it.
    if eval_class == EVAL_CLASS_MULTI_CHANNEL:
        # Peel off the channel table before rendering the read function.
        table_marker = "\nstatic const drivergen_eval_channel_t g_channels"
        if class_block.startswith(table_marker):
            end = class_block.find("};\n", len(table_marker)) + len("};\n")
            channels_table = class_block[:end]
            rest = class_block[end:]
            parts.append(channels_table)
            parts.append(_META_TEMPLATE.format(**params))
            parts.append(init_block)
            parts.append(rest)
        else:  # defensive; should never hit
            parts.append(_META_TEMPLATE.format(**params))
            parts.append(init_block)
            parts.append(class_block)
    else:
        parts.append(_META_TEMPLATE.format(**params))
        parts.append(init_block)
        parts.append(class_block)

    parts.append(_CLEANUP_STUB)

    source = "".join(parts)
    # Collapse accidental triple-newlines for tidy diffs.
    source = re.sub(r"\n{3,}", "\n\n", source)

    return GeneratedAdapter(
        device_id=dev,
        eval_class=eval_class,
        bus_kind=classify_result.bus_type,
        source_c=source,
        warnings=tuple(warnings_out),
        params=dict(params),
    )


__all__ = [
    "API_CONTRACT_SCHEMAS",
    "AdapterContractError",
    "GeneratedAdapter",
    "generate_adapter",
]
