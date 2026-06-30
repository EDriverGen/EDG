"""SYNTHESIS prompt rules and mechanical violation checks."""
from __future__ import annotations

import json
import re
from typing import Any, Iterable, Mapping, Sequence, Tuple


# ---------------------------------------------------------------------------
# Prompt constants
# ---------------------------------------------------------------------------

ROLE = (
    "You are a senior embedded-systems engineer producing one-shot driver "
    "bundles. You read a structured task description (Sections A..G) and "
    "emit a single JSON object describing four artefacts: a header file, a "
    "source file, an API contract, and a test plan. You never invent new "
    "RTOS / HAL APIs or new bus handles — you reuse the symbols Section C "
    "lists, exactly as listed."
)

OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. Top-level keys are: \"driver_header\" (string), "
    "\"driver_source\" (string), \"api_contract\" (object), "
    "\"test_plan\" (object). UTF-8 only; no base64. The header and source "
    "strings contain real C, terminated by a newline; do NOT pretty-print "
    "with leading line numbers or markdown bullets. Every required field "
    "of the four artefacts must be populated; empty strings, TODO "
    "placeholders, or ellipses count as schema violations."
)

HARD_RULES: Tuple[str, ...] = (
    # Rule 1 - header allow-list.
    'Hard rule 1 — header allow-list: Every "#include" in driver_header / '
    'driver_source MUST come from one of: '
    '(a) a header explicitly listed in Section C\'s "include:" lines '
    '(use the basename verbatim); '
    '(b) a C standard library header (e.g. <stdint.h>, <stddef.h>, '
    '<string.h>); '
    '(c) the driver\'s own header (e.g. "<device>.h" included from '
    'driver_source). '
    'Counter-example: including a vendor HAL bus header or low-level bus '
    'header that Section C did not enumerate (wrong — those '
    'are not on the allow-list and the stub-compile sandbox will fail '
    'with "fatal error: file not found").',

    # Rule 2 - pre-initialized bus handle.
    'Hard rule 2 — bus already initialised: The bus handle (hi2c1, hspi1, '
    '...) is pre-initialised by the task-package helper in Section C. '
    'Driver / adapter code MUST NOT touch peripheral macros (I2C1, SPI1, '
    'RCC_*, LL_*, __HAL_RCC_*) and MUST NOT call HAL_I2C_Init / '
    'HAL_SPI_Init. Reuse the handle as opaque context only. '
    'Counter-example: calling __HAL_RCC_I2C1_CLK_ENABLE() or '
    'HAL_I2C_Init(&hi2c1) from inside the driver (wrong — that is a '
    'duplicate init, breaks the kernel-side bus state, and is detected '
    'by the post-build symbol scan).',

    # Rule 3 - Section E is a lower bound.
    'Hard rule 3 — Section E is a lower bound: Your driver\'s actual bus '
    'transactions, and the entries in test_plan.expected_transactions, '
    'MUST be a superset of every line in Section E. Preserve the '
    'phase+addr_or_pin+write_prefix_any_of semantics. You MAY add extras '
    '(per-channel readbacks, sanity reads), but you MUST NOT drop any '
    'listed prefix. Counter-example: Section E lists a write-then-read '
    'on 0x48 with prefix [0x00] but the test_plan only emits a bare read '
    '(wrong — the pointer-write phase is missing).',

    # Rule 4 - integer-only source and matching expected values.
    'Hard rule 4 — driver_source is integer-only AND expected_* values '
    'must use the exact same formula: The driver MUST NOT declare "float" '
    'or "double" variables, MUST NOT cast to/from those types, and MUST NOT '
    'contain floating-point literals (e.g. 0.125, 1.5e-3, 25.0f). The IR '
    'already provides milli/micro-unit integer approximations under '
    'conversion_formulae.integer_approximation_expression — translate those '
    'expressions verbatim into C integer arithmetic (shifts, multiplies, '
    'divides) AND apply the SAME expression to compute test_plan expected_* '
    'values from mock_preload bytes. For example, if the IR says '
    '"raw * 625 / 10" for milli_degC output and mock_preload bytes yield '
    'raw=400, then expected_read_raw MUST be 400 * 625 / 10 = 25000, not '
    '400 and not 25. The test_plan expected value and the driver output '
    'must both use the same formula in the same public unit. '
    'Counter-example: '
    'returning "(float)(raw >> 5) * 0.125f" as the temperature path '
    '(wrong — the generation-side runtime probe compares integer outputs; float drift '
    'breaks the millideg comparison and the embedded toolchain may '
    'lack a hardware FPU).',

    # Rule 5 - api_contract and driver_source symbols must match.
    'Hard rule 5 — contract symbols must exist in driver_source: The C '
    'function names referenced inside api_contract.init_call, '
    'api_contract.read_call, and api_contract.channels[*].read_call '
    '(the bare identifier preceding "(") MUST be defined as functions '
    'in driver_source — not just declared in driver_header. The adapter '
    'generator pastes those expressions verbatim into '
    '<device>_eval_adapter.c; an undefined name produces an '
    'undefined-reference link error. Counter-example: setting '
    'init_call="sensor_init(&g_eval_dev, bus_name)" while driver_source '
    'only defines sensor_setup (wrong — the symbol named in the contract '
    'never gets emitted).',

    # Rule 6 - multi-channel devices expose channels[].
    'Hard rule 6 — multi-channel devices expose api_contract.channels[]: '
    'When the device IR declares more than one read_channels entry '
    '(for example multi-axis IMUs or environmental sensors with '
    'temperature/pressure/humidity outputs), api_contract MUST include a '
    '"channels" array with one entry per IR channel. Each entry MUST '
    'follow the per-channel schema given in Section F of the user '
    'prompt — namely { "id": "<snake_case channel id from IR>", '
    '"call": "<func_defined_in_driver_source>(<args>, <out_ptr>)", '
    '"out_arg": "<C identifier of the out parameter local>", '
    '"out_type": "<stdint type of that local>" } — and the IDs MUST '
    'cover every IR channel id (no omissions, no extras). The C '
    'function name preceding "(" inside each channels[*].call MUST be '
    'defined as a function in driver_source (same rule as Hard rule 5 '
    'but per channel). For multi-channel devices the top-level '
    'api_contract.read_call is NOT part of the schema — do not emit '
    'it; rely on channels[*].call as the per-axis entry points. '
    'Single-output devices keep the compact '
    'shape: api_contract.read_call REQUIRED and api_contract.channels '
    'MAY be omitted. Counter-example: a multi-axis sensor with several IR channels '
    'but api_contract={"init_call":"...","read_call":"sensor_read_all('
    '...)"} — wrong, the adapter cannot derive per-axis comparators '
    'and the adapter has no per-channel output to compare. '
    'Counter-example 2: emitting channels=[{"id":"temperature"},'
    '{"id":"pressure"},{"id":"humidity"}] without per-entry "call" / '
    '"out_arg" / "out_type" — wrong, the adapter has no way to spell '
    'the per-axis C call.',

    # Rule 7 - test_plan.test_stimuli must be runnable.
    'Hard rule 7 — test_stimuli must be runnable and carry expected values: '
    'test_plan.test_stimuli MUST contain at least 2 entries (3 when the task '
    'requires physical-unit conversion from a Device IR formula). Each entry '
    'must be a JSON object with a non-empty "name" (unique) and a non-empty '
    '"mock_preload" object. The majority of entries MUST declare a non-null '
    'expected_read_raw, expected_channels, or eval-class-appropriate expected_* '
    'field, computed from mock_preload bytes via the Device IR conversion '
    'formula. Do not leave expected_* as null for normal value-read stimuli. '
    'Every key inside mock_preload '
    'MUST match exactly ONE of these shapes — anything else is silently '
    'dropped by the Renode-side mock and the slave returns 0xFF for '
    'every register, so the driver\'s init / chip-id / status check '
    'fails: '
    '(a) hex register literal such as "0x10" or decimal "16"; '
    '(b) prefixed forms such as "reg_0x10" / "req_0x10" / "resp_0x10"; '
    '(c) addr:reg pair such as "0x50:0x10"; '
    '(d) named sentinels "read_bytes" / "schedule" / "payload" / '
    '"frame_ok" / "frame_err" / "status" / "status_err" / "status_ok" '
    '/ "status_zeros". '
    'Values MUST be either an array of integer / hex-string bytes '
    '(e.g. [0x60] or ["0x60"]) or a JSON-encoded array literal '
    '(e.g. "[0x60, 0x00]"); GPIO schedule values are arrays of '
    '[level, delay_us] pairs. '
    'Counter-example: emitting '
    '{ "name": "boot", "mock_preload": { "chip_id": [0x60], '
    '"status_sequence": [0x80, 0x00] } } '
    '— wrong, both keys are descriptive prose and get dropped; the '
    'mock returns 0xFF for the requested register and the driver status check '
    'fails. Correct shape: '
    '{ "name": "boot", "mock_preload": { "0x10": [0x12], '
    '"reg_0x11": [0x00] } }.',

    # Rule 8 - output locals must match adapter-declared names.
    'Hard rule 8 — output locals in read_call: '
    'The adapter generator declares the result-receiving local for '
    'you and copies it into the harness-visible storage AFTER the '
    'call returns. Your call expression MUST use that exact name as '
    'the address-of output argument, otherwise the harness will see '
    '0 even when the call succeeds (and the C compiler may reject '
    'an unrelated identifier as undeclared). '
    'Single-channel: api_contract.read_call MUST contain `&raw` '
    '(adapter declares `<primary_raw_type> raw = 0;` then copies '
    '`raw` into `*out`). '
    'Multi-channel: every api_contract.channels[i].call MUST '
    'contain `&<out_arg>` using the SAME spelling as the '
    '`channels[i].out_arg` field (adapter declares '
    '`<out_type> <out_arg> = 0;` then copies `<out_arg>` into '
    '`g_cached[i]`). '
    'Counter-example A (single-channel output-local mismatch): '
    'read_call="sensor_read_value(&g_eval_dev, &temperature_mdegC)" '
    '— wrong, the adapter expects `&raw`; this either fails to '
    'compile (`temperature_mdegC` undeclared) or, when paired with '
    'a stray `static int16_t temperature_mdegC = 0;` in '
    'init_extra_setup_c, silently publishes 0. '
    'Correct: read_call="sensor_read_value(&g_eval_dev, &raw)". '
    'Counter-example B (multi-channel spelling drift): '
    'channels[0]={"id":"channel_x","out_arg":"cx","call":"sensor_read_channel_x(&g_eval_dev, &channel_x)"} '
    '— wrong, `out_arg` says `cx` but the call writes into '
    '`channel_x`; the adapter still declares `int16_t cx = 0;` and '
    'caches `cx`, so g_cached[0] is always 0. Correct: '
    'either rename out_arg to "channel_x" or change the call to '
    '`sensor_read_channel_x(&g_eval_dev, &cx)`.',

    # Rule 9 - address selection.
    'Hard rule 9 — address selection (default vs broadcast): '
    '(a) PER-DEVICE COMMANDS — when Section B\'s "I2C Addresses" list '
    'contains an entry tagged "**...— default**" the driver MUST use '
    'that 7-bit address verbatim, both as the C constant '
    '(`#define <DEV>_I2C_ADDR 0x<addr>` and any matching '
    '`dev->i2c_addr = ...` initialiser) and as the value used inside '
    '`api_contract.expected_transactions[*].addr_or_pin` and any '
    'driver-side bus call that targets this device. Datasheets '
    'routinely show 8-bit read/write addresses (e.g. abstract '
    '"0x84/0x85") next to the 7-bit form (0x42 = 0x84 >> 1) — IR '
    'rendering gives the 7-bit form and that is the only one to encode. '
    'If you genuinely need to support a non-default strap-pin variant, '
    'expose it through a runtime parameter '
    '(`<dev>_init_with_address(...)`) and still default to the '
    'IR-default constant.\n'
    '(b) BUS-WIDE / GENERAL-CALL COMMANDS — when an entry of '
    '`registers_or_commands` is described in Section B as an I2C '
    'general-call, broadcast, or "all devices" command (datasheet '
    'phrases such as "general call reset", "broadcast soft reset", '
    '"I2C general call address"), the bus call sending that command '
    'MUST target the I2C general-call address 0x00, NOT the device\'s '
    '7-bit slave address from (a). The IR ships only the command bytes; '
    'the broadcast semantics live in the command\'s `description` text '
    '— read it before you decide which address to put on the wire. '
    'Per-device reads/writes around the broadcast still use the '
    'IR-default 7-bit address from (a).\n'
    'Counter-example A (alternate strap address): '
    '`#define SENSOR_I2C_ADDR <alternate_7bit_addr>` when Section B marks '
    '`<default_7bit_addr>` as default — wrong, the sandbox board uses the '
    'IR-default address and the alternate address is NACKed. '
    'Counter-example B (8-bit form misuse): '
    '`#define SENSOR_I2C_ADDR <8bit_write_addr>` copied from a datasheet '
    'read/write address pair — wrong, Section B already exposes the 7-bit '
    'wire address and that is the only value to encode. '
    'Counter-example C (broadcast misuse, abstract): a command whose '
    'description reads "general call reset — addresses every device on '
    'the bus" issued via `i2c_write(<dev>->i2c_addr, ...)` (i.e. to the '
    'per-device 7-bit address such as 0x4N) — wrong, the I2C '
    'general-call address 0x00 is the only address every device on the '
    'bus listens to for that frame; sending it to 0x4N either NACKs '
    'because no other device is listening, or, when a register-pointer '
    'mock interprets the first command byte as a register address, '
    'silently overwrites a register and the next read returns '
    'corrupted data. Use `i2c_write(0x00, ...)` for that frame and '
    'continue using the IR-default 7-bit address for every other '
    'transaction.',

    # Rule 10 - driver state struct owns the bus handle.
    'Hard rule 10 — driver state struct owns the bus handle: '
    'The dev / state struct (e.g. `<dev>_device_t`, `struct '
    '<dev>_dev`) MUST contain a field that holds the bus handle '
    'pointer (typed as `I2C_HandleTypeDef *` / `SPI_HandleTypeDef '
    '*` / `UART_HandleTypeDef *` / `void *` / `struct '
    'rt_i2c_bus_device *` / equivalent), and the init function MUST '
    'persist the incoming bus_handle into that field BEFORE any '
    'read / write helper is called. All internal helpers retrieve '
    'the bus from `dev->bus_handle` (or whichever field name the '
    'struct uses); they MUST NOT receive a separate bus parameter '
    'that can be omitted, and they MUST NEVER pass '
    '`NULL` / `(void *)0` / `0` as the bus argument to a HAL or '
    'RTOS bus call. The runtime check has no way to recover '
    'from a NULL handle: the call returns immediately, every read '
    'channel reports -1, and L2 fails the first stimulus. '
    'Counter-example A (lost bus handle): '
    '`typedef struct { uint8_t i2c_addr; } sensor_device_t;` paired '
    'with `int sensor_init(sensor_device_t *dev, void *bus_handle)` '
    'and a helper `sensor_read_raw_all(sensor_device_t *dev, ...)` '
    'whose body issues `HAL_I2C_Mem_Read((I2C_HandleTypeDef *)0, '
    '...)` — wrong, the dev struct lost the bus handle the moment '
    'init returned. Correct shape: '
    '`typedef struct { void *bus_handle; uint8_t i2c_addr; } '
    'sensor_device_t;` plus `dev->bus_handle = bus_handle;` inside '
    '`sensor_init`, then every helper does '
    '`HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, '
    '<dev->i2c_addr << 1>, ...)`. '
    'Counter-example B (drift between init signature and helper '
    'signature): public API exposes `read_temperature(dev, *out)` '
    '(no bus arg) but the internal helper takes `bus_handle` '
    'explicitly, so the read path keeps inventing fresh '
    '`(void *)0` casts to satisfy the compiler — wrong, drop the '
    'helper-level bus parameter and read it from `dev->bus_handle` '
    'instead.',
)

