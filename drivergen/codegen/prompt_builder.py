"""Prompt builder for driver synthesis."""
from __future__ import annotations

import json
import re
from typing import Any, List, Mapping, Optional, Sequence, Tuple

from drivergen.llm.prompts import synthesis as synthesis_prompt

from .classify_device import ClassifyResult
from .ir_to_expected_transactions import ExpectedTransaction
from .route import RoutingResult, SPI_SUB_COMMAND, SPI_SUB_REGISTER, SPI_SUB_STREAM
from .synthesis_schema import (
    build_driver_code_schema_hint,
    build_plan_schema_hint,
    build_prompt_schema_hint,
)
from .rtos_surface import (
    sanitize_rtos_contract_for_codegen,
    signature_config_pointer_args,
)

# System prompt re-exported for downstream imports.

SYSTEM_PROMPT = synthesis_prompt.build_system_prompt()

_C_STANDARD_INCLUDE_HEADERS = {
    "assert.h",
    "ctype.h",
    "errno.h",
    "float.h",
    "inttypes.h",
    "limits.h",
    "math.h",
    "stdbool.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "time.h",
}

_PLAN_COMMON_RULES = (
    "You are a senior embedded-systems engineer designing the public "
    "adapter contract and generation-side runtime probe plan for one "
    "driver task. Output exactly 1 JSON object with top-level keys "
    "\"api_contract\" and \"test_plan\" only. Do not output driver C code.",
    "The generated test_plan is a generation-side self-test. Use only "
    "the Device IR, platform contract, routing, "
    "and expected bus transactions in the user prompt.",
    "Every test_plan.test_stimuli entry must be runnable and must declare "
    "at least one expected_* field in the public adapter/API output unit. "
    "Do not use intermediate register codes as expected_read_raw when the "
    "api_contract exposes milli/micro/base-converted units.",
    "When SECTION B3 OUTPUT VALUE SEMANTICS is present, it is the sole source of "
    "truth for api_contract units and test_plan expected_* values for every "
    "eval_class including single_channel. If B3 says semantic_kind=physical_scaled "
    "(public_unit plus conversion_required=true), then "
    "api_contract.primary_raw_unit MUST be that public_unit, NOT \"raw_count\", even "
    "though the adapter ABI function is named drivergen_eval_read_raw_i32. The "
    "function name is an adapter-internal detail; the VALUE it returns must be in "
    "the B3 public_unit. Without SECTION B3, eval_class=single_channel uses the "
    "compatibility ABI function name drivergen_eval_read_raw_i32; in that case, "
    "prefer raw/count output when the Device IR provides source bytes and a "
    "separate conversion formula.",
    "At least one non-display test stimulus must be mechanically checkable "
    "before runtime probing: its mock_preload bytes/signals and expected_* value must "
    "line up through an obvious byte-order interpretation or an executable "
    "numeric derivation. Do not leave every numeric expectation unchecked.",
    "The final test_plan must contain only final arithmetic. If you correct "
    "a calculation while drafting, update mock_preload bytes, expected_* "
    "values, and derivation together before output. Do not leave "
    "self-correction text such as 'wait', 'to get', or 'let us fix' in any "
    "derivation.",
    "Each derivation string must be a concise final calculation for the "
    "bytes/signals currently present in mock_preload. Do not include draft "
    "reasoning, alternatives, binary scratch work, recalculation narrative, "
    "or words like wait/recalc/correct/fix. If a negative two's-complement "
    "case is complex, choose simpler bytes or write only the final sign "
    "extension equation.",
    "If you discover that different bytes are needed, rewrite the JSON with "
    "only those final bytes. Never keep old mock_preload bytes while saying "
    "inside derivation that they should be changed, corrected, chosen, or "
    "set to another value.",
    "When SECTION B3 OUTPUT VALUE SEMANTICS is present, api_contract units, "
    "test_plan expected_* values, and generated driver behavior MUST follow "
    "its public output semantics unconditionally — this rule applies to "
    "single_channel eval_class no differently than to any other class. If "
    "semantic_kind is raw_count, return the unconverted register/code value "
    "and set primary_raw_unit=\"raw_count\". If semantic_kind is physical_"
    "scaled or physical_base, apply the referenced Device IR conversion "
    "formula verbatim and use the listed public_unit for primary_raw_unit "
    "and all test_plan expected_* values. There is no single_channel exception.",
    "When the public output unit is scaled, such as milli_* or micro_*, use "
    "`int32_t` for adapter out arguments and channel out_type/read_raw_out_type "
    "unless the Device IR gives explicit numeric bounds proving a narrower "
    "type is safe. Do not use int16_t just because the source register field "
    "is 8 or 16 bits.",
    "When SECTION B3 declares conversion_required=true for any channel, the "
    "test_plan MUST contain at least 3 test_stimuli entries. Cover at least: "
    "(1) a nominal positive value, (2) a zero or near-zero value, and (3) for "
    "signed measurements, a negative two's-complement value. Each of these "
    "entries MUST declare a non-null expected_* field computed from its "
    "mock_preload bytes by applying the Device IR integer_approximation_"
    "expression verbatim. The expected_* "
    "value and the driver output must use the same conversion formula in the "
    "same public unit. Beyond these 3, extra error/fault stimuli with "
    "expected_err are encouraged but not required. When B3 is absent or all "
    "channels have conversion_required=false: include at least 2 runnable "
    "stimuli with unique names and valid mock_preload shapes. For "
    "eval_class=display, covering init + one frame-output stimulus is sufficient.",

)

_PLAN_MULTI_CHANNEL_RULES = (
    "For multi-channel drivers, put public outputs under expected_channels "
    "and include an arithmetic derivation for each expected channel value. "
    "The runtime harness calls every api_contract.channels[*].call for each "
    "stimulus, so each multi-channel stimulus must preload every register or "
    "signal needed by every expected channel it declares. Do not declare a "
    "channel expectation and then say that channel is not read in this "
    "stimulus.",
    "When SECTION B2 CHANNEL CANONICALIZATION is present for a multi-channel "
    "task, api_contract.channels[*].id and test_plan.expected_channels keys "
    "must use its canonical_id values. Treat source_channels and aliases as "
    "evidence for locating the matching IR bytes/functions, not as public "
    "output names.",
    "For multi-channel sensors with split high/low registers, each stimulus "
    "must provide all bytes needed by all channels in accepted mock_preload "
    "register-key form, and every expected_channels value must equal the final "
    "numeric derivation exactly. For fractional encodings, choose the bytes "
    "first and compute the public-unit expected value from those bytes; do not "
    "round to a nicer value or leave a different value in expected_channels.",
)

_PLAN_DISPLAY_RULES = (
    "For eval_class=display, output_frame_call must write/transmit the "
    "adapter-provided frame buffer (`data`/`buf`) and length (`len`) to the "
    "display. Do not model frame output as a read/get/status API.",
)

_PLAN_RTC_RULES = (
    "For eval_class=rtc, time_struct_decl/get_time_call/time_fields must use "
    "one consistent local time variable name. If time_struct_from_in declares "
    "a driver-native time variable, set_time_call must pass that same variable "
    "name (or the adapter `in` object) and must not reference undeclared "
    "placeholders.",
    "For eval_class=rtc, choose exactly one year convention across the adapter "
    "contract and driver. If time_fields.year adds 2000 and "
    "time_struct_from_in subtracts 2000, the driver-native year field is the "
    "device/register offset 0..99; the driver must not add or subtract 2000 "
    "again. If the driver-native struct stores a full Gregorian year, "
    "time_fields.year and time_struct_from_in must pass that year directly.",
)

_PLAN_MEMORY_RULES = (
    "For eval_class=memory, mock_preload keys must be numeric "
    "device-internal byte addresses such as \"0x0000\", each mapped to the "
    "bytes stored at that address. Do not use direct-read sentinel keys such "
    "as \"read_bytes\" for memory devices; expected_mem_bytes must be the "
    "full byte string that the memory harness reads back from the probe "
    "address.",
)

_PLAN_GPIO_RULES = (
    "For GPIO/pulse/timing drivers whose device response is a byte frame "
    "or scratchpad, use compact mock_preload.payload bytes in device order "
    "and compute expected_* from those bytes. Do not expand the byte frame "
    "into every per-bit GPIO pulse; that creates huge, fragile JSON and is "
    "not needed by the GPIO pulse runtime renderer.",
    "For pulse-width or bit-slot protocols, mock_preload.payload is the "
    "data frame bytes the virtual sensor should return, including checksum "
    "when the protocol uses one.",
    "For packed multi-channel GPIO byte frames, also include "
    "channel_preload_bytes mapping each expected channel id to the exact "
    "payload bytes that feed that channel. Keep mock_preload.payload, "
    "channel_preload_bytes, expected_channels, and derivation synchronized; "
    "derive checksum bytes from the final payload bytes when the Device IR "
    "states a checksum rule.",
    "Use mock_preload.schedule as [level, duration_us] pairs only when the "
    "measured signal itself is a duration/level waveform rather than a byte "
    "payload, such as echo-pulse or PWM-like devices. Keep schedules short "
    "(normally 16 pairs or fewer) and include raw_tolerance (typically 1) "
    "whenever the expected value is rounded from a fractional formula.",
    "For GPIO api_contract init_call arguments, prefer the concrete numeric "
    "pin identifier supplied by the fixed task context when present. Do not "
    "put board macro expressions such as GET_PIN(B,5) in the adapter "
    "contract unless SECTION C explicitly provides every macro/token needed "
    "for that expression to compile inside the generated adapter.",
)

_PLAN_I2C_RULES = (
    "For I2C/SMBus register-mapped devices, mock_preload keys must be real "
    "register or pointer addresses from the Device IR, such as \"0x10\" or "
    "\"reg_0x10\". Do not use descriptive state names such as chip_id or "
    "status_sequence as preload keys.",
)

_PLAN_SPI_RULES = (
    "For SPI devices, choose the mock_preload shape from SECTION D routing "
    "and the Device IR access model: stream-mode uses mock_preload.stream; "
    "command/full-duplex direct reads usually use read_bytes unless the IR "
    "models addressable registers; register-mode may use accepted register "
    "keys. Do not mix I2C-style register pointers into stream-only SPI.",
    "For SPI stream-mode devices, use mock_preload.stream as the complete "
    "MSB-first response frame served by the SPI slave. Do not use register "
    "address keys for stream-only devices.",
    "For SPI stream bitfields, include a signed two's-complement case when "
    "the IR marks a source as signed and the frame bytes plus expected value "
    "can be computed from the Device IR formula.",
    "For runtime-detectable SPI fault/status bits described by Device IR "
    "error_conditions or bitfields, add a small stimulus with expected_err=1 "
    "when the public driver should return a nonzero error. Do not use "
    "expected_err for normal value-read stimuli that should succeed.",
)

_PLAN_UART_RULES = (
    "For UART packet devices, mock_preload.read_bytes must be the complete "
    "device-to-MCU response frame served by the UART bot. Do not put the "
    "request command frame there.",
    "If a UART request or response frame has a checksum byte, compute the "
    "checksum for that exact frame. A request checksum only validates the "
    "request; never reuse it as the response checksum. Include the final "
    "response-checksum arithmetic in the stimulus derivation when the IR "
    "mentions checksum validation.",
    "For fixed-length UART frames, keep start/status/command bytes, payload "
    "bytes, reserved bytes, and checksum positions consistent with the Device "
    "IR frame description. The expected_* value must be derived from the "
    "response payload bytes, not from command bytes.",
)

# UART code guidance for the driver-generation prompt.
_UART_CODE_GUIDANCE = (
    "## UART FRAME & CHECKSUM GUIDANCE\n"
    "\n"
    "SECTION C lists the bound UART read/write primitives.  Use them"
    " exactly as listed; do not invent new function names.\n"
    "\n"
    "### Request command construction\n"
    "Copy the write transaction bytes verbatim from the Device IR"
    " ``operation_flows`` read step into a ``static const uint8_t``"
    " array.  The IR bytes already include the correct checksum; do"
    " not recompute it.\n"
    "\n"
    "### Response frame structure\n"
    "A typical sensor UART frame has a leading start/header byte,"
    " followed by a command echo byte, a variable-length payload, and"
    " a trailing checksum byte.  Consult the Device IR for the exact"
    " byte layout and field widths of your device's response.\n"
    "\n"
    "### Checksum computation\n"
    "The checksum is usually computed over the bytes BETWEEN the"
    " start/header byte and the checksum byte; the start byte is a"
    " framing delimiter, NOT part of the data payload.  When the"
    " checksum is the final byte of a fixed-length response, the"
    " range to sum is ``response[1]`` through ``response[len-2]``.\n"
    "Summing from ``response[0]`` includes the framing byte in the"
    " checksum and will reject every valid frame.\n"
    "\n"
    "### Read-and-parse pattern\n"
    "1. Send the write command from the Device IR.\n"
    "2. Read the expected response length into a buffer.\n"
    "3. Verify the start/header byte matches the expected value.\n"
    "4. Verify the checksum over the correct byte range.\n"
    "5. Extract payload fields per the Device IR conversion formula.\n"
    "\n"
    "The request command bytes and response length are defined in the"
    " Device IR ``operation_flows``; always derive them from there,"
    " never guess or hardcode protocol constants."
)

# GPIO code guidance for the driver-generation prompt.
_GPIO_CODE_GUIDANCE = (
    "## GPIO & TIMING PROTOCOL GUIDANCE\n"
    "\n"
    "SECTION C lists the bound GPIO and delay primitives for this"
    " platform. Use those symbols EXACTLY as written; do not invent"
    " GPIO function names, ioctl command codes, or pin macros.\n"
    "\n"
    "### GPIO pin assignment\n"
    "Use the fixed signal bindings rendered in SECTION C. Single-pin "
    "sensors use the same binding for trigger and echo/data unless the "
    "task provides separate bindings.\n"
    "A mismatch causes the slave to never see the driver's signals"
    " during runtime probing.\n"
    "\n"
    "### Pin direction management\n"
    "Configure a pin as OUTPUT before writing a level; switch to"
    " INPUT before reading.  For bidirectional single-wire protocols,"
    " re-configure direction inside the read/write sequence as needed.\n"
    "\n"
    "### Initialisation order\n"
    "(1) init GPIO peripheral/handle, (2) set pin direction,"
    " (3) set initial output level (HIGH for open-drain idle, LOW"
    " when datasheet requires a power-on default), (4) power-up /"
    " settling delay from Device IR timing constraints.\n"
    "\n"
    "### Delay primitives\n"
    "- For MILLISECOND delays: use the ms-level delay from SECTION C"
    " ``runtime.delay_ms``. Do not call a us-level function with a"
    " large argument.\n"
    "- For MICROSECOND delays: use the us-level delay from SECTION C"
    " ``runtime.delay_us``.  If the bound function takes a usec"
    " argument, pass the actual microsecond count. Do not write\n"
    "    ``delay_ms(0)`` or ``delay_ms(1)`` in a tight timing loop.\n"
    "    a zero or one-millisecond delay is far too coarse for"
    "    microsecond-level protocol timing and will cause the sensor"
    "    to miss the host's signal entirely.\n"
    "- A busy-wait loop WITHOUT a delay call (such as an empty ``for``"
    " loop or a bare ``while`` spin) provides no guaranteed timing"
    " reference and MUST NOT be used for pulse-width measurement or"
    " bit sampling.\n"
    "\n"
    "### Pulse-width measurement and bit-bang sampling\n"
    "- Every timing loop MUST call the bound ``delay_us`` function"
    " (or a calibrated NOP-loop with a stated clock assumption)"
    " inside the polling body.  Example correct pattern for"
    " measuring a high pulse:\n"
    "    uint32_t width_us = 0;\n"
    "    while (gpio_read(pin) == HIGH && width_us < timeout_us) {\n"
    "        delay_us(1);   // <-- actual delay, NOT a bare spin\n"
    "        width_us++;\n"
    "    }\n"
    "- The ``delay_us(1)`` call makes ``width_us`` a microsecond"
    " counter.  Without it, the counter reflects CPU loop iterations,"
    " which have no fixed relationship to microseconds.\n"
    "- For one-wire / bit-bang read sequences: drive the start"
    " condition, switch direction to input, then sample at the"
    " timing points from the Device IR operation_flows.  Use"
    " ``delay_us`` for the specified inter-bit and inter-byte gaps.\n"
    "\n"
    "### Other rules\n"
    "- If the protocol carries a checksum/CRC byte, verify it and"
    " treat a mismatch as a read error.\n"
    "- Every wait loop MUST have a timeout guard (Device IR maximum"
    " pulse/signal width + 50% margin) so the driver cannot hang on"
    " a missing or stuck sensor.\n"
    "- Do not call peripheral clock enables or use CPU no-op instructions "
    "as timing primitives; the harness initializes bus clocks."
)

_PLAN_HARD_RULES = (
    synthesis_prompt.HARD_RULES[2],
    synthesis_prompt.HARD_RULES[5],
    synthesis_prompt.HARD_RULES[6],
    synthesis_prompt.HARD_RULES[7],
    synthesis_prompt.HARD_RULES[8],
)


def build_plan_system_prompt(
    eval_class: str = "",
    routing: Optional[RoutingResult] = None,
) -> str:
    """build_plan_system_prompt helper."""
    eval_norm = str(eval_class or "").strip().lower()
    bus_norm = str(getattr(routing, "bus_kind", "") or "").strip().lower()

    rules: List[str] = list(_PLAN_COMMON_RULES)
    if eval_norm in ("", "multi_channel"):
        rules.extend(_PLAN_MULTI_CHANNEL_RULES)
    if eval_norm in ("", "display"):
        rules.extend(_PLAN_DISPLAY_RULES)
    if eval_norm in ("", "rtc"):
        rules.extend(_PLAN_RTC_RULES)
    if eval_norm in ("", "memory"):
        rules.extend(_PLAN_MEMORY_RULES)
    if bus_norm in ("", "i2c", "smbus"):
        rules.extend(_PLAN_I2C_RULES)
    if bus_norm in ("", "spi"):
        rules.extend(_PLAN_SPI_RULES)
    if bus_norm in ("", "uart"):
        rules.extend(_PLAN_UART_RULES)
    if bus_norm in ("", "gpio"):
        rules.extend(_PLAN_GPIO_RULES)

    rules.extend(_PLAN_HARD_RULES)
    return "\n\n".join(rules)


PLAN_SYSTEM_PROMPT = build_plan_system_prompt()