# Top-level shape reminder; Section F remains authoritative.
TOP_LEVEL_SHAPE_HINT = (
    "Top-level shape (full per-key schema is in Section F of the user "
    "prompt — follow that schema as authoritative):\n"
    "{\n"
    "  \"driver_header\":  \"<C header text>\",\n"
    "  \"driver_source\":  \"<C source text>\",\n"
    "  \"api_contract\":   { ...keys defined in Section F... },\n"
    "  \"test_plan\":      { ...keys defined in Section F... }\n"
    "}"
)


# ---------------------------------------------------------------------------
# Mechanical violation checks
# ---------------------------------------------------------------------------

# Tokens that should never appear in generated driver_source.
_BANNED_PERIPHERAL_TOKENS: Tuple[str, ...] = (
    "HAL_I2C_Init",
    "HAL_SPI_Init",
    "HAL_UART_Init",
    "__HAL_RCC_I2C1_CLK_ENABLE",
    "__HAL_RCC_I2C2_CLK_ENABLE",
    "__HAL_RCC_SPI1_CLK_ENABLE",
    "__HAL_RCC_GPIOA_CLK_ENABLE",  # GPIO clk gating — handled by kernel
    "MX_I2C1_Init",
    "MX_SPI1_Init",
    "LL_RCC_",
    "LL_I2C_",
    "LL_SPI_",
)