DRIVER_CODE_SYSTEM_PROMPT = "\n\n".join((
    "You are a senior embedded-systems engineer implementing driver C code "
    "against a frozen adapter contract and frozen runtime probe plan. "
    "Output exactly 1 JSON object with top-level keys \"driver_header\" "
    "and \"driver_source\" only.",
    "The api_contract and test_plan are frozen. Do not output them, do not "
    "rewrite expected_* values, and do not change public API units. If a "
    "runtime probe reports expected-vs-observed mismatch, fix the driver "
    "calculation or bus behavior so the frozen plan passes.",
    "For every frozen test_plan.expected_transactions row with "
    "write_prefix_any_of, the driver must emit one of those write prefixes "
    "as real bus traffic in init/read/write code. If Section E or the frozen "
    "plan contains separate register pointers, implement separate "
    "write_then_read transactions for those pointers; do not collapse them "
    "into a multi-byte burst read unless the Device IR explicitly states "
    "that the target registers are contiguous and burst-readable.",
    "Treat SECTION C as an allow-list for platform/API-surface calls. Every call "
    "into an external platform namespace must use one of the symbols rendered "
    "there, with the exact listed signature. Do not substitute a nearby API "
    "name from the same repository just because it appears in source. "
    "Private driver helpers should use device-specific names, not platform "
    "namespace names.",
    "For SPI register writes, if SECTION C only binds a send-plus-receive "
    "transfer primitive, use that exact primitive for write-only transfers "
    "with a null receive buffer and receive length 0. Do not invent an "
    "unlisted send-then-send/write helper.",
    "For SPI register burst reads, build the command byte from the register "
    "address plus the protocol read mask and multi-byte/burst mask when the "
    "Device IR, task protocol hints, or SECTION E expected_transactions encode "
    "one. A multi-byte read command is often `reg | read_mask | mb_mask`, not "
    "just `reg | read_mask`. Do not add the multi-byte/burst mask to "
    "single-byte identity/status reads; gate it on the requested read length.",
    "Your driver_header function prototypes must exactly match the frozen "
    "api_contract call expressions. If init_call passes a bus object such as "
    "`sensor_init(&g_eval_dev, bus)`, the driver init prototype must take "
    "the same bus-handle type used by the contract, not a string name; "
    "do not move bus lookup into the driver when "
    "init_extra_setup_c already found the bus handle. Apply the same rule to "
    "every read_call/channel call and out_arg type.",
    "For conversions, implement the Device IR integer_approximation_expression "
    "as the source of truth. Do not replace it with a prose formula, a "
    "placeholder zero, or a driver-local unit scale that disagrees with the "
    "frozen api_contract/test_plan units.",
    "When a conversion expression multiplies raw sensor codes by scaled "
    "coefficients, especially milli_* or micro_* outputs, promote the "
    "multiplication and division to `int64_t` or `uint64_t` intermediates "
    "before assigning back to the public output type. A 16-bit or wider raw "
    "field multiplied by 100000 or 175000 can overflow `int32_t` even when "
    "the final public result fits in `int32_t`.",
    "When the frozen api_contract primary_raw_unit or channel physical_unit "
    "says raw/count/lsb/code, return the bus/raw field directly and do not "
    "apply a physical conversion formula before assigning the output. If you "
    "also implement a converted helper, the frozen read_call must still use "
    "the raw/count function.",
    "When SECTION B3 OUTPUT VALUE SEMANTICS is present, treat it as the "
    "semantic source of truth behind the frozen api_contract/test_plan: "
    "semantic_kind=raw_count means no conversion in the frozen public output; "
    "semantic_kind=physical_scaled or physical_base means implement the Device "
    "IR conversion and return the listed public_unit unless the frozen "
    "api_contract explicitly says raw/count via its normalization note; "
    "in that case the eval ABI returns raw/count.",
    "When Device IR operation_flows or timing constraints specify delay_ms "
    "for a measurement flow, wait at least that many milliseconds before "
    "reading. Prefer datasheet max/required delays over typical delays.",
    "For GPIO pulse/timing flows, preserve sub-millisecond timing. If the "
    "Device IR says delay_ms=0.01, 10 microseconds, measure_pulse, pulse "
    "width, echo duration, or the frozen test_plan uses mock_preload.schedule, "
    "use the SECTION C microsecond delay/timing API when listed. "
    "Do not replace 10us/1us waits with a "
    "millisecond sleep or an uncalibrated nop loop.",
    "For timing constraints that mention a minimum interval between reads, do "
    "not invent tick/clock/time APIs to remember the previous read. Implement "
    "cross-call elapsed-time tracking only when SECTION C explicitly lists a "
    "monotonic time/tick getter and its unit conversion. If no such API is "
    "listed, use only the listed delay APIs for required blocking startup/read "
    "settling delays, or omit persistent interval tracking rather than calling "
    "unlisted symbols.",
    "For GPIO byte-frame protocols whose frozen test_plan uses "
    "mock_preload.payload, implement the complete pulse-framed read rather "
    "than skipping the response preamble with fixed delays. For a single-pin "
    "idle-high pulse-width frame, drive the start pulse, release/switch to "
    "input, wait for response LOW, wait until that LOW ends (HIGH), wait until "
    "the response HIGH ends (LOW), then for each data bit wait for LOW, wait "
    "for HIGH, measure the HIGH width until the next LOW, and classify the "
    "bit from that measured width. A wait-for-HIGH loop must poll while the "
    "pin is LOW; a wait-for-LOW/falling-edge loop must poll while the pin is "
    "HIGH. Use timeouts on every edge wait.",
    "For GPIO/integration attachments, use the exact fixed signal bindings "
    "rendered in SECTION C. Do not invent "
    "numeric pin constants or collapse two named signals onto the same pin.",
    "If the frozen test_plan contains expected_err for a runtime-detectable "
    "fault/status stimulus, implement the corresponding Device IR "
    "error_conditions or status-bit handling and return a nonzero error from "
    "the public read call. Do not silently return a numeric output for a "
    "fault frame that the plan expects to fail.",
    "If a frozen test_plan derivation mentions signed two's-complement "
    "sanity for an encoded raw field, decode that source field as signed "
    "before applying the positive-path scale. In that case the frozen "
    "test_plan expected value is the contract even if a Device IR expression "
    "would treat the encoded negative field as a large positive number.",
    "For I2C/SMBus, Device IR addresses and Section E addr_or_pin values are "
    "7-bit logical slave addresses. Pass them unchanged to APIs whose "
    "contract/signature/semantic_role takes a 7-bit or logical address. Only "
    "left-shift or add an R/W bit when the selected API contract explicitly "
    "states that its argument is an 8-bit on-wire address byte.",
    "For multi-address I2C packages, preserve the Section E "
    "expected_transactions addr_or_pin per flow/register. Do not reuse the "
    "primary address for every sub-device; a package may expose separate "
    "accelerometer/magnetometer/pressure/etc. slave addresses.",
    "For burst or split-register multi-channel reads, preserve the Device IR "
    "source byte/register order. If the bus burst returns low byte before "
    "high byte, combine the bytes accordingly instead of assuming buf[0] is "
    "always the high byte.",
    "For eval_class=rtc, obey the frozen api_contract year convention exactly. "
    "If time_fields.year is `driver_year + 2000` and time_struct_from_in uses "
    "`in->year - 2000`, then the driver struct year field must be 0..99: "
    "decode the year register into 0..99 and encode that field directly. Do "
    "not add 2000 in get_time or subtract 2000 again in set_time. If the "
    "frozen contract maps year directly, then use a full Gregorian year "
    "consistently instead.",
    "Include only headers needed by the symbols your driver actually calls. "
    "Do not include a binding's required_headers just because the binding was "
    "listed in Section C; unused slots do not require their headers, and "
    "missing/internal headers from unused slots must be left out.",
    "Do not manually redeclare platform APIs with ad-hoc extern "
    "prototypes. If the driver calls a SECTION C symbol, include the listed "
    "header basename that declares that symbol and call it with the exact "
    "SECTION C signature. Hand-written prototypes can pass syntax but fail "
    "linking or hide a wrong ABI.",
    synthesis_prompt.HARD_RULES[0],
    synthesis_prompt.HARD_RULES[1],
    synthesis_prompt.HARD_RULES[3],
    synthesis_prompt.HARD_RULES[4],
    synthesis_prompt.HARD_RULES[5],
    synthesis_prompt.HARD_RULES[7],
    synthesis_prompt.HARD_RULES[8],
    synthesis_prompt.HARD_RULES[9],
))


# Helpers
# and ``transaction_templates`` directly.)