_INCLUDE_RE = re.compile(
    r'^\s*#\s*include\s*(?:"([^"]+)"|<([^>]+)>)\s*',
    re.MULTILINE,
)

_C_STDLIB_HEADERS = frozenset(
    {
        "stdint.h", "stddef.h", "stdbool.h", "stdio.h", "stdlib.h",
        "string.h", "errno.h", "limits.h", "inttypes.h", "assert.h",
        "math.h", "ctype.h",
    }
)

# Strip comments and literals before scanning C tokens.
_C_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
_C_LINE_COMMENT_RE = re.compile(r"//[^\n]*")
_C_STRING_LITERAL_RE = re.compile(r'"(?:\\.|[^"\\])*"')
_C_CHAR_LITERAL_RE = re.compile(r"'(?:\\.|[^'\\])*'")

# Float literal patterns: dotted/exponent forms with optional suffix.
_FLOAT_LITERAL_RE = re.compile(
    r"(?:(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|\d+[eE][+-]?\d+)[fFlL]?"
)
_FLOAT_TYPE_DECL_RE = re.compile(r"\b(?:float|double)\b\s+(?![\*])")
_FLOAT_CAST_RE = re.compile(r"\(\s*(?:float|double)\s*\)")

# Mock-preload key shapes accepted by the runtime probe.
_MOCK_PRELOAD_KEY_RE = re.compile(
    r"^("
    r"0x[0-9A-Fa-f]+"
    r"|\d+"
    r"|(reg|req|resp)_0x[0-9A-Fa-f]+"
    r"|0x[0-9A-Fa-f]+:0x[0-9A-Fa-f]+"
    r"|read_bytes|schedule|payload"
    r"|frame_ok|frame_err|status|status_err|status_ok|status_zeros"
    r")$"
)

# Crude C identifier extraction — first non-space token before "(".
_C_CALL_NAME_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(")
# Loose function-definition matcher used to collect emitted function names.
_C_FUNC_DEF_RE = re.compile(
    r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{",
    re.MULTILINE,
)

# HAL bus-handle naming convention.
_HAL_BUS_HANDLES: Tuple[Tuple[re.Pattern[str], str], ...] = (
    (re.compile(r"\bhi2c\d+\b"),  "I2C_HandleTypeDef"),
    (re.compile(r"\bhspi\d+\b"),  "SPI_HandleTypeDef"),
    (re.compile(r"\bhuart\d+\b"), "UART_HandleTypeDef"),
    (re.compile(r"\bhusart\d+\b"),"USART_HandleTypeDef"),
    (re.compile(r"\bhcan\d+\b"),  "CAN_HandleTypeDef"),
)

# POSIX errno-style constants that require an explicit errno include.
_POSIX_ERRNO_NAMES: frozenset[str] = frozenset({
    "ENODEV", "EIO", "EINVAL", "EBUSY", "ETIMEDOUT", "EAGAIN",
    "ENOMEM", "EPERM", "ENOENT", "EACCES", "EFAULT", "ENOSPC",
    "EROFS", "EPROTO", "ENOTSUP", "EOPNOTSUPP", "EOVERFLOW",
    "ECONNRESET", "ENOSYS", "EINTR", "EBADF", "EILSEQ",
    "EWOULDBLOCK", "ENOTCONN", "EMSGSIZE", "ENOEXEC", "ENXIO",
    "ENODATA", "EPIPE", "ERANGE", "EDOM",
})

# Match isolated errno-like identifiers.
_ERRNO_TOKEN_RE = re.compile(r"-?\b([A-Z][A-Z0-9_]+)\b")

# Accepted forms of an errno include line, evaluated against `preamble_c`.
_ERRNO_INCLUDE_RE = re.compile(
    r'#\s*include\s*[<"](?:[^>"]*/)?errno\.h[>"]'
)

# Adapter ABI tokens must not leak into driver translation units.
_ADAPTER_ABI_TOKEN_RE = re.compile(r"\bDRIVERGEN_EVAL_[A-Z0-9_]+\b")


# ---------------------------------------------------------------------------
# Rule 10 — driver state struct owns the bus handle
# ---------------------------------------------------------------------------

# Names of types recognized as bus-handle types.
_BUS_HANDLE_TYPE_NAMES: Tuple[str, ...] = (
    "I2C_HandleTypeDef",
    "SPI_HandleTypeDef",
    "UART_HandleTypeDef",
    "USART_HandleTypeDef",
    "CAN_HandleTypeDef",
    "rt_i2c_bus_device",
    "rt_spi_device",
    "i2c_master_bus_handle_t",
    "void",
)

# Field names that look like they hold the bus handle.
_BUS_FIELD_NAMES: Tuple[str, ...] = (
    "bus", "bus_handle", "bus_ctx", "i2c", "i2c_handle", "i2c_bus",
    "spi", "spi_handle", "uart", "uart_handle",
    "hi2c", "hi2c1", "hi2c2", "hi2c3", "hi2c4",
    "hspi", "hspi1", "hspi2", "hspi3",
    "huart", "huart1", "huart2", "huart3",
    "handle",
)

# Match typedef struct blocks.
_TYPEDEF_STRUCT_RE = re.compile(
    r"typedef\s+struct(?:\s+\w+)?\s*\{(?P<body>[^{}]*)\}\s*"
    r"(?P<typename>\w+)\s*;",
    re.DOTALL,
)

# Match tagged struct blocks.
_STRUCT_TAG_RE = re.compile(
    r"struct\s+(?P<tag>\w+)\s*\{(?P<body>[^{}]*)\}\s*;",
    re.DOTALL,
)

# Build the bus-field detector from the configured type/name lists.
def _build_bus_field_regex() -> re.Pattern[str]:
    type_alt = "|".join(re.escape(t) for t in _BUS_HANDLE_TYPE_NAMES)
    name_alt = "|".join(re.escape(n) for n in _BUS_FIELD_NAMES)
    # Accept recognized bus-handle pointer fields and common typedef pointers.
    return re.compile(
        rf"(?:\b(?:{type_alt})\b|\bstruct\s+\w+\b|\b\w+_t\b|\b\w+TypeDef\b)"
        rf"\s*\*\s*(?:{name_alt})\b",
        re.IGNORECASE,
    )


_STRUCT_BUS_FIELD_RE = _build_bus_field_regex()

# Permissive fallback for typed bus-handle pointer fields.
_STRUCT_TYPED_BUS_FIELD_RE = re.compile(
    r"\b(?:" + "|".join(re.escape(t) for t in _BUS_HANDLE_TYPE_NAMES) + r")"
    r"\b\s*\*\s*\w+\s*;",
)

# Detect HAL bus calls whose first argument is a NULL-shaped expression.
_NULL_FIRST_ARG_HAL_RE = re.compile(
    r"\b(HAL_(?:I2C|SPI|UART)_(?:Master_Transmit|Master_Receive|"
    r"Mem_Read|Mem_Write|Transmit|Receive|TransmitReceive))"
    r"\s*\(\s*"
    r"(NULL|0|\(\s*(?:void|I2C_HandleTypeDef|SPI_HandleTypeDef|"
    r"UART_HandleTypeDef)\s*\*\s*\)\s*0)"
    r"\s*,",
)

# Detect NULL bus-handle casts used as function-call arguments.
_NULL_BUS_CAST_IN_ARG_RE = re.compile(
    r"[(,]\s*"
    r"\(\s*(?:void|I2C_HandleTypeDef|SPI_HandleTypeDef|"
    r"UART_HandleTypeDef|USART_HandleTypeDef|CAN_HandleTypeDef)"
    r"\s*\*\s*\)\s*0\s*[,)]"
)

# Detect NULL guards that mask an earlier NULL bus-handle call.
_NULL_GUARD_PARAM_RE = re.compile(
    r"\bif\s*\(\s*bus_handle\s*==\s*NULL\s*\)"
)


def _extract_struct_bodies(text: str) -> list[tuple[str, str]]:
    """Return ``[(typename_or_tag, body_text), ...]`` for every ``typedef struct {...} <name>;`` or ``struct <tag> {...};`` in ``text``."""
    if not text:
        return []
    scrubbed = _C_BLOCK_COMMENT_RE.sub(" ", text)
    scrubbed = _C_LINE_COMMENT_RE.sub(" ", scrubbed)
    scrubbed = _C_STRING_LITERAL_RE.sub('""', scrubbed)
    out: list[tuple[str, str]] = []
    for m in _TYPEDEF_STRUCT_RE.finditer(scrubbed):
        out.append((m.group("typename"), m.group("body")))
    for m in _STRUCT_TAG_RE.finditer(scrubbed):
        out.append((m.group("tag"), m.group("body")))
    return out


def _struct_has_bus_field(body: str) -> bool:
    """True iff the struct body declares a field that looks like it holds a bus handle."""
    if not body:
        return False
    if _STRUCT_BUS_FIELD_RE.search(body):
        return True
    # Permissive fallback: any field whose NAME is in the bus-field
    # list, regardless of declared type.
    name_alt = "|".join(re.escape(n) for n in _BUS_FIELD_NAMES)
    fallback = re.compile(
        rf"\*\s*(?:{name_alt})\b\s*;",
        re.IGNORECASE,
    )
    if fallback.search(body):
        return True
    # Last-resort: a typed bus handle field with ANY name.
    if _STRUCT_TYPED_BUS_FIELD_RE.search(body):
        return True
    return False


def _check_bus_handle_propagation(
    *,
    header: str,
    source: str,
    device_id: str,
) -> list[str]:
    """Ensure the driver state struct owns the bus handle."""
    flagged: list[str] = []

    scrubbed_source = ""
    if source:
        scrubbed_source = _C_BLOCK_COMMENT_RE.sub(" ", source)
        scrubbed_source = _C_LINE_COMMENT_RE.sub(" ", scrubbed_source)
        scrubbed_source = _C_STRING_LITERAL_RE.sub('""', scrubbed_source)
        scrubbed_source = _C_CHAR_LITERAL_RE.sub("' '", scrubbed_source)

    guard_present = (
        bool(_NULL_GUARD_PARAM_RE.search(scrubbed_source))
        if scrubbed_source else False
    )
    guard_hint = (
        " The accompanying `if (bus_handle == NULL) return -1;` "
        "guard is not a fix — it just hides the broken call site "
        "behind a returned -1, which the runtime check sees as "
        "a hard read failure."
        if guard_present else ""
    )

    null_call_match = (
        _NULL_FIRST_ARG_HAL_RE.search(scrubbed_source)
        if scrubbed_source else None
    )
    if null_call_match is not None:
        func_name = null_call_match.group(1)
        bad_arg = null_call_match.group(2)
        flagged.append(
            f"driver_source: {func_name} is invoked with "
            f"{bad_arg!r} as its bus-handle argument. The bus is the "
            "first parameter of every HAL_I2C_*/HAL_SPI_*/HAL_UART_* "
            "call; passing NULL means the read path never reaches the "
            "wire. Store the incoming bus handle into the dev struct "
            "during init "
            "(`dev->bus_handle = bus_handle;`) and dereference it via "
            "`(I2C_HandleTypeDef *)dev->bus_handle` (or the matching "
            "type) inside helpers." + guard_hint
        )

    if scrubbed_source and _NULL_BUS_CAST_IN_ARG_RE.search(scrubbed_source):
        # Only emit the indirect-call diagnostic if the direct-HAL one
        # did not already fire — they'd be duplicates otherwise.
        if null_call_match is None:
            flagged.append(
                "driver_source: passes a NULL bus-handle cast (e.g. "
                "`(void *)0`, `(I2C_HandleTypeDef *)0`) to a helper "
                "function. Even if the helper is a private wrapper, "
                "the next layer down ultimately hands that NULL to "
                "HAL_I2C_*/HAL_SPI_*/HAL_UART_* and the read path "
                "fails before it reaches the wire. The dev struct "
                "must own the bus handle: store it in init "
                "(`dev->bus_handle = bus_handle;`), drop the "
                "redundant `bus_handle` parameter from internal "
                "helpers, and pass `dev->bus_handle` (cast to the "
                "right HAL type at the call site)." + guard_hint
            )

    # Pick struct candidates from header first, then source.
    candidates: list[tuple[str, str]] = []
    candidates.extend(_extract_struct_bodies(header))
    candidates.extend(_extract_struct_bodies(source))
    if not candidates:
        return flagged

    device_token = (device_id or "").strip().lower()
    dev_structs = [
        (name, body)
        for name, body in candidates
        if (
            (device_token and device_token in name.lower())
            or "_dev" in name.lower()
            or "_device_t" in name.lower()
            or name.lower().endswith("_t")
        )
    ]
    if not dev_structs:
        return flagged

    if any(_struct_has_bus_field(body) for _, body in dev_structs):
        return flagged

    primary_name = dev_structs[0][0]
    flagged.append(
        f"driver_header / driver_source: dev struct {primary_name!r} "
        "does not declare a field that holds the bus handle "
        "(expected something like `void *bus_handle;` or "
        "`I2C_HandleTypeDef *hi2c;`). Without that field the read "
        "helpers cannot reach the bus, and generated code is forced to "
        "fabricate `(void *)0` casts at every call site. Add the "
        "field to the struct and persist the incoming bus_handle "
        "via `dev->bus_handle = bus_handle;` inside the init "
        "function — see Hard rule 10 in the system prompt."
    )
    return flagged


def detect_violations(
    output: Any,
    *,
    allowed_headers: Iterable[str] = (),
    expected_transactions: Sequence[Mapping[str, Any]] = (),
    device_id: str = "",
    channel_ids: Sequence[str] = (),
    address_rule: Mapping[str, Any] | None = None,
) -> list[str]:
    """Run mechanical checks against synthesis output."""
    if not isinstance(output, Mapping):
        return [
            f"top-level output must be a JSON object (got "
            f"{type(output).__name__})"
        ]
    violations: list[str] = []

    for key in ("driver_header", "driver_source", "api_contract", "test_plan"):
        if key not in output:
            violations.append(f"missing required top-level key {key!r}")

    header = output.get("driver_header")
    source = output.get("driver_source")
    if isinstance(header, str) and not header.strip():
        violations.append("driver_header: empty string")
    if isinstance(source, str) and not source.strip():
        violations.append("driver_source: empty string")

    # Rule 1 - header allow-list.
    allowed = _normalise_header_set(allowed_headers, device_id=device_id)
    for label, body in (("driver_header", header), ("driver_source", source)):
        if isinstance(body, str):
            for include in _extract_includes(body):
                base = include.rsplit("/", 1)[-1]
                if base in _C_STDLIB_HEADERS:
                    continue
                if base in allowed:
                    continue
                violations.append(
                    f"{label}: '#include {include!r}' is not on Section C's "
                    f"allow-list "
                    f"(allowed: {sorted(allowed) or '<none>'} + C stdlib)"
                )

    # Adapter ABI symbols must not leak into the driver translation unit.
    violations.extend(
        _check_no_abi_symbols_in_driver(
            header=header if isinstance(header, str) else "",
            source=source if isinstance(source, str) else "",
        )
    )

    # Rule 2 - peripheral / re-init tokens.
    if isinstance(source, str):
        for token in _BANNED_PERIPHERAL_TOKENS:
            if token in source:
                violations.append(
                    f"driver_source contains banned token {token!r} — "
                    "the bus is already initialised; reuse the handle "
                    "passed in via Section C"
                )

    # Rule 3 - Section E superset.
    test_plan = output.get("test_plan")
    if isinstance(test_plan, Mapping):
        emitted = test_plan.get("expected_transactions") or []
        if not isinstance(emitted, list):
            violations.append(
                "test_plan.expected_transactions: must be a list of "
                "transaction objects"
            )
        else:
            for i, expected in enumerate(expected_transactions):
                if not _has_matching_transaction(expected, emitted):
                    violations.append(
                        f"test_plan.expected_transactions: missing entry "
                        f"matching Section E[{i}] "
                        f"(phase={expected.get('phase')!r}, "
                        f"addr_or_pin={expected.get('addr_or_pin')!r})"
                    )

    # Rule 4 - driver_source must stay integer-only.
    if isinstance(source, str):
        violations.extend(_check_integer_only(source))

    # Rules 5/6 - contract symbols and channel layout.
    api_contract = output.get("api_contract")
    is_multi_channel = len(tuple(channel_ids)) >= 2
    if isinstance(api_contract, Mapping) and isinstance(source, str):
        violations.extend(
            _check_contract_symbols(
                api_contract, source, multi_channel=is_multi_channel,
            )
        )
        violations.extend(
            _check_channels_for_multichannel(
                api_contract, source, channel_ids,
            )
        )
        # init_call bus-handle shape.
        violations.extend(
            _check_init_call_shape(api_contract, source, header)
        )
        # read_call / channels[*].call output locals.
        violations.extend(_check_read_call_out_arg(api_contract))
        # errno-style constants need a matching include.
        violations.extend(_check_no_unincluded_errno(api_contract))

    # Rule 7 - test_stimuli runnable shape.
    if isinstance(test_plan, Mapping):
        violations.extend(
        _check_test_stimuli(test_plan, api_contract)
    )

    # Rule 9 - address selection.
    if address_rule is not None:
        violations.extend(
            _check_address_default_used(
                api_contract if isinstance(api_contract, Mapping) else None,
                source if isinstance(source, str) else "",
                header if isinstance(header, str) else "",
                test_plan if isinstance(test_plan, Mapping) else None,
                address_rule,
                device_id=device_id,
            )
        )

    # Rule 10 - driver state struct owns the bus handle.
    violations.extend(
        _check_bus_handle_propagation(
            header=header if isinstance(header, str) else "",
            source=source if isinstance(source, str) else "",
            device_id=device_id,
        )
    )

    return violations


def _check_integer_only(source: str) -> list[str]:
    """Flag float types / casts / literals in driver_source."""
    code = _C_BLOCK_COMMENT_RE.sub(" ", source)
    code = _C_LINE_COMMENT_RE.sub(" ", code)
    code = _C_STRING_LITERAL_RE.sub('""', code)
    code = _C_CHAR_LITERAL_RE.sub("' '", code)

    flagged: list[str] = []
    if _FLOAT_TYPE_DECL_RE.search(code):
        flagged.append(
            "driver_source declares a float/double variable — the "
            "runtime probe compares integer outputs; translate the IR's "
            "integer_approximation_expression into C integer arithmetic "
            "instead"
        )
    if _FLOAT_CAST_RE.search(code):
        flagged.append(
            "driver_source contains a (float)/(double) cast — drop the "
            "cast and stay in integer arithmetic"
        )
    float_lits = sorted({m.group(0) for m in _FLOAT_LITERAL_RE.finditer(code)})
    if float_lits:
        flagged.append(
            "driver_source contains floating-point literal(s) "
            f"{float_lits!r} — replace with milli/micro-unit integer "
            "constants (e.g. 0.125 -> *125 / 1000)"
        )
    return flagged


def _check_contract_symbols(
    api_contract: Mapping[str, Any],
    source: str,
    *,
    multi_channel: bool = False,
) -> list[str]:
    """Ensure init_call / read_call function names are defined in source."""
    flagged: list[str] = []
    defined = {m.group(1) for m in _C_FUNC_DEF_RE.finditer(source)}
    for field in ("init_call", "read_call"):
        call = api_contract.get(field)
        if not isinstance(call, str) or not call.strip():
            if field == "read_call" and multi_channel:
                continue
            continue
        m = _C_CALL_NAME_RE.match(call)
        if not m:
            flagged.append(
                f"api_contract.{field}: cannot extract the leading C "
                f"identifier from {call!r} — use the form "
                f"'<func_name>(...)'"
            )
            continue
        name = m.group(1)
        if name not in defined:
            flagged.append(
                f"api_contract.{field}: function {name!r} is not defined "
                f"in driver_source — the adapter pastes this expression "
                f"verbatim into <device>_eval_adapter.c so the symbol "
                f"must exist as a real definition (not just a header "
                f"declaration)"
            )
    return flagged


def _split_top_level_args(args_text: str) -> list[str]:
    """Split a C call argument list into top-level args."""
    out: list[str] = []
    depth = 0
    buf: list[str] = []
    for ch in args_text:
        if ch == "(":
            depth += 1
            buf.append(ch)
        elif ch == ")":
            depth -= 1
            buf.append(ch)
        elif ch == "," and depth == 0:
            out.append("".join(buf).strip())
            buf = []
        else:
            buf.append(ch)
    tail = "".join(buf).strip()
    if tail or out:
        out.append(tail)
    return [a for a in out if a]


_INIT_FUNC_DECL_RE_TMPL = (
    r"\b{name}\s*\(([^)]*)\)\s*[;{{]"
)


def _find_init_second_arg_type(
    body: str, init_func_name: str,
) -> str | None:
    """Return the spelling of the SECOND parameter type in the init declaration / definition, or ``None`` if not found."""
    pat = re.compile(_INIT_FUNC_DECL_RE_TMPL.format(name=re.escape(init_func_name)))
    m = pat.search(body)
    if not m:
        return None
    args = _split_top_level_args(m.group(1))
    if len(args) < 2:
        return None
    # Strip trailing identifier and keep type tokens.
    second = args[1]
    id_match = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*$", second)
    if id_match is None:
        return second.strip()
    type_part = second[: id_match.start()].rstrip()
    if type_part:
        return type_part
    # Fall back for declarations without leading type tokens.
    return second.strip()


def _check_init_call_shape(
    api_contract: Mapping[str, Any],
    source: str,
    header: Any,
) -> list[str]:
    """Statically catch bus-handle mistakes in ``api_contract.init_call``."""
    flagged: list[str] = []

    init_call = api_contract.get("init_call")
    if not isinstance(init_call, str) or not init_call.strip():
        return flagged

    name_match = _C_CALL_NAME_RE.match(init_call)
    if name_match is None:
        return flagged
    init_func_name = name_match.group(1)
    paren_open = init_call.find("(", name_match.end() - 1)
    paren_close = init_call.rfind(")")
    if paren_open < 0 or paren_close < 0 or paren_close <= paren_open:
        return flagged
    args = _split_top_level_args(init_call[paren_open + 1 : paren_close])

    second_arg = args[1] if len(args) >= 2 else None

    # Check 0 - init_call must have exactly 2 positional args.
    if len(args) > 2:
        flagged.append(
            f"api_contract.init_call: has {len(args)} positional "
            "arguments; the adapter template requires exactly 2 "
            "args (dev_ptr, bus_handle). Move extra parameters "
            "(address, config, etc.) into #define constants or the "
            "driver init body."
        )

    # Check 0b - init function must return int.
    _hdr = header if isinstance(header, str) else ""
    _body = (_hdr or "") + "\n" + (source or "")
    init_decl = re.search(
        r"\b" + re.escape(init_func_name) + r"\s*\([^)]*\)",
        _body,
    )
    if init_decl:
        decl_text = _body[max(0, init_decl.start() - 50):init_decl.start()]
        if re.search(r"\bvoid\b\s*$", decl_text):
            flagged.append(
                f"api_contract.init_call: {init_func_name!r} returns "
                "void, but the adapter casts init_call to (int). "
                "Use 'int' return type (0 on success, negative on error)."
            )

    preamble = api_contract.get("preamble_c") or ""
    init_extra = api_contract.get("init_extra_setup_c") or ""
    if not isinstance(preamble, str):
        preamble = ""
    if not isinstance(init_extra, str):
        init_extra = ""

    # ------------------------------------------------------------------
    # Check 1 — bare ``bus_name`` passed as pointer
    # ------------------------------------------------------------------
    if second_arg == "bus_name":
        flagged.append(
            "api_contract.init_call: second argument is the bare "
            "'bus_name' parameter, which is `const char *`. The "
            "driver's init expects a typed bus pointer. Resolve a "
            "properly typed handle in `init_extra_setup_c` first — "
            "e.g. `I2C_HandleTypeDef *bus_handle = "
            "(I2C_HandleTypeDef *)bus_name;` for HAL-style opaque "
            "pointer harnesses, `rt_i2c_bus_device_find(bus_name)` "
            "for bus lookup helpers, or a typed lookup/cast for device "
            "handles — and pass the resolved handle instead "
            "of `bus_name`."
        )

    # ------------------------------------------------------------------
    # Check 2 - HAL globals redeclared in generated C strings
    # ------------------------------------------------------------------
    redecl_blob = preamble + "\n" + init_extra
    for handle_re, type_name in _HAL_BUS_HANDLES:
        for handle in handle_re.findall(redecl_blob):
            redecl_pat = re.compile(
                r"\b(?:extern|static)\b[^;\n]*\b"
                + re.escape(type_name)
                + r"\b[^;\n]*\b"
                + re.escape(handle)
                + r"\b[^;\n]*;"
            )
            if redecl_pat.search(redecl_blob):
                flagged.append(
                    f"api_contract: redeclares HAL handle "
                    f"{handle!r} (type {type_name}) in preamble_c "
                    "or init_extra_setup_c. The adapter generator "
                    "auto-injects `extern " + type_name + " "
                    + handle + ";` whenever your init_call mentions "
                    "the handle, so a second declaration triggers a "
                    "C type/redefinition error. Either drop the manual "
                    "declaration and reference the global with `&"
                    + handle + "` in init_call, or switch entirely to "
                    "the portable cast pattern "
                    "`" + type_name + " *bus_handle = ("
                    + type_name + " *)bus_name;`."
                )
                break  # one diagnostic per handle type is enough

    # ------------------------------------------------------------------
    # Check 3 — value passed where the driver init wants a pointer
    # ------------------------------------------------------------------
    if second_arg is not None:
        header_text = header if isinstance(header, str) else ""
        body = (header_text or "") + "\n" + (source or "")
        second_param_type = _find_init_second_arg_type(body, init_func_name)
        if second_param_type and "*" in second_param_type:
            type_name_clean = second_param_type.replace("*", "").strip()
            for handle_re, _hal_type in _HAL_BUS_HANDLES:
                if handle_re.fullmatch(second_arg):
                    flagged.append(
                        "api_contract.init_call: passes the HAL "
                        f"global {second_arg!r} (a value of "
                        f"{_hal_type}) as the second argument, but the "
                        f"driver's `{init_func_name}(...)` declares the "
                        f"second parameter as pointer "
                        f"`{second_param_type}`. Take the address: "
                        f"`{init_func_name}(..., &{second_arg})` — or "
                        "use the portable cast pattern instead."
                    )
                    break
            else:
                # Only flag clear non-pointer locals.
                if (
                    re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", second_arg)
                    and second_arg not in {"bus_name"}
                ):
                    decl_value_pat = re.compile(
                        r"\b" + re.escape(type_name_clean)
                        + r"\s+" + re.escape(second_arg) + r"\b\s*[=;]"
                    )
                    decl_pointer_pat = re.compile(
                        r"\b" + re.escape(type_name_clean)
                        + r"\s*\*\s*" + re.escape(second_arg) + r"\b"
                    )
                    if (
                        decl_value_pat.search(init_extra)
                        and not decl_pointer_pat.search(init_extra)
                    ):
                        flagged.append(
                            f"api_contract.init_call: second argument "
                            f"{second_arg!r} is declared as VALUE "
                            f"{type_name_clean!r} in "
                            "init_extra_setup_c but the driver's "
                            f"`{init_func_name}(...)` second parameter "
                            f"is pointer `{second_param_type}`. Either "
                            "declare the local as a pointer "
                            f"(`{type_name_clean} *{second_arg} = ...;`) "
                            "or pass `&"
                            f"{second_arg}` in init_call."
                        )

    # ------------------------------------------------------------------
    # Check 4 - HAL global tokens in the contract.
    # ------------------------------------------------------------------
    full_blob = init_call + "\n" + preamble + "\n" + init_extra
    for handle_re, type_name in _HAL_BUS_HANDLES:
        handles = sorted(set(handle_re.findall(full_blob)))
        if handles:
            first = handles[0]
            flagged.append(
                f"api_contract: references HAL global "
                f"{first!r} (type {type_name}) in init_call, "
                "preamble_c, or init_extra_setup_c. The stub-compile "
                "sandbox does not link board-generated globals, so any "
                "reference fails with `undefined "
                f"reference to '{first}'`. For HAL-style opaque-pointer "
                f"harnesses use Pattern A: `{type_name} *bus_handle = "
                f"({type_name} *)bus_name;` in init_extra_setup_c, "
                "then pass `bus_handle` (no `&`, it is already a "
                "pointer) as the bus argument of init_call. Pick a "
                "fresh local name — do NOT reuse `hi2c1` / `hspi1` "
                "etc., even as a local pointer, because the adapter "
                "generator's safety net auto-injects `extern "
                f"{type_name} {first};` whenever it spots that token "
                "and you would get a `conflicting types` error."
            )
            break  # one diagnostic per handle family is enough

    return flagged


_ADDR_OF_TOKEN_RE = re.compile(r"&\s*([A-Za-z_][A-Za-z0-9_]*)")


def _addr_of_targets(call: str) -> list[str]:
    """Return every identifier used as `&<ident>` in a call expression."""
    return _ADDR_OF_TOKEN_RE.findall(call)


def _check_read_call_out_arg(
    api_contract: Mapping[str, Any],
) -> list[str]:
    """Enforce the auto-declared output-local naming rule."""
    flagged: list[str] = []
    eval_class = api_contract.get("eval_class")

    if eval_class == "single_channel":
        rc = api_contract.get("read_call")
        if isinstance(rc, str) and rc.strip():
            targets = _addr_of_targets(rc)
            if "raw" not in targets:
                non_dev = [t for t in targets if t != "g_eval_dev"]
                hint = (
                    f" (currently passes &{non_dev[0]!r})"
                    if non_dev else ""
                )
                flagged.append(
                    "api_contract.read_call: missing the adapter-"
                    "supplied output local `&raw`. The adapter "
                    "auto-declares `<primary_raw_type> raw = 0;` and "
                    "copies `raw` into `*out` after your call, so "
                    "`read_call` MUST pass `&raw` as the result "
                    "pointer"
                    + hint
                    + ". Example: "
                    "`<func>(&g_eval_dev, &raw)`."
                )

    elif eval_class == "multi_channel":
        channels = api_contract.get("channels")
        if isinstance(channels, list):
            for idx, entry in enumerate(channels):
                if not isinstance(entry, Mapping):
                    continue
                out_arg = entry.get("out_arg")
                call = entry.get("call")
                if (
                    not isinstance(out_arg, str)
                    or not out_arg.strip()
                    or not isinstance(call, str)
                    or not call.strip()
                ):
                    continue
                targets = _addr_of_targets(call)
                if out_arg not in targets:
                    non_dev = [t for t in targets if t != "g_eval_dev"]
                    hint = (
                        f" (currently passes &{non_dev[0]!r})"
                        if non_dev else ""
                    )
                    flagged.append(
                        f"api_contract.channels[{idx}]: out_arg="
                        f"{out_arg!r} but call={call!r} does not "
                        f"reference `&{out_arg}`"
                        + hint
                        + ". The adapter auto-declares `<out_type> "
                        f"{out_arg} = 0;` and caches `{out_arg}` "
                        f"into `g_cached[{idx}]`, so the call must "
                        f"use that exact spelling as the result "
                        f"pointer."
                    )

    return flagged


def _check_channels_for_multichannel(
    api_contract: Mapping[str, Any],
    source: str,
    channel_ids: Sequence[str],
) -> list[str]:
    """Enforce Rule 6 when the IR declares multiple read_channels."""
    ir_ids = tuple(channel_ids)
    if len(ir_ids) < 2:
        return []

    flagged: list[str] = []
    channels = api_contract.get("channels")
    if channels is None:
        flagged.append(
            "api_contract.channels: required for multi-channel device "
            f"(IR exposes {list(ir_ids)!r}) — emit one entry per IR "
            "channel with { id, call, out_arg, out_type } per the "
            "Section F schema"
        )
        return flagged
    if not isinstance(channels, list):
        flagged.append(
            "api_contract.channels: must be a JSON array of "
            "{ id, call, out_arg, out_type } objects (got "
            f"{type(channels).__name__})"
        )
        return flagged

    seen_ids: list[str] = []
    defined = {m.group(1) for m in _C_FUNC_DEF_RE.finditer(source)}
    for idx, entry in enumerate(channels):
        loc = f"api_contract.channels[{idx}]"
        if not isinstance(entry, Mapping):
            flagged.append(
                f"{loc}: must be a JSON object with 'id' and 'call' "
                f"keys (got {type(entry).__name__})"
            )
            continue
        cid = entry.get("id")
        if not isinstance(cid, str) or not cid.strip():
            flagged.append(f"{loc}.id: missing or empty channel id")
        else:
            seen_ids.append(cid)

        call = entry.get("call")
        if not isinstance(call, str) or not call.strip():
            flagged.append(
                f"{loc}.call: missing — every channel entry needs "
                "its own 'call' string so the adapter can emit per-axis "
                "read paths"
            )
            continue
        m = _C_CALL_NAME_RE.match(call)
        if not m:
            flagged.append(
                f"{loc}.call: cannot extract the leading C "
                f"identifier from {call!r} — use the form "
                f"'<func_name>(...)'"
            )
            continue
        name = m.group(1)
        if name not in defined:
            flagged.append(
                f"{loc}.call: function {name!r} is not defined "
                "in driver_source — the adapter pastes this expression "
                "verbatim into <device>_eval_adapter.c so the symbol "
                "must exist as a real definition"
            )

    seen_set = set(seen_ids)
    ir_set = set(ir_ids)
    missing = sorted(ir_set - seen_set)
    extra = sorted(seen_set - ir_set)
    if missing:
        flagged.append(
            f"api_contract.channels: missing IR channel id(s) {missing!r} "
            f"(IR declared {list(ir_ids)!r}; channels[*].id = "
            f"{seen_ids!r})"
        )
    if extra:
        flagged.append(
            f"api_contract.channels: contains channel id(s) {extra!r} "
            f"that are not in the IR (IR declared {list(ir_ids)!r}) — "
            "do not invent extra channels"
        )
    return flagged


def _check_no_abi_symbols_in_driver(
    *, header: str, source: str,
) -> list[str]:
    """Flag adapter ABI symbols leaked into the driver translation unit."""
    flagged: list[str] = []
    for label, body in (
        ("driver_header", header),
        ("driver_source", source),
    ):
        if not body:
            continue
        scrubbed = _C_BLOCK_COMMENT_RE.sub(" ", body)
        scrubbed = _C_LINE_COMMENT_RE.sub(" ", scrubbed)
        scrubbed = _C_STRING_LITERAL_RE.sub('""', scrubbed)
        scrubbed = _C_CHAR_LITERAL_RE.sub("' '", scrubbed)
        seen: list[str] = []
        for m in _ADAPTER_ABI_TOKEN_RE.finditer(scrubbed):
            token = m.group(0)
            if token not in seen:
                seen.append(token)
        if seen:
            flagged.append(
                f"{label}: references adapter ABI symbol(s) "
                f"{seen!r}. These live in "
                "`drivergen_eval_adapter.h`, which the driver TU "
                "does NOT include — the L1 stub-compile fails with "
                f"`'{seen[0]}' undeclared`. Driver functions MUST "
                "return plain `int` (0 = OK, non-zero = failure); "
                "the auto-generated adapter wraps the result via "
                "`(_rc == 0) ? DRIVERGEN_EVAL_OK : "
                "DRIVERGEN_EVAL_ERR_IO`. The DRIVERGEN_EVAL_* "
                "constants are only safe inside "
                "`api_contract.init_extra_setup_c` / "
                "`api_contract.preamble_c`, which are pasted into "
                "the adapter source file and DO see that header."
            )
    return flagged


def _check_no_unincluded_errno(
    api_contract: Mapping[str, Any],
) -> list[str]:
    """Flag POSIX errno constants used without `<errno.h>`."""
    flagged: list[str] = []

    preamble = api_contract.get("preamble_c") or ""
    init_extra = api_contract.get("init_extra_setup_c") or ""
    if not isinstance(preamble, str):
        preamble = ""
    if not isinstance(init_extra, str):
        init_extra = ""

    has_errno_include = bool(_ERRNO_INCLUDE_RE.search(preamble))
    if has_errno_include:
        return flagged

    for label, body in (
        ("api_contract.init_extra_setup_c", init_extra),
        ("api_contract.preamble_c", preamble),
    ):
        if not body:
            continue
        # Strip C comments and literals before token scanning.
        scrubbed = _C_BLOCK_COMMENT_RE.sub(" ", body)
        scrubbed = _C_LINE_COMMENT_RE.sub(" ", scrubbed)
        scrubbed = _C_STRING_LITERAL_RE.sub('""', scrubbed)
        scrubbed = _C_CHAR_LITERAL_RE.sub("' '", scrubbed)

        seen: list[str] = []
        for m in _ERRNO_TOKEN_RE.finditer(scrubbed):
            token = m.group(1)
            if token in _POSIX_ERRNO_NAMES and token not in seen:
                seen.append(token)
        if seen:
            flagged.append(
                f"{label}: references POSIX errno constant(s) "
                f"{seen!r} but `preamble_c` does not include "
                "`<errno.h>`. The stub-compile sandbox does not "
                "auto-include errno.h, so the adapter fails with "
                f"`'{seen[0]}' undeclared (first use in this "
                "function)`. Either add `#include <errno.h>` to "
                "`preamble_c`, or — preferred — use the adapter "
                "ABI constants from `drivergen_eval_adapter.h` "
                "(`DRIVERGEN_EVAL_ERR_INVALID`, `_IO`, `_NACK`, "
                "`_TIMEOUT`, `_CRC`, `_UNSUPPORTED`) which are "
                "always in scope."
            )

    return flagged


def _check_test_stimuli(
    test_plan: Mapping[str, Any],
    api_contract: Optional[Mapping[str, Any]] = None,
) -> list[str]:
    """Enforce Rule 7 - test_stimuli must be a runnable, distinct shape."""
    flagged: list[str] = []
    stimuli = test_plan.get("test_stimuli")
    if stimuli is None:
        flagged.append(
            "test_plan.test_stimuli: required — emit at least 2 entries "
            "with { name, mock_preload } so the runtime probe has "
            "something to run"
        )
        return flagged
    if not isinstance(stimuli, list):
        flagged.append(
            "test_plan.test_stimuli: must be a JSON array (got "
            f"{type(stimuli).__name__})"
        )
        return flagged
    # Determine the minimum stimulus count from eval_class.
    _eval_for_min = ""
    if isinstance(api_contract, Mapping):
        _eval_for_min = str(api_contract.get("eval_class") or "").strip().lower()
    _min_required = 3 if _eval_for_min in ("single_channel", "multi_channel") else 2

    if len(stimuli) < _min_required:
        _extra = (
            " Physical sensor device test_plans need at least 3 entries "
            "covering positive, zero, and (for signed) negative values."
            if _eval_for_min in ("single_channel", "multi_channel")
            else ""
        )
        flagged.append(
            f"test_plan.test_stimuli: need at least {_min_required} entries, "
            f"got {len(stimuli)}.{_extra}"
        )

    seen_names: dict[str, int] = {}
    for idx, stim in enumerate(stimuli):
        loc = f"test_plan.test_stimuli[{idx}]"
        if not isinstance(stim, Mapping):
            flagged.append(
                f"{loc}: must be a JSON object with 'name' and "
                f"'mock_preload' keys (got {type(stim).__name__})"
            )
            continue

        name = stim.get("name")
        if not isinstance(name, str) or not name.strip():
            flagged.append(
                f"{loc}.name: missing or empty — give the case a short "
                "snake_case label (e.g. 'boot', 'overrange')"
            )
        else:
            seen_names.setdefault(name, idx)
            if seen_names[name] != idx:
                flagged.append(
                    f"{loc}.name: duplicates "
                    f"test_plan.test_stimuli[{seen_names[name]}].name "
                    f"({name!r}) — names must be unique"
                )

        preload = stim.get("mock_preload")
        if not isinstance(preload, Mapping):
            flagged.append(
                f"{loc}.mock_preload: must be a JSON object "
                f"(got {type(preload).__name__}) — keys are register "
                "addresses, values are byte sequences"
            )
            continue
        if not preload:
            flagged.append(
                f"{loc}.mock_preload: empty — list at least one register "
                "(e.g. {'0x10': [0x12]}) so the slave has data to serve"
            )
            continue

        bad_keys = sorted(
            k for k in preload.keys()
            if not (isinstance(k, str) and _MOCK_PRELOAD_KEY_RE.match(k))
        )
        if bad_keys:
            flagged.append(
                f"{loc}.mock_preload: key(s) "
                f"{bad_keys!r} do not match any accepted shape — the "
                "Renode mock will SILENTLY drop them and serve 0xFF "
                "for every register. Use one of: '0x10', '16', "
                "'reg_0x10', 'req_0x10', 'resp_0x10', '0x50:0x10', "
                "or named sentinels (read_bytes / schedule / payload "
                "/ frame_ok / frame_err / status / status_err / "
                "status_ok / status_zeros)"
            )

    # Expected-* coverage for physical sensor classes.
    _eval_cov = ""
    if isinstance(api_contract, Mapping):
        _eval_cov = str(api_contract.get("eval_class") or "").strip().lower()
    if _eval_cov in ("single_channel", "multi_channel"):
        expected_covered = 0
        for _idx, stim in enumerate(stimuli):
            if not isinstance(stim, Mapping):
                continue
            has_expected = bool(
                stim.get("expected_read_raw") is not None
                or stim.get("expected_channels")
                or stim.get("expected_mem_bytes")
                or stim.get("expected_time")
                or stim.get("expected_frame_err") is not None
            )
            if has_expected:
                expected_covered += 1

        total = len(stimuli)
        if total >= 3 and expected_covered < total - 1:
            flagged.append(
                f"test_plan.test_stimuli: only {expected_covered}/{total} entries "
                "have non-null expected_* values; at least N-1 must declare "
                "a computed expected_read_raw or eval-class-appropriate "
                "expected_* field. Normal value-read stimuli must derive "
                "expected_* from mock_preload bytes via the Device IR "
                "conversion formula."
            )
        elif total == 2 and expected_covered < 1:
            flagged.append(
                "test_plan.test_stimuli: at least 1 of the 2 entries must "
                "declare a non-null expected_* value computed from "
                "mock_preload bytes via the Device IR conversion formula."
            )

    return flagged


# ---------------------------------------------------------------------------
# Rule 9 — address selection
# ---------------------------------------------------------------------------

# Lines that look like address constants.
_ADDR_DEFINE_RE = re.compile(
    r"^[ \t]*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(0x[0-9A-Fa-f]+|\d+)",
    re.MULTILINE,
)
# Tokens that mark the macro / field name as an address.
_ADDR_NAME_RE = re.compile(r"(?:_|\b)(addr|address|slave|i2c_addr)(?:_|\b)", re.IGNORECASE)
# Address field initializers.
_ADDR_ASSIGN_RE = re.compile(
    r"\b(?:dev|self|cfg|s)\s*->\s*"
    r"(?P<field>i2c_addr|address|slave_addr|slave_address|addr)\s*=\s*"
    r"(?P<value>0x[0-9A-Fa-f]+|\d+)\s*;",
    re.IGNORECASE,
)


def _check_address_default_used(
    api_contract: Mapping[str, Any] | None,
    source: str,
    header: str,
    test_plan: Mapping[str, Any] | None,
    address_rule: Mapping[str, Any],
    *,
    device_id: str,
) -> list[str]:
    """Enforce Rule 9 - driver MUST use the IR-default 7-bit slave address."""
    flagged: list[str] = []

    addresses = address_rule.get("addresses") if isinstance(address_rule, Mapping) else None
    if not isinstance(addresses, list) or not addresses:
        return flagged

    default_int = _extract_default_address_int(addresses)
    if default_int is None:
        return flagged

    eight_bit_form = (default_int << 1) & 0xFF
    valid_candidates = {default_int}
    valid_eight_bit = {eight_bit_form, (default_int << 1) | 0x01}

    locations: list[tuple[str, str, str]] = []  # (label, identifier, value)

    if header:
        for m in _ADDR_DEFINE_RE.finditer(header):
            name = m.group(1)
            if not _ADDR_NAME_RE.search(name):
                continue
            locations.append(
                ("driver_header", name, m.group(2))
            )
    if source:
        for m in _ADDR_DEFINE_RE.finditer(source):
            name = m.group(1)
            if not _ADDR_NAME_RE.search(name):
                continue
            locations.append(
                ("driver_source", name, m.group(2))
            )
        for m in _ADDR_ASSIGN_RE.finditer(source):
            locations.append(
                ("driver_source", m.group("field"), m.group("value"))
            )

    if not locations:
        return flagged

    canonical_hex = f"0x{default_int:02X}"
    for label, name, value in locations:
        observed = _coerce_int(value)
        if observed is None:
            continue
        if observed in valid_candidates:
            continue
        if observed in valid_eight_bit:
            flagged.append(
                f"{label}: {name} = {value} looks like the 8-bit datasheet "
                f"form (= 0x{observed:02X}) of the canonical 7-bit address "
                f"{canonical_hex}. The IR address rule already exposes the "
                f"7-bit form; encode {canonical_hex} verbatim — see "
                "Hard rule 9 in the system prompt."
            )
            continue
        listed_alt = _matches_listed_alternative(addresses, observed)
        if listed_alt is not None:
            flagged.append(
                f"{label}: {name} = {value} matches an alternative entry "
                f"({listed_alt!r}) instead of the IR-default "
                f"{canonical_hex}. The synthesis sandbox board ties the "
                "strap pins to the silicon default; using the alternative "
                "produces an L2 NACK. Switch to "
                f"{canonical_hex} or expose runtime selection via a "
                "`<dev>_init_with_address(...)` entry point."
            )
            continue
        flagged.append(
            f"{label}: {name} = {value} does not match any address listed "
            f"in `address_rule.addresses` (canonical default is "
            f"{canonical_hex}). Hard-code {canonical_hex} as the I2C slave "
            "address — see Hard rule 9 in the system prompt."
        )

    if not flagged:
        return flagged

    if device_id:
        flagged = [f"[{device_id}] {msg}" for msg in flagged]
    return flagged


def _extract_default_address_int(addresses: Sequence[Any]) -> int | None:
    for entry in addresses:
        if not isinstance(entry, Mapping):
            continue
        if entry.get("is_default") is not True:
            continue
        addr = entry.get("address") or entry.get("value") or entry.get("addr")
        coerced = _coerce_int(addr)
        if coerced is not None:
            return coerced
    return None


def _matches_listed_alternative(
    addresses: Sequence[Any], observed: int
) -> str | None:
    for entry in addresses:
        if not isinstance(entry, Mapping):
            continue
        addr = entry.get("address") or entry.get("value") or entry.get("addr")
        coerced = _coerce_int(addr)
        if coerced is None:
            continue
        if coerced == observed:
            desc = entry.get("description") or entry.get("condition") or ""
            return f"{addr} {desc}".strip()
    return None


def _coerce_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None
        try:
            if text.lower().startswith("0x"):
                return int(text, 16)
            if text.lower().startswith("0b"):
                return int(text, 2)
            return int(text, 0)
        except ValueError:
            return None
    return None


def _normalise_header_set(
    headers: Iterable[str], *, device_id: str
) -> set[str]:
    out: set[str] = set()
    for h in headers:
        if not h:
            continue
        out.add(h.rsplit("/", 1)[-1])
    if device_id:
        out.add(f"{device_id}.h")
    return out


def _extract_includes(body: str) -> list[str]:
    seen: list[str] = []
    for m in _INCLUDE_RE.finditer(body):
        seen.append(m.group(1) or m.group(2))
    return seen


def _has_matching_transaction(
    expected: Mapping[str, Any],
    emitted: Sequence[Any],
) -> bool:
    """A loose superset check."""
    e_phase = expected.get("phase")
    e_addr = expected.get("addr_or_pin")
    for tx in emitted:
        if not isinstance(tx, Mapping):
            continue
        if tx.get("phase") != e_phase:
            continue
        if e_addr is not None and tx.get("addr_or_pin") != e_addr:
            continue
        return True
    return False


# ---------------------------------------------------------------------------
# Prompt builder
# ---------------------------------------------------------------------------

def build_system_prompt() -> str:
    parts: list[str] = [ROLE, OUTPUT_FORMAT]
    parts.extend(HARD_RULES)
    parts.append(TOP_LEVEL_SHAPE_HINT)
    return "\n\n".join(parts)


__all__ = [
    "ROLE",
    "OUTPUT_FORMAT",
    "HARD_RULES",
    "TOP_LEVEL_SHAPE_HINT",
    "detect_violations",
    "build_system_prompt",
]