def _section_a_task(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> str:
    device_id = device_ir.get("device_id") or "<unknown-device>"
    rtos_id = rtos_contract.get("rtos") or "<unknown-rtos>"
    ch = classify_result.channel_count
    chan_note = f", channel_count={ch}" if ch else ""
    roots = ", ".join(classify_result.channel_roots)
    roots_note = f" (channels={roots})" if roots else ""
    return (
        "## SECTION A — TASK\n\n"
        f"- Device: **{device_id}**\n"
        f"- Target platform: **{rtos_id}**\n"
        f"- Bus: **{classify_result.bus_type}**\n"
        f"- eval_class: **{classify_result.eval_class}**{chan_note}{roots_note}\n"
        f"- Files to emit: `{str(device_id).lower()}.h` + `{str(device_id).lower()}.c`\n"
    )


def _repair_context_values(
    repair_context: Optional[Mapping[str, Any]],
    key: str,
) -> Tuple[Any, ...]:
    if not isinstance(repair_context, Mapping):
        return ()
    value = repair_context.get(key)
    if isinstance(value, Sequence) and not isinstance(value, (str, bytes, bytearray)):
        return tuple(value)
    return ()


def _repair_context_has(
    repair_context: Optional[Mapping[str, Any]],
    key: str,
    value: str,
) -> bool:
    return str(value) in {str(item) for item in _repair_context_values(repair_context, key)}


def _section_b_device_spec(
    device_ir: Mapping[str, Any],
    repair_context: Optional[Mapping[str, Any]] = None,
) -> str:
    """SECTION B - render device_ir into a compact Markdown spec block."""
    ir = dict(device_ir) if device_ir else {}
    selected_fields = {
        str(field)
        for field in _repair_context_values(repair_context, "device_fields")
    }
    focused = bool(selected_fields)
    lines: List[str] = ["## SECTION B — DEVICE SPEC\n"]
    if focused:
        tags = ", ".join(
            f"`{tag}`" for tag in _repair_context_values(repair_context, "focus_tags")
        )
        lines.append(
            "Repair context: this section renders only the Device IR fields "
            "selected from the previous failure"
            + (f" ({tags})." if tags else ".")
        )
        lines.append("")

    def include(field: str) -> bool:
        return not focused or field in selected_fields

    if include("address_rule"):
        _render_address_rule(ir.get("address_rule") or {}, lines)
    if include("registers_or_commands"):
        _render_registers(ir.get("registers_or_commands") or [], lines)
    if include("read_channels"):
        _render_read_channels(ir.get("read_channels") or [], lines)
    if include("init_sequence"):
        _render_sequence("Init Sequence", ir.get("init_sequence") or [], lines)
    if include("read_sequence"):
        _render_sequence("Read Sequence", ir.get("read_sequence") or [], lines)
    if include("operation_flows"):
        _render_operation_flows(ir.get("operation_flows") or [], lines)
    if include("raw_encoding"):
        _render_raw_encoding(ir.get("raw_encoding") or {}, lines)
    if include("timing_constraints"):
        _render_timing(ir.get("timing_constraints") or [], lines)
    if include("conversion_formulae"):
        _render_conversion_formulae(ir.get("conversion_formulae") or [], lines)
    if include("error_conditions"):
        _render_error_conditions(ir.get("error_conditions") or [], lines)
    if include("bitfields"):
        _render_bitfields(ir.get("bitfields") or [], lines)

    return "\n".join(lines)


def _section_b2_channel_alias_map(
    channel_alias_map: Optional[Mapping[str, Any]],
) -> Optional[str]:
    if not isinstance(channel_alias_map, Mapping):
        return None
    channels = channel_alias_map.get("channels")
    if not isinstance(channels, list) or not channels:
        return None

    lines: List[str] = [
        "## SECTION B2 - CHANNEL CANONICALIZATION",
        "",
        "Use `canonical_id` as the public multi-channel API/test channel id. "
        "`source_channels` and `aliases` identify the corresponding Device IR "
        "read_channels and may be used internally in the driver.",
        "",
        "### Canonical Channel Map",
    ]
    for row in channels:
        if not isinstance(row, Mapping):
            continue
        canonical = row.get("canonical_id") or ""
        if not canonical:
            continue
        sources = row.get("source_channels") or []
        aliases = row.get("aliases") or []
        details: List[str] = []
        if isinstance(sources, list) and sources:
            details.append(
                "source_channels=[" + ", ".join(str(v) for v in sources) + "]"
            )
        if isinstance(aliases, list) and aliases:
            details.append("aliases=[" + ", ".join(str(v) for v in aliases[:8]) + "]")
        for key in ("quantity", "location", "axis", "unit"):
            value = row.get(key)
            if value:
                details.append(f"{key}={value}")
        detail_str = " | ".join(details)
        lines.append(f"- `{canonical}`" + (f": {detail_str}" if detail_str else ""))

    warnings = channel_alias_map.get("warnings")
    if isinstance(warnings, list) and warnings:
        lines.append("")
        lines.append("### Alias Map Warnings")
        for warning in warnings[:8]:
            lines.append(f"- {warning}")
    return "\n".join(lines)


def _section_b3_output_semantics_map(
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Optional[str]:
    if not isinstance(output_semantics_map, Mapping):
        return None
    channels = output_semantics_map.get("channels")
    if not isinstance(channels, list) or not channels:
        return None

    lines: List[str] = [
        "## SECTION B3 - OUTPUT VALUE SEMANTICS",
        "",
        "Use this section as the public output semantic contract for "
        "api_contract, test_plan expected values, and driver_code. It decides "
        "whether each output is raw/count or converted physical value. This "
        "section is generation context only and is not an evaluation answer.",
        "",
        "### Public Output Semantics Map",
    ]
    for row in channels:
        if not isinstance(row, Mapping):
            continue
        public_id = row.get("public_id") or row.get("source_channel") or ""
        if not public_id:
            continue
        details: List[str] = []
        for key in (
            "source_channel",
            "semantic_kind",
            "public_unit",
            "c_type",
            "conversion_required",
            "formula_id",
        ):
            value = row.get(key)
            if value not in (None, ""):
                details.append(f"{key}={value}")
        evidence = str(row.get("evidence") or "").strip()
        if evidence:
            details.append(f"evidence={evidence[:180]}")
        detail_str = " | ".join(details)
        lines.append(f"- `{public_id}`" + (f": {detail_str}" if detail_str else ""))

    warnings = output_semantics_map.get("warnings")
    if isinstance(warnings, list) and warnings:
        lines.append("")
        lines.append("### Output Semantics Warnings")
        for warning in warnings[:8]:
            lines.append(f"- {warning}")
    return "\n".join(lines)


def _render_address_rule(addr: Any, lines: List[str]) -> None:
    """Render the device address rule."""
    if not isinstance(addr, Mapping) or not addr:
        return
    addr_type = addr.get("type") or addr.get("addressing_mode") or ""
    addressing_form = addr.get("addressing_form")
    resolution = addr.get("default_address_resolution") or {}
    addresses = addr.get("addresses")

    if isinstance(addresses, list) and addresses:
        default_strs: List[str] = []
        alt_strs: List[str] = []
        for a in addresses:
            if isinstance(a, Mapping):
                av = a.get("address") or a.get("value") or ""
                desc = a.get("description") or a.get("condition") or ""
                base = f"`{av}` ({desc})" if desc else f"`{av}`"
                if a.get("is_default") is True:
                    default_strs.append(f"**{base} - default**")
                else:
                    alt_strs.append(base)
            else:
                alt_strs.append(f"`{a}`")
        type_tag = f" ({addr_type})" if addr_type else ""
        form_tag = f" [{addressing_form}]" if isinstance(addressing_form, str) else ""
        rendered = ", ".join(default_strs + alt_strs)
        lines.append(f"- I2C Addresses{type_tag}{form_tag}: {rendered}")

        if default_strs:
            lines.append(
                "  - Use the **default** address for the slave whenever a "
                "single address is needed (init, read, sequence pointer); "
                "alternative entries are only relevant when the integration "
                "binding overrides the bus address."
            )
        method = resolution.get("method") if isinstance(resolution, Mapping) else None
        if method == "first-fallback":
            warning = (
                resolution.get("warning")
                if isinstance(resolution, Mapping) and resolution.get("warning")
                else "Multiple address candidates without an explicit default - "
                     "treat the first as default but record uncertainty in "
                     "requires_human."
            )
            lines.append(f"  - Resolution: first-fallback. {warning}")
        return

    if isinstance(addresses, Mapping) and addresses:
        addr_strs = [
            f"{k}=`{('0x%02X' % v) if isinstance(v, int) else v}`"
            for k, v in addresses.items()
        ]
        type_tag = f" ({addr_type})" if addr_type else ""
        form_tag = f" [{addressing_form}]" if isinstance(addressing_form, str) else ""
        lines.append(f"- I2C Address map{type_tag}{form_tag}: {', '.join(addr_strs)}")
        return

    options = addr.get("address_7bit_options")
    if isinstance(options, list) and options:
        opts = ", ".join(f"`{a}`" for a in options)
        type_tag = f" ({addr_type})" if addr_type else ""
        form_tag = f" [{addressing_form}]" if isinstance(addressing_form, str) else ""
        lines.append(f"- I2C Address Options{type_tag}{form_tag}: {opts}")
        return

    fallback_keys = {
        k: v for k, v in addr.items()
        if k not in (
            "type", "addressing_mode", "addressing_form",
            "default_address_resolution", "evidence_spans",
        ) and v
    }
    if fallback_keys:
        type_tag = f" ({addr_type})" if addr_type else ""
        form_tag = f" [{addressing_form}]" if isinstance(addressing_form, str) else ""
        lines.append(
            f"- I2C Address{type_tag}{form_tag}: "
            f"{json.dumps(fallback_keys, ensure_ascii=False)}"
        )


# Broadcast phrase markers for register/command rendering.
_BROADCAST_PHRASES: Tuple[str, ...] = (
    "general call",
    "general-call",
    "broadcast",
    "all devices on the bus",
    "every device on the bus",
)


def _is_broadcast_command(name: str, description: str) -> bool:
    """True iff ``name`` or ``description`` indicates an I2C broadcast."""
    blob = f"{name or ''} {description or ''}".lower()
    return any(phrase in blob for phrase in _BROADCAST_PHRASES)


def _split_broadcast_value_bytes(val: str) -> Optional[Tuple[str, str]]:
    """Split a 16-bit broadcast command value into address and payload bytes."""
    if not isinstance(val, str):
        return None
    text = val.strip()
    if not text.lower().startswith("0x"):
        return None
    try:
        v = int(text, 16)
    except ValueError:
        return None
    if v < 0 or v > 0xFFFF:
        return None
    high = (v >> 8) & 0xFF
    low = v & 0xFF
    return f"0x{high:02X}", f"0x{low:02X}"


def _render_registers(cmds: Any, lines: List[str]) -> None:
    if not isinstance(cmds, list) or not cmds:
        return
    lines.append("\n### Commands / Registers\n")
    has_broadcast = False
    for c in cmds:
        if not isinstance(c, Mapping):
            continue
        name = c.get("name") or ""
        val = c.get("value") or c.get("opcode") or c.get("address") or ""
        if isinstance(val, int):
            val = f"0x{val:02X}"
        access = c.get("access") or ""
        size = c.get("size_bits")
        desc = c.get("description") or ""
        extras: List[str] = []
        if access:
            extras.append(access)
        if size:
            extras.append(f"{size}-bit")
        is_broadcast = _is_broadcast_command(str(name), str(desc))
        if is_broadcast:
            has_broadcast = True
            extras.append("BROADCAST")
        extra_str = f" [{', '.join(extras)}]" if extras else ""
        head = f"- `{name}` = `{val}`{extra_str}"
        lines.append(f"{head} - {desc}" if desc else head)
        if is_broadcast:
            split = _split_broadcast_value_bytes(str(val))
            if split is not None:
                addr_byte, cmd_byte = split
                lines.append(
                    "    - **Wire format**: this is a 2-byte I2C "
                    "broadcast frame. The first byte ("
                    f"`{addr_byte}`) IS the I2C general-call address "
                    "(7-bit `0x00`, R/W=0); the driver MUST emit it "
                    "as the wire address, NOT pack it into a "
                    "command word. Only the second byte ("
                    f"`{cmd_byte}`) goes to the data phase."
                )
                lines.append(
                    "    - **Driver action**: issue exactly one "
                    "I2C write transaction with `slave_addr=0x00` "
                    f"(NOT the per-device 7-bit address) and "
                    f"payload `[{cmd_byte}]`. Per-device reads "
                    "and writes around this broadcast continue to "
                    "use the IR-default 7-bit address from the "
                    "Address Rule section."
                )
            else:
                lines.append(
                    "    - **Wire format**: this is an I2C "
                    "broadcast frame. The driver MUST send it to "
                    "the I2C general-call address (7-bit `0x00`), "
                    "NOT to the per-device address."
                )
    if has_broadcast:
        lines.append(
            "\n> NOTE: any command tagged `[BROADCAST]` above is "
            "addressed to the I2C general-call address `0x00`, never "
            "to the per-device 7-bit address. Drivers that hard-code "
            "the device address for those frames either NACK or, "
            "when paired with a register-pointer mock, silently "
            "corrupt downstream reads.\n"
        )


def _render_read_channels(channels: Any, lines: List[str]) -> None:
    if not isinstance(channels, list) or not channels:
        return
    lines.append("\n### Read Channels (driver-visible measurements)\n")
    for ch in channels:
        if isinstance(ch, Mapping):
            cid = ch.get("id") or "?"
            raw_type = ch.get("raw_type") or ""
            unit = ch.get("physical_unit") or ""
            desc = ch.get("description") or ""
            tags: List[str] = []
            if raw_type:
                tags.append(f"raw=`{raw_type}`")
            if unit:
                tags.append(f"unit=`{unit}`")
            tag_str = f" ({', '.join(tags)})" if tags else ""
            lines.append(f"- `{cid}`{tag_str}" + (f" - {desc}" if desc else ""))
        else:
            lines.append(f"- `{ch}`")


def _render_sequence(title: str, seq: Any, lines: List[str]) -> None:
    if not isinstance(seq, list) or not seq:
        return
    lines.append(f"\n### {title}\n")
    for i, step in enumerate(seq, 1):
        if not isinstance(step, Mapping):
            lines.append(f"{i}. `{step!r}`")
            continue
        notes = step.get("notes") or step.get("comment") or ""
        if "transaction" in step:
            tx = step["transaction"]
            if tx is None:
                tag = "*(no bus traffic)*"
                lines.append(
                    f"{i}. {tag}" + (f" - {notes}" if notes else "")
                )
                continue
            if isinstance(tx, Mapping):
                lines.append(
                    f"{i}. {_format_transaction_step(tx)}"
                    + (f" - {notes}" if notes else "")
                )
                continue
        action = step.get("action")
        if action:
            details = {
                k: v for k, v in step.items()
                if k not in ("action", "comment", "notes") and v
            }
            details_str = (
                f" ({json.dumps(details, ensure_ascii=False)})" if details else ""
            )
            lines.append(
                f"{i}. `{action}`{details_str}"
                + (f" - {notes}" if notes else "")
            )
            continue
        desc = step.get("description") or ""
        if desc:
            opt = " *(optional)*" if step.get("optional") else ""
            lines.append(f"{i}. {desc}{opt}")
            continue
        lines.append(f"{i}. {json.dumps(step, ensure_ascii=False)}")


def _render_operation_flows(flows: Any, lines: List[str]) -> None:
    """Render executable IR operation flows, especially required delays."""
    if not isinstance(flows, list) or not flows:
        return
    lines.append("\n### Operation Flows\n")
    max_flows = 10
    max_steps = 8
    for flow_index, flow in enumerate(flows[:max_flows], 1):
        if not isinstance(flow, Mapping):
            continue
        flow_id = flow.get("flow_id") or f"flow_{flow_index}"
        kind = flow.get("kind") or ""
        channels = flow.get("channels") or []
        ch_str = ""
        if isinstance(channels, list) and channels:
            ch_str = " channels=[" + ", ".join(str(ch) for ch in channels) + "]"
        notes = flow.get("notes") or ""
        head = f"- `{flow_id}`"
        if kind:
            head += f" kind=`{kind}`"
        head += ch_str
        if notes:
            head += f" - {str(notes)[:180]}"
        lines.append(head)

        steps = flow.get("steps") or []
        if not isinstance(steps, list):
            continue
        for step_index, step in enumerate(steps[:max_steps], 1):
            if not isinstance(step, Mapping):
                lines.append(f"  {step_index}. `{step!r}`")
                continue
            op = step.get("op") or "step"
            if op == "delay":
                delay = step.get("delay_ms")
                step_notes = step.get("notes") or ""
                suffix = f" - {step_notes}" if step_notes else ""
                lines.append(f"  {step_index}. delay_ms=`{delay}`{suffix}")
                continue
            tx = step.get("transaction")
            if isinstance(tx, Mapping):
                step_notes = step.get("notes") or tx.get("notes") or ""
                suffix = f" - {step_notes}" if step_notes else ""
                lines.append(
                    f"  {step_index}. `{op}`: {_format_transaction_step(tx)}"
                    f"{suffix}"
                )
                continue
            step_notes = step.get("notes") or step.get("description") or ""
            suffix = f" - {step_notes}" if step_notes else ""
            lines.append(f"  {step_index}. `{op}`{suffix}")
        if len(steps) > max_steps:
            lines.append(f"  ... ({len(steps) - max_steps} more step(s) elided)")
    if len(flows) > max_flows:
        lines.append(f"- ... ({len(flows) - max_flows} more flow(s) elided)")


def _format_transaction_step(tx: Mapping[str, Any]) -> str:
    kind = tx.get("kind") or "?"
    parts: List[str] = [f"**{kind}**"]
    addr = tx.get("address")
    if addr is not None:
        parts.append(f"addr=`{addr}`")
    bytes_ = tx.get("bytes")
    if isinstance(bytes_, list) and bytes_:
        bytes_str = ", ".join(str(b) for b in bytes_)
        parts.append(f"bytes=[{bytes_str}]")
    length = tx.get("length")
    if length is not None:
        parts.append(f"length={length}")
    pointer = tx.get("pointer_target") or tx.get("pointer")
    if pointer:
        parts.append(f"pointer=`{pointer}`")
    return " | ".join(parts)


def _render_raw_encoding(enc: Any, lines: List[str]) -> None:
    if not isinstance(enc, Mapping) or not enc:
        return
    lines.append("\n### Raw Encoding\n")
    for key in (
        "byte_order", "bit_width", "effective_bits", "signed",
        "right_shift", "sign_extend_from_bit",
    ):
        if key in enc and enc[key] is not None:
            lines.append(f"- {key}: `{enc[key]}`")
    notes = enc.get("notes")
    if notes:
        lines.append(f"- notes: {notes}")


def _render_timing(timing: Any, lines: List[str]) -> None:
    if not isinstance(timing, list) or not timing:
        return
    lines.append("\n### Timing Constraints\n")
    for t in timing:
        if not isinstance(t, Mapping):
            continue
        name = t.get("name") or ""
        val = t.get("value")
        unit = t.get("unit") or ""
        cond = t.get("condition") or ""
        notes = t.get("notes") or t.get("description") or ""
        val_str = ""
        if val is not None and val != "":
            val_str = f" {val} {unit}".rstrip()
        cond_str = f" [{cond}]" if cond else ""
        notes_str = f" - {notes}" if notes else ""
        lines.append(f"- {name}:{val_str}{cond_str}{notes_str}")


def _render_conversion_formulae(formulae: Any, lines: List[str]) -> None:
    if not isinstance(formulae, list) or not formulae:
        return
    lines.append("\n### Conversion Formulae\n")
    for f in formulae:
        if not isinstance(f, Mapping):
            continue
        name = f.get("name") or f.get("mode") or ""
        formula = f.get("formula") or ""
        lines.append(f"- **{name}**: `{formula}`")
        approx = f.get("integer_approximation_expression")
        if isinstance(approx, Mapping):
            expr = approx.get("expression") or ""
            if expr:
                lines.append(f"  - integer expression: `{expr}`")
            inputs = approx.get("inputs") or []
            if isinstance(inputs, list) and inputs:
                input_strs: List[str] = []
                for inp in inputs:
                    if not isinstance(inp, Mapping):
                        continue
                    iname = inp.get("name") or "?"
                    src = inp.get("byte_source") or ""
                    src_tag = f" from {src}" if src else ""
                    input_strs.append(f"`{iname}`{src_tag}")
                if input_strs:
                    lines.append(f"  - inputs: {', '.join(input_strs)}")
            output = approx.get("output")
            if isinstance(output, Mapping):
                oname = output.get("name") or ""
                ounit = output.get("unit") or ""
                if oname:
                    out_str = f"`{oname}`"
                    if ounit:
                        out_str += f" ({ounit})"
                    lines.append(f"  - output: {out_str}")
        elif isinstance(approx, str) and approx:
            lines.append(f"  - integer expression: `{approx}`")


def _render_error_conditions(errors: Any, lines: List[str]) -> None:
    if not isinstance(errors, list) or not errors:
        return
    lines.append("\n### Error Conditions\n")
    for e in errors:
        if not isinstance(e, Mapping):
            continue
        cond = e.get("condition") or e.get("name") or e.get("type") or ""
        act = (
            e.get("driver_action")
            or e.get("action")
            or e.get("handling")
            or e.get("response")
            or ""
        )
        detect = e.get("detection") or ""
        notes = e.get("notes") or ""
        parts = [f"**{cond}**"] if cond else []
        if detect:
            parts.append(f"detect: {detect}")
        if act:
            parts.append(f"action: {act}")
        if notes:
            parts.append(f"notes: {notes}")
        lines.append(f"- {' - '.join(parts)}" if parts else "- (empty entry)")


def _render_bitfields(bitfields: Any, lines: List[str]) -> None:
    if not isinstance(bitfields, list) or not bitfields:
        return
    lines.append("\n### Configuration Bitfields\n")
    for bf in bitfields:
        if not isinstance(bf, Mapping):
            continue
        reg = bf.get("register") or ""
        name = bf.get("name") or ""
        pos = bf.get("bit_position", "")
        if isinstance(pos, list):
            pos = f"[{pos[-1]}:{pos[0]}]"
        else:
            pos = f"[{pos}]"
        enums = bf.get("enum_values") or []
        enum_parts = [
            f"{ev.get('name')}={ev.get('value')}"
            for ev in enums if isinstance(ev, Mapping)
        ]
        enum_str = f": {', '.join(enum_parts)}" if enum_parts else ""
        lines.append(f"- {reg}.{name} bit{pos}{enum_str}")


_MAX_STEPS_PER_TEMPLATE = 12


def _uses_i2c_message_address_slot(
    symbol: str,
    signature: str,
    role: str,
) -> bool:
    """True when a binding exposes an I2C message struct with an addr field."""
    blob = f"{symbol} {signature} {role}".lower()
    return (
        "i2c_msg" in blob
        or "message" in role.lower()
    )


def _render_signature_usage_notes(
    lines: List[str],
    *,
    symbol: str,
    signature: str,
    role: str,
) -> None:
    if _uses_i2c_message_address_slot(symbol, signature, role):
        lines.append(
            "- address convention: this is a message-struct I2C "
            "API. Assign Section E / Device IR 7-bit slave "
            "addresses directly to the message `.addr` field; keep "
            "read/write direction in the message flags. Do not "
            "left-shift the address or add an R/W bit for this API."
        )

    for index, struct_name, param_name, _arg in signature_config_pointer_args(signature):
        param = f"`{param_name}`" if param_name else "this parameter"
        lines.append(
            f"- config-pointer argument: parameter #{index} {param} expects "
            f"a pointer to `struct {struct_name}`. Do not pass the device "
            "address or another scalar directly in that position. Declare "
            f"and initialize a local `struct {struct_name}` using the "
            "struct field allow-list below, set address/frequency/address "
            "length fields from the Device IR and task context when those "
            "fields exist, then pass a pointer such as `&cfg` or an "
            "already-valid config pointer."
        )


def _contract_include_name(raw_header: Any) -> str:
    text = str(raw_header or "").strip().strip("<>").strip('"').replace("\\", "/")
    if not text:
        return ""
    include_match = re.search(r"(?:^|/)include/(.+)$", text)
    if include_match:
        return include_match.group(1).strip("/")
    for prefix in ("cpukit/include/", "include/"):
        if text.startswith(prefix):
            return text[len(prefix):].strip("/")
    return text


def _format_contract_include(raw_header: Any) -> str:
    name = _contract_include_name(raw_header)
    if not name:
        return ""
    if "/" in name or name in _C_STANDARD_INCLUDE_HEADERS:
        return f"#include <{name}>"
    return f'#include "{name}"'


def _section_c_integration_bindings(rtos_contract: Mapping[str, Any]) -> List[str]:
    if not isinstance(rtos_contract, Mapping):
        return []
    integration = rtos_contract.get("integration_contract")
    connection = rtos_contract.get("connection")
    if not isinstance(integration, Mapping):
        integration = {}
    if not isinstance(connection, Mapping):
        connection = {}

    fixed_attachment = integration.get("fixed_attachment")
    if not isinstance(fixed_attachment, Mapping):
        fixed_attachment = connection.get("fixed_attachment")
    if not isinstance(fixed_attachment, Mapping):
        fixed_attachment = {}

    bus_instance = integration.get("bus_instance") or connection.get("bus_instance")
    bus_symbol = integration.get("bus_symbol") or connection.get("bus_symbol")
    backend = connection.get("backend") or integration.get("backend")
    include_headers = integration.get("include_headers") or []
    helper_usage_patterns = (
        integration.get("helper_usage_patterns")
        or connection.get("helper_usage_patterns")
        or []
    )
    runtime_required = integration.get("runtime_provision_required_for") or []

    if not any((
        fixed_attachment,
        bus_instance,
        bus_symbol,
        backend,
        include_headers,
        helper_usage_patterns,
        runtime_required,
    )):
        return []

    lines: List[str] = ["### Integration bindings (task / board context)\n"]
    lines.append(
        "These are concrete task bindings, not optional examples. Use them "
        "exactly when initialising bus handles or GPIO signal pins; do not "
        "replace named signal bindings with guessed numeric constants."
    )
    if bus_instance:
        lines.append(f"- bus_instance: `{bus_instance}`")
    if bus_symbol:
        lines.append(f"- bus_symbol: `{bus_symbol}`")
    if backend:
        lines.append(f"- backend: `{backend}`")
    if (
        isinstance(include_headers, Sequence)
        and not isinstance(include_headers, (str, bytes))
        and include_headers
    ):
        rendered_headers: list[str] = []
        seen_headers: set[str] = set()
        for raw_header in include_headers:
            directive = _format_contract_include(raw_header)
            if not directive or directive in seen_headers:
                continue
            seen_headers.add(directive)
            rendered_headers.append(directive)
        if rendered_headers:
            lines.append("- support headers from extracted platform source:")
            for directive in rendered_headers[:16]:
                lines.append(f"  - include: `{directive}`")
    if fixed_attachment:
        lines.append("- fixed_attachment:")
        for key in sorted(fixed_attachment.keys()):
            lines.append(f"  - {key}: `{fixed_attachment[key]}`")
    if (
        isinstance(helper_usage_patterns, Sequence)
        and not isinstance(helper_usage_patterns, (str, bytes))
        and helper_usage_patterns
    ):
        lines.append("- helper_usage_patterns from task context:")
        for raw_pattern in helper_usage_patterns:
            if not isinstance(raw_pattern, Mapping):
                continue
            pattern_id = raw_pattern.get("pattern_id") or "(unnamed)"
            applies_to = raw_pattern.get("applies_to")
            purpose = raw_pattern.get("purpose")
            head = f"  - `{pattern_id}`"
            if applies_to:
                head += f" (applies_to={applies_to})"
            lines.append(head)
            if purpose:
                lines.append(f"    - purpose: {purpose}")
            lines.append(
                "    - declaration rule: symbols, types, and constants listed "
                "below are provided by the RTOS/task headers already named in "
                "SECTION C. Use them as existing declarations; do not "
                "redeclare structs or `#define` constants with the same names."
            )
            for field, label in (
                ("required_symbols", "required_symbols"),
                ("required_types", "required_types"),
                ("required_constants", "required_constants"),
            ):
                values = raw_pattern.get(field) or []
                if (
                    isinstance(values, Sequence)
                    and not isinstance(values, (str, bytes))
                    and values
                ):
                    lines.append(
                        f"    - {label}: "
                        + ", ".join(f"`{value}`" for value in values)
                    )
            steps = raw_pattern.get("steps") or []
            if (
                isinstance(steps, Sequence)
                and not isinstance(steps, (str, bytes))
                and steps
            ):
                lines.append("    - required sequence:")
                for index, step in enumerate(steps[:8], start=1):
                    lines.append(f"      {index}. {step}")
    if (
        isinstance(runtime_required, Sequence)
        and not isinstance(runtime_required, (str, bytes))
        and runtime_required
    ):
        lines.append(
            "- runtime_provision_required_for: "
            + ", ".join(f"`{h}`" for h in runtime_required)
        )
    lines.append("")
    return lines


def _section_c_rtos_api(
    rtos_contract: Mapping[str, Any],
    repair_context: Optional[Mapping[str, Any]] = None,
) -> str:
    """SECTION C - platform API reference."""
    selected_slots = {
        str(slot)
        for slot in _repair_context_values(repair_context, "rtos_slots")
    }
    focused = bool(selected_slots)
    if focused and isinstance(rtos_contract, Mapping):
        filtered_contract = dict(rtos_contract)
        raw_bindings = rtos_contract.get("api_bindings")
        if isinstance(raw_bindings, Mapping):
            filtered_contract["api_bindings"] = {
                slot_id: binding
                for slot_id, binding in raw_bindings.items()
                if str(slot_id) in selected_slots
            }
        rtos_contract = filtered_contract

    rtos_contract = sanitize_rtos_contract_for_codegen(rtos_contract)
    lines: List[str] = ["## SECTION C — PLATFORM API REFERENCE\n"]
    if focused:
        terms = ", ".join(
            f"`{term}`" for term in _repair_context_values(repair_context, "compile_terms")
        )
        lines.append(
            "Repair context: this section renders the API bindings selected "
            "from the previous failure"
            + (f" (matched terms: {terms})." if terms else ".")
        )
        lines.append("")

    surface = rtos_contract.get("codegen_surface")
    if isinstance(surface, Mapping):
        warnings = [
            str(item)
            for item in (surface.get("warnings") or [])
            if str(item or "").strip()
        ]
        forbidden_symbols = [
            str(item)
            for item in (surface.get("forbidden_symbols") or [])
            if str(item or "").strip()
        ]
        forbidden_headers = [
            str(item)
            for item in (surface.get("forbidden_headers") or [])
            if str(item or "").strip()
        ]
        if warnings or forbidden_symbols or forbidden_headers:
            lines.append("### Codegen surface sanitization\n")
            lines.append(
                "The platform contract has been checked against the public "
                "codegen/stub surface. Treat the sanitized bindings below "
                "as authoritative; do not use removed symbols or unavailable "
                "headers even if they appeared in earlier repository evidence.\n"
            )
            for item in warnings[:12]:
                lines.append(f"- {item}")
            if forbidden_symbols:
                lines.append(
                    "- forbidden symbols: "
                    + ", ".join(f"`{sym}`" for sym in forbidden_symbols[:16])
                )
            if forbidden_headers:
                lines.append(
                    "- forbidden headers: "
                    + ", ".join(f"`{hdr}`" for hdr in forbidden_headers[:16])
                )
            lines.append("")

    api_bindings: Mapping[str, Any] = {}
    if isinstance(rtos_contract, Mapping):
        ab = rtos_contract.get("api_bindings")
        if isinstance(ab, Mapping):
            api_bindings = ab

    if not api_bindings:
        lines.append(
            "*(No `api_bindings` were produced for this platform contract. "
            "Stay within the symbols already named in Section D/E and "
            "the schema constants in Section F; do NOT invent additional "
            "platform function names; every call must resolve "
            "to a symbol the platform contract has explicitly listed.)*\n"
        )
        lines.extend(_section_c_integration_bindings(rtos_contract))
        return "\n".join(lines)

    manifest_entries: List[tuple[str, Mapping[str, Any]]] = []
    helper_entries: List[tuple[str, Mapping[str, Any]]] = []
    for slot_id, binding in api_bindings.items():
        if focused and str(slot_id) not in selected_slots:
            continue
        if not isinstance(binding, Mapping):
            continue
        if binding.get("source_kind") == "task_package_helper":
            helper_entries.append((slot_id, binding))
        else:
            manifest_entries.append((slot_id, binding))

    if manifest_entries:
        lines.append("### API bindings (from platform source)\n")
        lines.append(
            "This section is a call allow-list: generated driver code may "
            "call only the platform symbols rendered below, plus any "
            "task-package helpers listed later. Do not use adjacent function "
            "names from the same namespace unless they are explicitly "
            "listed here; the signatures below are authoritative.\n"
        )
        lines.append(
            "Each `include` directive below is normalized to the path the "
            "stub-compile sandbox can resolve (for example "
            "`<linux/i2c-dev.h>` or `<sys/ioctl.h>` keeps required "
            "subdirectories). The original repo-relative path may be shown "
            "for traceability only. Use the rendered directive verbatim. "
            "These include lines are per-slot requirements, not a global "
            "include checklist: include "
            "a header only when your emitted C code directly calls the "
            "symbol in that same slot.\n"
        )
        seen_headers: set[str] = set()
        for slot_id, b in manifest_entries:
            sym = b.get("symbol") or "?"
            sig = (b.get("signature") or "").rstrip(";").strip()
            kind = str(b.get("kind") or "function").strip().lower()
            callable_binding = kind in {"", "function", "func"}
            headers = b.get("required_headers") or []
            role = (b.get("semantic_role") or "").strip()
            lines.append(f"#### slot `{slot_id}` -> `{sym}`")
            if callable_binding:
                for h in headers:
                    if not h or h in seen_headers:
                        continue
                    seen_headers.add(h)
                    directive = _format_contract_include(h)
                    include_name = _contract_include_name(h)
                    if include_name != str(h):
                        lines.append(
                            f"- include: `{directive}`  "
                            f"(repo path: `{h}`)"
                        )
                    else:
                        lines.append(f"- include: `{directive}`")
            elif not callable_binding:
                lines.append(
                    "- supporting non-function binding: do not include this "
                    "slot's header just because it is listed here; include "
                    "only headers required by the callable API or a helper "
                    "usage pattern your code actually uses."
                )
            if sig:
                lines.append("```c")
                lines.append(f"{sig};")
                lines.append("```")
            if role:
                lines.append(f"- semantic role: {role}")
            _render_signature_usage_notes(
                lines,
                symbol=str(sym),
                signature=str(sig),
                role=str(role),
            )
            lines.append("")

    if helper_entries:
        lines.append("### Task-package helpers (link-time provided)\n")
        lines.append(
            "These symbols are NOT part of the platform source "
            "source. The task package contract guarantees they will "
            "resolve at link time (a stub or board-specific weak "
            "occurrence supplies the body). Call them like any other "
            "declared function.\n"
        )
        for slot_id, b in helper_entries:
            sym = b.get("symbol") or "?"
            role = (b.get("semantic_role") or "").strip()
            suffix = f" - {role}" if role else ""
            lines.append(f"- slot `{slot_id}` -> `{sym}`{suffix}")
            sig = (b.get("signature") or "").rstrip(";").strip()
            kind = str(b.get("kind") or "function").strip().lower()
            callable_binding = kind in {"", "function", "func"}
            headers = b.get("required_headers") or []
            if callable_binding:
                for h in headers:
                    if not h:
                        continue
                    directive = _format_contract_include(h)
                    if directive:
                        lines.append(f"  - include: `{directive}`")
            if sig:
                lines.append("```c")
                lines.append(f"{sig};")
                lines.append("```")
            if role:
                lines.append(f"  - semantic role: {role}")
            _render_signature_usage_notes(
                lines,
                symbol=str(sym),
                signature=str(sig),
                role=str(role),
            )
            lines.append("")

    if isinstance(surface, Mapping):
        fields = surface.get("struct_fields")
        if isinstance(fields, Mapping) and fields:
            rendered = 0
            lines.append("\n### RTOS struct field allow-list\n")
            lines.append(
                "When using the following platform struct types, access only "
                "the fields listed here. Do not invent nearby field names "
                "from other implementations.\n"
            )
            for struct_name, raw_fields in sorted(fields.items()):
                if rendered >= 12:
                    break
                if not isinstance(raw_fields, Sequence) or isinstance(raw_fields, (str, bytes)):
                    continue
                field_names = [str(f) for f in raw_fields if str(f or "").strip()]
                if not field_names:
                    continue
                rendered += 1
                lines.append(
                    f"- `struct {struct_name}` fields: "
                    + ", ".join(f"`{field}`" for field in field_names)
                )

    lines.extend(_section_c_integration_bindings(rtos_contract))

    templates: List[Mapping[str, Any]] = [
        raw for raw in (rtos_contract.get("transaction_templates") or [])
        if isinstance(raw, Mapping)
    ]
    if templates:
        lines.append("\n### Transaction templates (expected step order)\n")
        lines.append(
            "Each template below was derived from the device's "
            "`read_sequence`. Call these slots in order; "
            "`requires_human=true` flags any template whose step "
            "lacks a bound symbol; mark that step unsupported or escalate.\n"
        )
        for tmpl in templates:
            tid = tmpl.get("template_id", "(no id)")
            deriv = tmpl.get("derivation", "?")
            conf = tmpl.get("confidence", 0.0)
            requires_human = bool(tmpl.get("requires_human"))
            lines.append(
                f"#### template `{tid}` "
                f"(derivation={deriv}, confidence={conf:.2f}"
                + (", requires_human=true" if requires_human else "")
                + ")"
            )
            steps = tmpl.get("steps") or []
            rendered_steps = 0
            for i, step in enumerate(steps, start=1):
                if not isinstance(step, Mapping):
                    continue
                op = step.get("op", "?")
                slot_id = step.get("slot_id")
                bound = step.get("bound_symbol")
                desc = (step.get("description") or "").strip()
                if op == "description" and not slot_id and not bound:
                    continue
                if rendered_steps >= _MAX_STEPS_PER_TEMPLATE:
                    lines.append(
                        f"     ... ({len(steps) - i + 1} more step(s) elided)"
                    )
                    break
                rendered_steps += 1
                head = f"  {i}. **{op}**"
                if slot_id:
                    head += f" - slot `{slot_id}`"
                    if bound:
                        head += f" -> `{bound}`"
                elif bound:
                    head += f" -> `{bound}`"
                lines.append(head)
                if desc:
                    lines.append(f"     - description: {desc[:160]}")
                args = step.get("args") or {}
                if isinstance(args, Mapping):
                    meaningful = {
                        k: v for k, v in args.items()
                        if v not in (None, "", [], {}, "description")
                        and k != "description"
                    }
                    if meaningful:
                        lines.append(
                            "     - args: "
                            + ", ".join(
                                f"{k}={meaningful[k]!r}"
                                for k in sorted(meaningful.keys())
                            )
                        )
            lines.append("")

    return "\n".join(lines)


def _section_d_routing(routing: RoutingResult) -> str:
    lines: List[str] = ["## SECTION D — RUNTIME ROUTING\n"]
    lines.append(f"- runtime_path: **{routing.runtime_path}**")
    lines.append(f"- slave_kind:   **{routing.slave_kind}**")
    if routing.spi_sub_mode:
        lines.append(f"- spi_sub_mode: **{routing.spi_sub_mode}**")
    lines.append(f"- bus_kind:     **{routing.bus_kind}**")
    lines.append(f"- rule_applied: `{routing.rule_applied}` (confidence={routing.confidence:.2f})")
    if routing.warnings:
        lines.append("- warnings:")
        for w in routing.warnings:
            lines.append(f"  - {w}")
    lines.append("")
    lines.append(
        "The evaluator will run your driver against the slave kind above. "
        "Your driver must behave correctly under that bus model."
    )
    if routing.spi_sub_mode == SPI_SUB_STREAM:
        lines.append(
            "Note: stream-mode SPI slave replies with a PRELOADED frame on every "
            "transfer; do NOT assume addressable registers."
        )
        lines.append(
            "For read-only stream devices, every clocked byte is part of the "
            "device response. Do not send a dummy frame first and then read a "
            "second frame; that discards the response. Prefer a receive-only "
            "API or a same-length full-duplex transfer "
            "that stores the received bytes. If the only bound platform API "
            "has separate send and receive lengths, its send length must be 0 for stream "
            "reads."
        )
    elif routing.spi_sub_mode == SPI_SUB_REGISTER:
        lines.append(
            "Note: register-mode SPI slave expects register command bytes "
            "derived from the Device IR / protocol hints. Use the read flag "
            "(commonly 0x80) for reads, and include the multi-byte/burst flag "
            "when SECTION E expected_transactions encode a burst-read command."
        )
        lines.append(
            "For register read transactions, clock the command/register byte "
            "and the response bytes in the same logical SPI exchange. If the "
            "bound API is a full-duplex byte transfer, send a TX buffer whose "
            "first byte is the register command followed by dummy bytes, read "
            "the RX buffer from that same call, and discard the command echo "
            "byte if present. Do not emit a send-only command transfer followed "
            "by a separate receive-only transfer unless the platform contract "
            "explicitly provides a register-transfer helper that preserves the "
            "single transaction."
        )
    elif routing.spi_sub_mode == SPI_SUB_COMMAND:
        lines.append(
            "Note: command-mode SPI slave keys on an opcode byte (`cmd_0xNN` "
            "lookups); multi-byte commands must be emitted as one CS-low burst."
        )
        lines.append(
            "If the Device IR or platform contract describes a full-duplex "
            "command/response transaction, send the command bytes and capture "
            "the response bytes in the SAME SPI exchange. Do not use a "
            "send-then-receive helper that clocks the command first and then "
            "clocks a second receive phase; that creates extra clocks and "
            "shifts result bits. Prefer a same-length tx/rx transfer primitive."
        )
    return "\n".join(lines)


def _tx_get(tx: Any, key: str, default: Any = None) -> Any:
    if isinstance(tx, Mapping):
        return tx.get(key, default)
    return getattr(tx, key, default)


def _format_transaction(tx: ExpectedTransaction) -> str:
    parts = [f"phase={_tx_get(tx, 'phase')}"]
    addr_or_pin = _tx_get(tx, "addr_or_pin")
    write_prefix_any_of = _tx_get(tx, "write_prefix_any_of")
    read_any = _tx_get(tx, "read_any")
    forbid_write_prefix = _tx_get(tx, "forbid_write_prefix", False)
    note = _tx_get(tx, "note")
    if addr_or_pin:
        parts.append(f"addr_or_pin={addr_or_pin}")
    if write_prefix_any_of:
        prefixes = [
            "[" + ", ".join(str(x) for x in p) + "]"
            for p in write_prefix_any_of
        ]
        parts.append("write_prefix_any_of=[" + ", ".join(prefixes) + "]")
    if read_any:
        parts.append("read_any=true")
    if forbid_write_prefix:
        parts.append("forbid_write_prefix=true")
    if note:
        parts.append(f"note={note}")
    return "- " + " | ".join(parts)


def _section_e_expected_transactions(
    expected_transactions: Sequence[ExpectedTransaction],
    repair_context: Optional[Mapping[str, Any]] = None,
) -> str:
    lines: List[str] = [
        "## SECTION E — EXPECTED BUS TRANSACTIONS (mechanical lower bound)\n"
    ]
    selected_transactions = _repair_context_values(
        repair_context, "expected_transactions",
    )
    if selected_transactions:
        expected_transactions = selected_transactions  # type: ignore[assignment]
        lines.append(
            "Repair context: this section renders the transaction subset "
            "selected from the previous failure.\n"
        )
    if not expected_transactions:
        lines.append(
            "*(No mechanical transactions could be derived — either this bus "
            "family does not have a register map or the DeviceIR was too "
            "sparse. You still MUST populate `test_plan.expected_transactions` "
            "with at least one entry that matches your driver's behaviour.)*"
        )
        return "\n".join(lines)
    lines.append(
        "Each line below was derived mechanically from the DeviceIR. Your "
        "driver's actual bus traffic MUST produce every phase/address "
        "combination at least once across its init + read sequences, and your "
        "`test_plan.expected_transactions` MUST be a superset of these lines "
        "(preserve phase + addr_or_pin + write_prefix_any_of semantics; you "
        "may add extras like per-stimulus read-back prefixes).\n"
    )
    for tx in expected_transactions:
        lines.append(_format_transaction(tx))
    return "\n".join(lines)


def _section_f_schema(eval_class: str) -> str:
    hint = build_prompt_schema_hint(eval_class)
    return (
        "## SECTION F — OUTPUT JSON SCHEMA\n\n"
        "Output a single JSON object that validates against the schema below. "
        "No extra top-level keys. All strings must be plain UTF-8 (no "
        "base64, no escapes beyond what JSON requires).\n\n"
        "```json\n"
        f"{hint}\n"
        "```"
    )


def _section_f_plan_schema(eval_class: str) -> str:
    hint = build_plan_schema_hint(eval_class)
    return (
        "## SECTION F — OUTPUT JSON SCHEMA\n\n"
        "Output only `api_contract` and `test_plan`. No driver code. "
        "The object must validate against this schema:\n\n"
        "```json\n"
        f"{hint}\n"
        "```"
    )


def _section_f_frozen_plan(
    frozen_plan: Mapping[str, Any],
    repair_context: Optional[Mapping[str, Any]] = None,
) -> str:
    plan_obj = dict(frozen_plan)
    selected_names = {
        str(name)
        for name in _repair_context_values(repair_context, "failing_stimuli")
    }
    if selected_names and repair_context and repair_context.get("include_selected_test_stimuli"):
        plan_obj = dict(plan_obj)
        test_plan = dict(plan_obj.get("test_plan") or {})
        stimuli = test_plan.get("test_stimuli")
        if isinstance(stimuli, list):
            filtered = [
                row for row in stimuli
                if isinstance(row, Mapping) and str(row.get("name") or "") in selected_names
            ]
            if filtered:
                test_plan["test_stimuli"] = filtered
                plan_obj["test_plan"] = test_plan
                plan_obj["repair_context_note"] = (
                    "Only failure-selected test_stimuli are rendered in this "
                    "repair prompt; the frozen plan hash and adapter contract "
                    "remain unchanged."
                )
    return (
        "## SECTION F — FROZEN API CONTRACT AND TEST PLAN\n\n"
        "The following object is frozen. Your code must implement the "
        "`api_contract` exactly and must pass the rendered `test_plan` "
        "obligations exactly. "
        "Do not rename functions referenced by the contract unless you also "
        "define the exact referenced symbol in driver_source.\n\n"
        "```json\n"
        f"{json.dumps(plan_obj, ensure_ascii=False, indent=2)}\n"
        "```"
    )


def _section_g_driver_code_schema() -> str:
    hint = build_driver_code_schema_hint()
    return (
        "## SECTION G — OUTPUT JSON SCHEMA\n\n"
        "Output only `driver_header` and `driver_source`. Do not include "
        "`api_contract`, `test_plan`, markdown, or prose.\n\n"
        "```json\n"
        f"{hint}\n"
        "```"
    )


def _section_feedback(
    prior_feedback: Optional[str],
    *,
    section: str = "G",
) -> Optional[str]:
    if not prior_feedback:
        return None
    trimmed = prior_feedback.strip()
    if not trimmed:
        return None
    return (
        f"## SECTION {section} — FEEDBACK FROM PRIOR ATTEMPT\n\n"
        "A prior attempt failed verification. Address each issue below in "
        "this response. Do NOT re-introduce any listed regression.\n\n"
        f"{trimmed}"
    )


def _section_g_feedback(prior_feedback: Optional[str]) -> Optional[str]:
    return _section_feedback(prior_feedback, section="G")


def _section_h_feedback(prior_feedback: Optional[str]) -> Optional[str]:
    return _section_feedback(prior_feedback, section="H")


# Public API

def build_synthesis_prompt(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
) -> Tuple[str, str]:
    """Return ``(system_prompt, user_prompt)`` for driver synthesis."""
    if classify_result.eval_class == "":
        raise ValueError("classify_result.eval_class must not be empty")
    if routing.bus_kind == "":
        raise ValueError("routing.bus_kind must not be empty")

    _ = artifact  # Parameter retained for API compatibility.
    sections: List[str] = [
        _section_a_task(device_ir, rtos_contract, classify_result),
        _section_b_device_spec(device_ir),
    ]
    alias_section = _section_b2_channel_alias_map(channel_alias_map)
    if alias_section is not None:
        sections.append(alias_section)
    semantics_section = _section_b3_output_semantics_map(output_semantics_map)
    if semantics_section is not None:
        sections.append(semantics_section)
    sections.extend([
        _section_c_rtos_api(rtos_contract),
        _section_d_routing(routing),
        _section_e_expected_transactions(expected_transactions),
        _section_f_schema(classify_result.eval_class),
    ])
    # Bus-specific code guidance injected into the synthesis prompt.
    if routing.bus_kind == "uart":
        sections.append(_UART_CODE_GUIDANCE)
    elif routing.bus_kind in ("gpio", "gpio_timing", "gpio_pulse", "gpio_oneshot"):
        sections.append(_GPIO_CODE_GUIDANCE)
    feedback = _section_g_feedback(prior_feedback)
    if feedback is not None:
        sections.append(feedback)

    user_prompt = "\n\n---\n\n".join(sections)
    return SYSTEM_PROMPT, user_prompt


def build_contract_test_plan_prompt(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
) -> Tuple[str, str]:
    """build_contract_test_plan_prompt helper."""
    if classify_result.eval_class == "":
        raise ValueError("classify_result.eval_class must not be empty")
    if routing.bus_kind == "":
        raise ValueError("routing.bus_kind must not be empty")

    _ = artifact
    sections: List[str] = [
        _section_a_task(device_ir, rtos_contract, classify_result),
        _section_b_device_spec(device_ir),
    ]
    alias_section = _section_b2_channel_alias_map(channel_alias_map)
    if alias_section is not None:
        sections.append(alias_section)
    semantics_section = _section_b3_output_semantics_map(output_semantics_map)
    if semantics_section is not None:
        sections.append(semantics_section)
    sections.extend([
        _section_c_rtos_api(rtos_contract),
        _section_d_routing(routing),
        _section_e_expected_transactions(expected_transactions),
        _section_f_plan_schema(classify_result.eval_class),
    ])
    feedback = _section_g_feedback(prior_feedback)
    if feedback is not None:
        sections.append(feedback)
    return (
        build_plan_system_prompt(classify_result.eval_class, routing),
        "\n\n---\n\n".join(sections),
    )


def build_driver_code_prompt(
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    frozen_plan: Mapping[str, Any],
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    channel_alias_map: Optional[Mapping[str, Any]] = None,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    prior_feedback: Optional[str] = None,
    repair_context: Optional[Mapping[str, Any]] = None,
) -> Tuple[str, str]:
    """build_driver_code_prompt helper."""
    if classify_result.eval_class == "":
        raise ValueError("classify_result.eval_class must not be empty")
    if routing.bus_kind == "":
        raise ValueError("routing.bus_kind must not be empty")

    _ = artifact
    focus_tags = {
        str(tag)
        for tag in _repair_context_values(repair_context, "focus_tags")
    }
    sections: List[str] = [
        _section_a_task(device_ir, rtos_contract, classify_result),
        _section_b_device_spec(device_ir, repair_context=repair_context),
    ]
    include_alias = (
        not repair_context
        or not focus_tags
        or bool(focus_tags & {"output-semantics", "conversion", "protocol", "test-stimuli"})
    )
    alias_section = _section_b2_channel_alias_map(channel_alias_map) if include_alias else None
    if alias_section is not None:
        sections.append(alias_section)
    include_semantics = (
        not repair_context
        or not focus_tags
        or bool(focus_tags & {"output-semantics", "conversion", "test-stimuli", "fault-handling"})
    )
    semantics_section = (
        _section_b3_output_semantics_map(output_semantics_map)
        if include_semantics else None
    )
    if semantics_section is not None:
        sections.append(semantics_section)
    sections.extend([
        _section_c_rtos_api(rtos_contract, repair_context=repair_context),
        _section_d_routing(routing),
        _section_e_expected_transactions(expected_transactions, repair_context=repair_context),
        _section_f_frozen_plan(frozen_plan, repair_context=repair_context),
        _section_g_driver_code_schema(),
    ])
    feedback = _section_h_feedback(prior_feedback)
    if feedback is not None:
        sections.append(feedback)
    return DRIVER_CODE_SYSTEM_PROMPT, "\n\n---\n\n".join(sections)


__all__ = [
    "DRIVER_CODE_SYSTEM_PROMPT",
    "PLAN_SYSTEM_PROMPT",
    "SYSTEM_PROMPT",
    "build_contract_test_plan_prompt",
    "build_driver_code_prompt",
    "build_plan_system_prompt",
    "build_synthesis_prompt",
]
