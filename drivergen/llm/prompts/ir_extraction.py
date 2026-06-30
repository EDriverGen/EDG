"""Prompts and validators for Device IR extraction."""
from __future__ import annotations

import json
import re
from typing import Any, Mapping, Tuple

from ...core.response_schemas import IR_SCHEMA_VERSION

# ---------------------------------------------------------------------------
# Prompt constants
# ---------------------------------------------------------------------------

ROLE = (
    "You are a datasheet-to-IR extractor for embedded device drivers. "
    "You read structured datasheet content (Markdown headings + tables + prose) "
    "and emit a single JSON object describing how to drive the device. "
    "You never invent register addresses, bus types, or timing values that the "
    "datasheet does not state — when uncertain, leave the field blank or list "
    "the doubt under requires_human."
)

OUTPUT_FORMAT = (
    "Output exactly 1 JSON object — no markdown fences, no prose, no "
    "trailing commas. Top-level keys come from the schema below; do NOT "
    "wrap the object under 'device', 'result', 'data', or 'payload'. "
    "All numeric fields use plain JSON numbers; hex literals appear as "
    "strings (e.g. \"0x48\"). UTF-8 only; no base64. "
    f"If you emit ir_schema_version, set it to the exact string "
    f"'{IR_SCHEMA_VERSION}'; the orchestrator will overwrite the field "
    "either way, but echoing the expected version keeps the output "
    "self-describing."
)

HARD_RULES: Tuple[str, ...] = (
    # Rule 1 - bus_type is the routing root field.
    'Hard rule 1 — bus_type: Set "bus_type" to one of '
    '"i2c" / "spi" / "uart" / "gpio_pulse" / "gpio_timing" / '
    '"display_parallel" / "display_spi" (lowercase). Counter-example: "I2C" or "i2c-bus" '
    '(wrong — case must be exact lowercase, no separators).',

    # Rule 2 - 7-bit I2C addresses plus default metadata.
    'Hard rule 2 — I2C address rule (always 7-bit form, default tagged): '
    'I2C uses 7-bit slave addresses (range 0x08..0x77 inclusive; 0x00..0x07 '
    'and 0x78..0x7F are reserved). Datasheets sometimes quote the 8-bit '
    '"wire byte" instead — the 7-bit address shifted left by 1 with the '
    'data-direction bit (R/W) appended in the LSB. Decide which form the '
    'datasheet uses, then ALWAYS emit the 7-bit form.\n'
    '  Treat the source as 8-bit (right-shift by 1 before emitting) when '
    'ANY of these signals holds:\n'
    '    (a) the numeric value is greater than 0x7F — a 7-bit address '
    'cannot exceed 0x7F.\n'
    '    (b) the datasheet text uses 8-bit-byte vocabulary: "address byte", '
    '"slave address byte", "device address (with R/W)", "wire byte", '
    '"controller-write byte", "the LSB selects read/write", "data '
    'direction bit", "8-bit address", or shows a bit-layout diagram '
    'where the LSB position is labelled R/W (or RW, R/W*).\n'
    '    (c) two distinct values appear together described as "write" and '
    '"read", with the lower one even and the upper exactly 1 greater (any '
    'pair of the form 2k / 2k+1) — these are the same logical address '
    'rendered in 8-bit form for write and read; emit (lower) >> 1 once.\n'
    '  Otherwise, when the value sits in 0x08..0x77 AND the datasheet '
    'talks about strap-pin variants (A2/A1/A0, ADDR, SDO, AD0, CS) or '
    'simply names a "slave address" / "device address" without R/W '
    'qualification, trust it as 7-bit and emit it verbatim. Strap-pin '
    'pairs that legitimately differ by 1 (e.g. A0=GND vs A0=Vcc selecting '
    '0xN6 / 0xN7) DO belong as two separate entries — they are not a '
    'write/read split, just two physical addresses.\n'
    '  After the form decision, attach metadata: every entry MUST carry '
    '"is_default": <bool> with EXACTLY ONE entry true (the silicon '
    'default — all strap pins tied as the datasheet specifies, or the '
    'address the datasheet labels "default" / "factory default" / '
    '"preset"). Do not add "(default)" to an address table row unless the '
    'datasheet uses default/preset wording for that row. If a tri-state '
    'address table has an all-floating/open/NC row and no row is explicitly '
    'labelled default, choose the all-floating row as the unconnected-pin '
    'default and record that reasoning in description/notes. Set '
    '"address_rule.addressing_form": "7-bit" so downstream '
    'consumers can confirm the form.\n'
    '  Counter-example A: emitting a value from a "0xAA (write) / 0xAB '
    '(read)" pair as 0xAA — wrong, that is the 8-bit wire form; emit '
    '0x55 (= 0xAA >> 1).\n'
    '  Counter-example B: emitting a single value 0x84 because the '
    'datasheet states "the device address is 0x84 with the R/W bit in '
    'bit 0" — wrong, that is the 8-bit form by signal (b); emit 0x42.\n'
    '  Counter-example C: leaving every entry without "is_default" — '
    'wrong, downstream cannot tell which strap-pin variant the sandbox '
    'board uses.',

    # Rule 3 - read_channels are public data outputs, not registers.
    'Hard rule 3 — read_channels scope: read_channels lists '
    'public data outputs the driver exposes to its user. For sensors this '
    'means measurement-output channels (e.g. "temperature", "accel_x", '
    '"seconds"). For memory, packet, stream, display-readback, or EEPROM/'
    'FRAM-like devices, include a payload/data channel such as "data", '
    '"data_byte", "data_bytes", "memory_data", "packet_payload", or '
    '"stream_sample" with raw_type "bytes" when variable-length data is '
    'returned. Configuration / setpoint / threshold / status registers are '
    'NOT channels even when readable. '
    'Do not drop public measurement/data channels just because their '
    'conversion formula is complex. Keep the read_channel, bind it to the '
    'raw source bytes or signal, and let the formula row carry '
    'integer_approximation_expression=null with a complex-algorithm status '
    'when necessary. '
    'Measurement/data outputs are channels; configuration, setpoint, '
    'threshold, and status registers are not channels by themselves.',

    # Rule 4 - registers_or_commands must be an array of objects.
    'Hard rule 4 — registers_or_commands: Use a JSON array '
    '(list of objects), NOT a nested dict. Each entry has '
    '{ "name": <datasheet name verbatim, e.g. "Conf", "Temp">, '
    '"value": <pointer/opcode hex string, e.g. "0x01">, '
    '"access": "ro" | "rw" | "wo", '
    '"size_bits": <integer total width, e.g. 8 or 16>, '
    '"description": <short snippet from datasheet> }. '
    'Counter-example: { "configuration": {...}, "temperature": {...} } '
    '(wrong — keys are device-specific labels, downstream tooling '
    'cannot iterate this shape).\n'
    '  RTC NAMING (CONDITIONAL — applies ONLY when the datasheet '
    'describes a real-time clock chip whose primary function is to '
    'track wall-clock time): for the time-of-day / calendar registers, '
    'use canonical snake_case tokens so downstream classify_device '
    'recognises the device as RTC without regex guesswork. Acceptable '
    'tokens (pick one per time field): \'seconds\', \'minutes\', '
    '\'hours\', \'day_of_week\', \'day_of_month\', \'month\', \'year\'. '
    'Counter-example: \'RTC_SEC\', \'TIME_REG\', \'SEC_MIN\', or raw '
    'hex (\'0x00\', \'0x01\') as the register name (wrong — the '
    'address is already in the value field; the name carries semantic '
    'meaning). For control / status / alarm registers keep the '
    'datasheet name verbatim (e.g. CONTROL, STATUS, ALARM1_SECONDS) '
    '— only the seven time-field names above are normative.\n'
    '  NEGATIVE EXAMPLE — DO NOT use the seven RTC tokens for non-RTC '
    'devices: temperature sensors, pressure sensors, light sensors, '
    'accelerometers, EEPROMs, and displays. For non-RTC devices the register name MUST '
    'come verbatim from the datasheet text — never from this rule\'s '
    'token list.',

    # Rule 5 - init_sequence / read_sequence are lists of step objects.
    'Hard rule 5 — init_sequence / read_sequence shape: Both fields are '
    'JSON arrays of step objects. Each step is '
    '{ "transaction": <transaction-object-or-null>, "notes": <prose> }. '
    'A transaction object has '
    '"kind" ∈ {"write","read","write_then_read"}, '
    '"bytes": [<int 0..255 | "0xNN" | "DATA" | null>, ...], '
    'and optional "length" (integer for fixed-size reads, or "DATA" only '
    'for variable-length memory/stream reads), '
    '"pointer_target" (the symbolic register name, e.g. "Temp"). '
    'Use "transaction": null for descriptive-only steps (waits, post-'
    'processing, comments). Counter-example: '
    '{ "step1": "write 0x00", "step2": "read 2 bytes" } '
    '(wrong — must be a list, and each step must use the structured '
    '"transaction" object so downstream tooling can synthesise expected '
    'bus traffic).',

    # Rule 5b - flow-level behaviour, generic across devices.
    'Hard rule 5b - operation_flows and access_model: Extract the '
    'device operation model, not only the register table. Populate '
    '"access_model" with the generic bus access style: '
    '"register_pointer" for devices that require writing a register '
    'pointer before reads, "register_auto_increment" for pointer reads '
    'that can fetch consecutive bytes, "command_then_direct_read" for '
    'command/mode devices where data is read by a plain bus read after '
    'a command has selected the mode, "memory" for addressable EEPROM/'
    'FRAM-like devices, "stream" for clocked streams, "packet" for '
    'framed UART/SPI protocols, "gpio_timing" for pulse/timing sensors, '
    'or "unknown" when the datasheet is ambiguous. Then populate '
    '"operation_flows" with executable flows. Each flow has '
    '{ "flow_id", "kind", "channels", "preconditions", "steps", '
    '"outputs", "requires_human", "notes" }. Flow steps use generic '
    'ops: "write", "read", "write_then_read", "delay", "poll_until", '
    '"wait_until_ready", "select_page", "clear", "postprocess", '
    '"set_signal", "wait_signal", "measure_pulse", or "sample_signal"; '
    'bus-producing steps MUST carry the same structured "transaction" '
    'object as Rule 5. Signal/timing steps MUST name the signal or '
    'source_signal (GPIO pin, echo pulse, interrupt line, ready line, ADC '
    'sample source, or pulse-width measurement) and must not invent fake '
    'bus bytes. Model one physical bus transaction as one bus-producing '
    'step: if a write sends command/address/pointer bytes and then runtime '
    'data in the same transfer, put those slots together in one '
    'transaction.bytes array (for example ["0x00", "0x10", "DATA"]), '
    'rather than splitting each byte into its own write step. Use "DATA" '
    'or null for runtime payload/value bytes; do not put an example value '
    'such as "0x00" when the byte is supplied by the driver at runtime. '
    'For variable-length payloads such as EEPROM page writes, display pixel '
    'data, packet payloads, or streams, use one compact "DATA" placeholder '
    'plus length/page-size notes instead of enumerating dozens of identical '
    '"DATA" entries. '
    'Capture trigger/write, required waits, status polls, '
    'result reads, interrupt clears, page selects, signal edges/pulses, '
    'and calibration reads as separate ordered steps when the datasheet '
    'states them. Completion conditions are generic, not I2C-specific: '
    'if the datasheet states a nonvolatile write/program/erase cycle, '
    'busy/ready flag or pin, ACK polling, conversion time, measurement '
    'time, data-ready flag, end-of-conversion flag, or reset/startup wait, '
    'represent it as "delay", "poll_until", "wait_until_ready", or '
    '"wait_signal" in the relevant flow. Do not continue directly from '
    'trigger/write to result read or next command when the datasheet says '
    'the device is still busy or the result is not ready. For ACK polling, '
    'model the condition that the configured 7-bit device address ACKs; do '
    'not put the 8-bit I2C control/address byte into transaction.bytes. '
    'When the datasheet names writable setup/config/control/mode registers '
    'or command bits for channel enable, oversampling, resolution, sample '
    'rate, filtering, one-shot/continuous/background mode, conversion '
    'start, or sensor power state, preserve that setup decision in the IR: '
    'either add concrete init/probe/read-flow steps for the chosen mode, or '
    'add an explicit precondition/notes entry that power-up defaults require '
    'no bus writes for this flow. If several modes are valid, choose one '
    'conservative executable mode and record the required setup for that '
    'mode; put only unresolved mode choices in requires_human. If you choose '
    'a non-default precision, rate, resolution, filter, or oversampling mode, '
    'show the setup write(s) selecting it; otherwise keep timing/formula scale '
    'assumptions aligned with explicit reset/default settings. Do not drop '
    'a relevant config register just because a result register can be read. '
    'For register-pointer devices, a bus operation that writes a register '
    'address/opcode and immediately reads bytes MUST use op '
    '"write_then_read" with transaction.kind "write_then_read"; reserve '
    'op "read" for a plain bus receive that has no register pointer in '
    'front of it. For I2C, transaction.bytes are payload/register/memory '
    'address/command bytes only; do NOT place the 8-bit slave address, '
    'wire address, control byte, or R/W address byte in transaction.bytes '
    'unless the target bus API explicitly consumes raw on-wire address '
    'bytes. When one public read operation requires trigger/write, '
    'wait/poll, result read, and clear steps, keep those ordered steps in '
    'the same "read" flow instead of splitting the trigger into an '
    'unrelated "other" flow. '
    'When high/low/MSB/LSB result bytes live at non-contiguous register '
    'addresses (for example a high byte at 0x00 and low byte at 0x10), '
    'do NOT model the read as one "write_then_read" from the high-byte '
    'address with length=2 unless the datasheet explicitly states that '
    'auto-increment reaches the non-contiguous low-byte address. Otherwise '
    'represent separate pointer reads or separate write_then_read steps and '
    'bind both bytes in source_bytes / byte_source. '
    'For command-mode direct-read devices, represent the command write '
    'and the later plain read as two separate steps; do NOT rewrite it '
    'as a register-pointer "write_then_read" unless the datasheet says '
    'a pointer/register address is sent immediately before the read. '
    'Counter-example: a range sensor flow that only reads result bytes '
    'but omits the measurement trigger/status poll (wrong - the driver '
    'will read stale or default data); a light sensor that emits '
    '"write_then_read 0x10 length=2" when the datasheet describes '
    '"start measurement command, wait, then direct read" (wrong - that '
    'changes the bus protocol).',

    # Rule 5c - channel/source/formula binding.
    'Hard rule 5c - read channel binding: For every measurement channel '
    'in "read_channels", fill "flow_id" when a flow reads that channel, '
    '"source_bytes" with symbolic returned bytes/register fields for '
    'byte-oriented devices, or "source_signal" with the measured signal '
    'for non-byte devices such as GPIO pulse, interrupt, analog, stream, '
    'or packet-derived measurements. Fill "formula_id" when a conversion '
    'formula applies, even when that formula is marked as a complex '
    'non-executable compensation algorithm. Every operation_flows[*].outputs entry must name a '
    'channel from read_channels and must describe either "byte_source" or '
    '"source_signal" precisely enough for codegen to know which returned '
    'bytes, pulse width, signal edge, packet field, stream sample, or '
    'constant/default setting becomes the raw value. If the device '
    'has a probe or identity flow that reads manufacturer/product/chip ID '
    'registers, keep that probe in operation_flows[*].steps and notes; do '
    'NOT put manufacturer_id, device_id, product_id, or chip_id in '
    'operation_flows[*].outputs unless the same name is intentionally listed '
    'as a public measurement read_channel. If the device '
    'needs calibration/coefficient bytes for compensation, add a '
    'calibration operation flow and reference those bytes from the '
    'formula inputs; if a formula input is a configuration/default value '
    'rather than measured data, bind it with "default_value" or '
    '"config_source" and cite the datasheet text. If the datasheet does '
    'not reveal enough information, put the uncertainty in requires_human '
    'instead of inventing values. '
    'Counter-example: listing pressure and temperature channels but one '
    'global read_sequence that only reads pressure bytes (wrong - each '
    'channel must trace to a flow output or be marked unresolved).',

    # Rule 6 - timing_constraints shape.
    'Hard rule 6 — timing_constraints: Use a JSON array of objects, each '
    '{ "name": <snake_case identifier, e.g. "conversion_time">, '
    '"value": <number — never a range like "1..2">, '
    '"unit": <SI unit string: "ms" | "us" | "ns" | "Hz" | "kHz" | "MHz" | '
    '"V" | "mA" | "degC">, '
    '"condition": <when this applies, e.g. "normal mode" or "after power-up">, '
    '"notes": <verbatim datasheet phrase> }. '
    'Counter-example: { "conversion_time": "100 ms" } '
    '(wrong — keys are device-specific labels, value is a string with the '
    'unit baked in; downstream timing checks need a numeric value plus a '
    'separate unit).',

    # Rule 7 - conversion_formulae include executable expressions when possible.
    'Hard rule 7 — conversion_formulae: Use a JSON array of objects, each '
    '{ "name": <snake_case>, "formula": <prose form>, '
    '"integer_approximation_expression": { "expression": <single-line Python '
    'arithmetic expression accepted by the safe evaluator: no C ternary, no '
    'comparisons, no statements, no semicolons, and no floats when an integer '
    'scale is possible; allowed operators are + - * / // % << >> & | ^ ~ and '
    'parentheses, e.g. "(raw * 125) // 1000">, '
    '"inputs": [ { "name": "raw", "byte_source": "<msb>:8 || <lsb>:8", '
    '"source_signal": null, "default_value": null, "config_source": null, '
    '"description": <prose> } ], '
    '"output": { "name": <snake_case>, "unit": <e.g. "milli_degC"> } } }. '
    'Always emit milli/micro units in the output to keep the codegen path '
    'integer-only. If sign extension is required, prefer an input name such '
    'as raw_signed_11 with byte_source/description explaining the sign bit; '
    'for pulse-width or other non-byte sensors, bind the measured value '
    'with source_signal (for example "echo_pulse_width_us") rather than '
    'inventing source_bytes. The expression output unit must be the actual '
    'computed unit, not only a label: if output.unit is "milli_degC" and '
    'the datasheet says 0.0625 degC/LSB, the expression must multiply by '
    '62.5 milli-degC per count using integer arithmetic (for example '
    '(raw_counts * 625) // 10), not merely return raw_counts. For '
    'two\'s-complement values, avoid ternary expressions; use an integer '
    'sign-extension idiom such as "(((raw >> shift) ^ sign_mask) - '
    'sign_mask) * milli_lsb" and explain sign_mask in inputs/notes. For '
    'register layouts where the high byte is whole degrees and the upper '
    'nibble of the low byte is fractional 0.0625 degC, do NOT pack '
    'high_byte << 8 and scale the whole word by 62.5 milli-degC; that '
    'over-scales the high byte. Use high_byte * 1000 plus '
    '((low_byte >> 4) * 625) // 10, with sign extension if the high byte '
    'is two\'s-complement. Keep '
    'expressions concise (preferably under 500 characters and no deeply '
    'nested parenthesis chains). For vendor compensation algorithms that '
    'are multi-step or branchy, preserve the exact algorithm in prose, '
    'inputs, and operation_flows; set integer_approximation_expression to '
    'null, set "executable_expression_status" to '
    '"complex_compensation_algorithm", and add notes explaining that the '
    'driver must implement the vendor algorithm, rather than emitting an '
    'invalid or unbounded single expression. '
    'Do not write C expressions such as '
    '"cond ? a : b". Counter-example: '
    '"expression": "raw * 0.125", "Temp = raw / 8.0", or '
    '"(raw & 0x800) ? raw - 4096 : raw" '
    '(wrong — float literals will be rejected by the integer-only synthesis '
    'pipeline; rewrite as integer-safe arithmetic over named inputs).',

    # Rule 8 - raw_encoding describes raw-byte assembly.
    'Hard rule 8 — raw_encoding: Single object describing how to assemble the '
    'raw integer that conversion_formulae feeds on. Required keys: '
    '"byte_order" ∈ {"big_endian","little_endian","single_byte"}, '
    '"bit_width" (total register width in bits, e.g. 16 for 2-byte regs), '
    '"signed" (boolean — whether the raw is two\'s complement). '
    'Optional but strongly recommended: '
    '"effective_bits" (real significant width, e.g. 11 when 5 LSB are '
    'padding zeros), '
    '"right_shift" (how many bits to drop from the LSB before using, e.g. 5), '
    '"sign_extend_from_bit" (the sign bit position after right_shift, '
    'e.g. 10 for an 11-bit signed value). '
    'Counter-example: { "format": "11-bit two\'s complement, 0.125 degC/LSB" } '
    '(wrong — single prose string; downstream codegen must read structured '
    'integer fields to emit C bit operations).',

    # Rule 9 - evidence_spans must trace back to the datasheet.
    'Hard rule 9 — evidence_spans: A JSON array of citation objects. Each '
    'entry is { "source_id": <device_id>, '
    '"page": <integer page number, 1-based, taken from the structured '
    'content if available>, '
    '"snippet": <verbatim phrase, ≤ 200 chars, copied character-for-'
    'character from the datasheet text> }. '
    'Cite at least one snippet per non-trivial extracted fact (address '
    'rule, register table row, power-up timing, formula). '
    'Counter-example: { "page": 12, "evidence": "register table" } '
    '(wrong — must be wrapped in the array, key is "snippet" not '
    '"evidence", and the value must be a quoted phrase from the source '
    'rather than a paraphrase).',

    # Rule 10 — error_conditions ↔ requires_human SCOPE BOUNDARY
    # Two disjoint fields: runtime branches vs ambiguity escalation.
    # Keep the scope boundary explicit for ambiguous datasheet facts.
    'Hard rule 10 — error_conditions vs requires_human SCOPE BOUNDARY: '
    'these two fields have disjoint responsibilities — do NOT mix them.\n'
    '  - error_conditions: runtime-detectable faults the generated '
    'driver MUST handle at runtime. Each entry describes an observable '
    'condition (I2C NACK, CRC mismatch, measurement timeout, range '
    'overflow, reserved bit set, …), its deterministic detection rule, '
    'and a concrete driver_action (\'return -EIO\', \'abort transaction '
    'and report\', \'retry once then surface error\'). error_conditions '
    'is NEVER the place to log datasheet ambiguity.\n'
    '  - requires_human: datasheet ambiguities or unresolved review '
    'items — things the driver CANNOT branch on at runtime (e.g. '
    '"datasheet timing table unit is rendered as \'m s\', unclear '
    'whether millisecond or microsecond — please confirm", "the '
    'reserved-bit default is not stated — assumed 0", "two alternative '
    'init sequences are given (§6.2 vs §7.1) — confirm which applies"). '
    'requires_human is NEVER the place to put runtime-detectable '
    'errors.\n'
    '  Rule of thumb: if the driver must REACT at runtime, put it in '
    'error_conditions; if a HUMAN must approve a decision before '
    'shipping, put it in requires_human. An entry MUST live in '
    'exactly one bucket. Counter-example: filing "datasheet AC timing '
    'table unit rendered as \'m s\'" under error_conditions[*].'
    'driver_action (wrong — that\'s ambiguity, not a runtime fault; '
    'it belongs in requires_human). If returned data includes CRC, '
    'checksum, PEC, parity, or another integrity byte/bit, preserve the '
    'returned integrity field in the flow/source binding and add an '
    'error_conditions entry describing the runtime mismatch check and '
    'driver_action. This applies to any bus or packet/stream protocol, not '
    'only I2C sensor reads.',
)

PRIMARY_INTERFACE_RULE = (
    'Hard rule 2P - target primary interface and family variant: The user '
    'task names one target device and one target bus/interface. Emit an '
    'executable IR for that concrete combination only. If the datasheet is a '
    'family datasheet, keep facts for the requested part number and drop '
    'channels, flows, registers, and formulas that the text marks as '
    'available only on sibling variants. If the device exposes alternate '
    'interfaces or outputs (for example UART plus analog/PWM, or I2C plus '
    'SPI), keep the interface selected by Target bus/interface and put only '
    'unresolved target-interface decisions in requires_human. Do not add '
    'alternate-interface public read_channels just because they appear in the '
    'same datasheet.'
)

ROUTED_OPERATION_FLOW_RULE = (
    'Hard rule 5b-routed - operation_flows and access_model: Extract the '
    'device operation model, not only the register table. Populate '
    '"access_model" with the access style selected by the target bus rule, '
    'or "unknown" only when the datasheet is ambiguous. Then populate '
    '"operation_flows" with executable flows. Each flow has '
    '{ "flow_id", "kind", "channels", "preconditions", "steps", '
    '"outputs", "requires_human", "notes" }. Flow steps use the generic '
    'ops named by the selected bus rule plus "delay", "poll_until", '
    '"wait_until_ready", "clear", "postprocess", and "select_page" when '
    'the datasheet states them. Bus-producing steps MUST carry the same '
    'structured "transaction" object as Rule 5. Signal/timing steps MUST '
    'name the signal or source_signal precisely and must not invent fake '
    'bus bytes. Model one physical bus transaction as one bus-producing '
    'step: if a write sends command/address/pointer bytes and then runtime '
    'data in the same transfer, put those slots together in one '
    'transaction.bytes array rather than splitting each byte into its own '
    'write step. Use "DATA" or null for runtime payload/value bytes; do '
    'not put an example value such as "0x00" when the byte is supplied by '
    'the driver at runtime. For variable-length payloads such as EEPROM '
    'page writes, display pixel data, packet payloads, or streams, use one '
    'compact "DATA" placeholder plus length/page-size notes instead of '
    'enumerating dozens of identical "DATA" entries. Preserve trigger/write, required waits, status '
    'polls, result reads, interrupt clears, page selects, signal edges/'
    'pulses, and calibration reads as separate ordered steps when the '
    'datasheet states them. Do not continue directly from trigger/write '
    'to result read or the next command when the datasheet says the device '
    'is still busy or the result is not ready. When the datasheet names '
    'writable setup/config/control/mode registers or command bits for '
    'channel enable, oversampling, resolution, sample rate, filtering, '
    'one-shot/continuous/background mode, conversion start, or sensor power '
    'state, preserve that setup decision in the IR: either add concrete '
    'init/probe/read-flow steps for the chosen mode, or add an explicit '
    'precondition/notes entry that power-up defaults require no writes for '
    'this flow. If several modes are valid, choose one conservative '
    'executable mode and record the required setup for that mode; put only '
    'unresolved mode choices in requires_human. If you choose a non-default '
    'precision, rate, resolution, filter, or oversampling mode, show the '
    'setup write(s) selecting it; otherwise keep timing/formula scale '
    'assumptions aligned with explicit reset/default settings. Do not drop '
    'a relevant config register just because a result register can be read. '
    'When one public read operation requires trigger/write, wait/poll, '
    'result read, and clear steps, keep those ordered steps in the same '
    '"read" flow instead of splitting the trigger into an unrelated "other" '
    'flow. Counter-example: a range sensor flow that only reads result '
    'bytes but omits the measurement trigger/status poll (wrong - the '
    'driver will read stale or default data).'
)

BUS_SPECIFIC_RULES: Mapping[str, Tuple[str, ...]] = {
    "i2c": (
        HARD_RULES[1],
        'I2C-specific extraction rule: model register-pointer reads with '
        'op "write_then_read" and transaction.kind "write_then_read" when the '
        'datasheet sends a register/pointer byte immediately before the read. '
        'Use transaction.bytes for payload/register/command bytes only; never '
        'put the raw 8-bit on-wire slave address/control byte there. Keep '
        'ACK-polling as a poll_until/wait_until_ready condition on the '
        'configured 7-bit address. For register-pointer devices, reserve op '
        '"read" for a plain bus receive that has no register pointer in front '
        'of it. When high/low/MSB/LSB result bytes live at non-contiguous '
        'register addresses, do NOT model the read as one "write_then_read" '
        'from the high-byte address with length=2 unless the datasheet '
        'explicitly states that auto-increment reaches the non-contiguous '
        'low-byte address. For command-mode direct-read devices, represent '
        'the command write and the later plain read as two separate steps; '
        'do NOT rewrite it as a register-pointer "write_then_read" unless '
        'the datasheet says a pointer/register address is sent immediately '
        'before the read. If a timing diagram shows ST | Slave Address | '
        'R/W | ACK | opcode/register/payload, transaction.bytes must contain '
        'only the opcode/register/payload fields. Do not copy the 7-bit slave '
        'address, 8-bit address byte, write byte, or read byte into '
        'transaction.bytes. For I2C EEPROM/FRAM memory devices, '
        'transaction.bytes starts with runtime memory word-address bytes '
        '(use null placeholders when the driver supplies them) followed by '
        'runtime DATA payload; never include the 1010xxx slave/control byte '
        'as a payload byte. Use length="DATA" for variable-length sequential '
        'reads when the caller chooses how many bytes to read.',
    ),
    "spi": (
        'SPI-specific extraction rule: model CS/SCK/MOSI/MISO transfers, '
        'command bytes, register addresses, read/write mask bits, burst/auto-'
        'increment bits, and required dummy bytes exactly as the datasheet '
        'states. Do not invent I2C address_rule fields for an SPI task. If a '
        'read command embeds a read/write bit or multi-byte bit in the opcode, '
        'record the mask semantics in registers_or_commands, operation_flows, '
        'or raw_encoding notes so codegen can build the opcode generically. '
        'Use access_model "register_pointer" or "register_auto_increment" '
        'for register-style SPI devices, "stream" for clocked data streams, '
        'or "packet" for framed SPI protocols. Preserve dummy clocks/bytes '
        'and chip-select lifetime when the datasheet states them. When '
        'multi-byte fields live at non-contiguous registers, do not model '
        'them as one auto-increment burst unless the datasheet explicitly '
        'states that the burst reaches those addresses.',
    ),
    "uart": (
        'UART-specific extraction rule: model framed serial protocols as '
        'packet/stream operations. Preserve baud rate, parity/stop-bit '
        'requirements when stated, fixed packet length or delimiter, command '
        'bytes, response layout, checksum/CRC/parity fields, and timeout '
        'rules. Do not turn UART packets into fake register-pointer reads and '
        'do not expose analog/PWM alternate outputs as read_channels for a '
        'UART task. Use access_model "packet" for command/response frames or '
        '"stream" for continuous serial output. In operation_flows, represent '
        'each emitted command frame and each expected response frame '
        'separately enough that codegen can preserve ordering, timeouts, and '
        'integrity checks.',
    ),
    "gpio": (
        'GPIO/timing-specific extraction rule: model signals, levels, pulses, '
        'edges, trigger width, echo/pulse measurement, timeout, and required '
        'microsecond delays with set_signal, wait_signal, measure_pulse, '
        'sample_signal, delay, or poll_until steps. Bind true pulse-width, '
        'edge-count, level, interrupt, or analog-style outputs through '
        'source_signal. When the GPIO/timing protocol carries a compact byte '
        'frame or scratchpad over timed bits, preserve the returned frame '
        'layout with source_bytes and operation_flows[*].outputs[*].byte_source '
        'instead of collapsing it to source_signal. Do not reinterpret a '
        'two-byte frame field as separate integral/decimal bytes unless the '
        'datasheet explicitly says those bytes are integral and fractional '
        'decimal fields; if the datasheet says high/low byte, MSB/LSB, x10 '
        'raw count, sign bit, sign-and-magnitude, checksum, or CRC, keep those '
        'facts in source byte names, formula inputs, and postprocess notes. '
        'For fixed byte frames, keep frame length, byte order, checksum/CRC '
        'byte position, and checksum arithmetic generic and executable enough '
        'for codegen to build a payload vector. Do not invent I2C/SPI/UART '
        'transactions for a GPIO timing task. Use access_model "gpio_timing" '
        'for pulse-width/timing sensors and describe trigger and measured '
        'signals with stable names such as trig, echo, data, irq, ready, or '
        'source_signal values copied from the datasheet. Preserve timeout and '
        'minimum/maximum pulse-width constraints because downstream tests use '
        'them to decide whether a zero measurement is a driver bug or a valid '
        'no-echo condition.',
    ),
    "display_parallel": (
        'Display-parallel-specific extraction rule: model command/data bus '
        'writes, data/command select, write/read strobes, reset timing, '
        'initialisation command tables, address-window setup, pixel payload '
        'format, and required delays exactly as the datasheet states. Do not '
        'invent I2C/SPI/UART transactions for a parallel-display task. Use '
        'access_model "packet" or "stream" when the interface is command/'
        'payload oriented, and keep framebuffer/pixel data as runtime DATA '
        'payload rather than example bytes.',
    ),
}


def _normalise_target_bus_type(target_bus_type: str = "") -> str:
    text = str(target_bus_type or "").strip().lower()
    if not text or text == "unknown":
        return ""
    if text.startswith("smbus") or text.startswith("i2c"):
        return "i2c"
    if text.startswith("spi") or text == "display_spi":
        return "spi"
    if text.startswith("uart") or text.startswith("serial"):
        return "uart"
    if text.startswith("gpio") or text in {"gpio_pulse", "gpio_timing"}:
        return "gpio"
    if text in {"display_parallel", "parallel", "8080", "6800"}:
        return "display_parallel"
    return text.split("_", 1)[0].split("-", 1)[0]


def rules_for_target_bus(target_bus_type: str = "") -> Tuple[str, ...]:
    """Return common IR rules plus the selected bus appendix."""
    family = _normalise_target_bus_type(target_bus_type)
    if not family or family not in BUS_SPECIFIC_RULES:
        return HARD_RULES
    return (
        HARD_RULES[0],
        PRIMARY_INTERFACE_RULE,
        *BUS_SPECIFIC_RULES[family],
        HARD_RULES[2],
        HARD_RULES[3],
        HARD_RULES[4],
        ROUTED_OPERATION_FLOW_RULE,
        HARD_RULES[6],
        HARD_RULES[7],
        HARD_RULES[8],
        HARD_RULES[9],
        HARD_RULES[10],
        HARD_RULES[11],
    )

# ---------------------------------------------------------------------------
# Output validation helpers
# ---------------------------------------------------------------------------

_VALID_BUS_TYPES = frozenset(
    {"i2c", "spi", "uart", "gpio_pulse", "gpio_timing", "display_parallel", "display_spi"}
)

_SETPOINT_TOKENS = frozenset(
    {
        "config", "ctrl", "mode", "status", "who_am_i",
        "thyst", "t_hyst", "hyst",
        "tos", "t_os", "t_overshutdown",
        "tlow", "t_low", "thigh", "t_high",
        "alarm_threshold", "limit_high", "limit_low", "setpoint",
        "int_enable", "fifo_ctrl",
    }
)

_VALID_TRANSACTION_KINDS = frozenset({"write", "read", "write_then_read"})

_VALID_TIMING_UNITS = frozenset(
    {"ms", "us", "ns", "s", "Hz", "kHz", "MHz", "V", "mV", "mA", "uA", "degC"}
)

_VALID_BYTE_ORDERS = frozenset({"big_endian", "little_endian", "single_byte"})

_VALID_ACCESS_MODEL_KINDS = frozenset({
    "register_pointer",
    "register_auto_increment",
    "command_then_direct_read",
    "memory",
    "stream",
    "packet",
    "gpio_timing",
    "unknown",
})

_VALID_FLOW_KINDS = frozenset({
    "init",
    "probe",
    "read",
    "calibration",
    "write",
    "power",
    "other",
})

_VALID_FLOW_STEP_OPS = frozenset({
    "write",
    "read",
    "write_then_read",
    "delay",
    "poll_until",
    "wait_until_ready",
    "select_page",
    "clear",
    "postprocess",
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
})

_SIGNAL_FLOW_STEP_OPS = frozenset({
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
})

_DEFAULT_ONLY_INIT_RE = re.compile(
    r"\b(defaults?|reset\s+state|no\s+bus\s+(?:init|initiali[sz]ation|write)|"
    r"no\s+explicit\s+(?:config|configuration|setup|write)|"
    r"config(?:uration)?\s+writes?\s+(?:is|are)\s+not\s+required)\b",
    re.IGNORECASE,
)


def detect_violations(output: Any) -> list[str]:
    """Return human-readable violation messages; empty list = compliant."""
    if not isinstance(output, Mapping):
        return [
            f"top-level output must be a JSON object (got {type(output).__name__})"
        ]

    violations: list[str] = []

    # Rule 1 — bus_type
    bus_type = output.get("bus_type")
    if not isinstance(bus_type, str):
        violations.append("bus_type: missing or not a string")
    elif bus_type not in _VALID_BUS_TYPES:
        violations.append(
            f"bus_type: {bus_type!r} not in allowed set "
            f"{sorted(_VALID_BUS_TYPES)}"
        )

    # Rule 2 — I2C 7-bit address
    if bus_type == "i2c":
        addr_rule = output.get("address_rule") or {}
        addresses = []
        if isinstance(addr_rule, Mapping):
            raw = addr_rule.get("addresses", [])
            if isinstance(raw, list):
                for item in raw:
                    if isinstance(item, Mapping):
                        a = item.get("address") or item.get("value")
                    else:
                        a = item
                    if a is None:
                        continue
                    addresses.append(a)
        for a in addresses:
            n = _to_int(a)
            if n is not None and n > 0x7F:
                violations.append(
                    f"address_rule: {a!r} > 0x7F — looks like an 8-bit "
                    "wire address, expected 7-bit"
                )

    # Rule 3 — read_channels not setpoints
    channels = output.get("read_channels") or []
    if not isinstance(channels, list):
        violations.append("read_channels: must be a list")
    else:
        for i, ch in enumerate(channels):
            cid = ""
            if isinstance(ch, Mapping):
                cid = str(ch.get("id") or "").strip().lower()
            else:
                cid = str(ch).strip().lower()
            cid_norm = re.sub(r"[^a-z0-9]+", "_", cid).strip("_")
            if cid_norm in _SETPOINT_TOKENS:
                violations.append(
                    f"read_channels[{i}].id={cid!r}: this is a setpoint / "
                    "config register, not a measurement channel"
                )

    # Rule 4 — registers_or_commands is a list of {name, value, access, ...}
    regs = output.get("registers_or_commands")
    if regs is not None:
        if not isinstance(regs, list):
            violations.append(
                "registers_or_commands: must be a JSON array (list of "
                f"objects), got {type(regs).__name__}"
            )
        else:
            for i, r in enumerate(regs):
                if not isinstance(r, Mapping):
                    violations.append(
                        f"registers_or_commands[{i}]: must be an object, "
                        f"got {type(r).__name__}"
                    )
                    continue
                missing = [k for k in ("name", "value") if k not in r]
                if missing:
                    violations.append(
                        f"registers_or_commands[{i}]: missing required key(s) "
                        f"{missing}"
                    )

    # Rule 5 — init_sequence / read_sequence are lists of step objects
    for fname in ("init_sequence", "read_sequence"):
        violations.extend(_check_sequence(fname, output.get(fname)))

    violations.extend(_check_access_model(output.get("access_model")))
    violations.extend(_check_operation_flows(output))

    # Rule 6 — timing_constraints
    violations.extend(_check_timing_constraints(output.get("timing_constraints")))

    # Rule 7 — conversion_formulae
    violations.extend(_check_conversion_formulae(output.get("conversion_formulae")))

    # Rule 8 — raw_encoding
    violations.extend(_check_raw_encoding(output.get("raw_encoding")))

    # Rule 9 — evidence_spans
    violations.extend(_check_evidence_spans(output.get("evidence_spans")))

    # Rule 10 — error_conditions ↔ requires_human SCOPE BOUNDARY
    violations.extend(_check_error_conditions_scope(output.get("error_conditions")))

    return violations


def _check_sequence(field: str, value: Any) -> list[str]:
    """Validate an init_sequence / read_sequence value."""
    if value is None:
        return []
    if not isinstance(value, list):
        return [
            f"{field}: must be a JSON array (list of step objects), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for i, step in enumerate(value):
        if not isinstance(step, Mapping):
            out.append(
                f"{field}[{i}]: each step must be an object with a "
                f"'transaction' key, got {type(step).__name__}"
            )
            continue
        if "transaction" not in step:
            out.append(
                f"{field}[{i}]: missing required 'transaction' key "
                "(use null for descriptive-only steps)"
            )
            continue
        tx = step["transaction"]
        if tx is None:
            continue
        if not isinstance(tx, Mapping):
            out.append(
                f"{field}[{i}].transaction: must be an object or null, "
                f"got {type(tx).__name__}"
            )
            continue
        kind = tx.get("kind")
        if kind not in _VALID_TRANSACTION_KINDS:
            out.append(
                f"{field}[{i}].transaction.kind={kind!r}: must be one of "
                f"{sorted(_VALID_TRANSACTION_KINDS)}"
            )
        b = tx.get("bytes")
        if b is not None and not isinstance(b, list):
            out.append(
                f"{field}[{i}].transaction.bytes: must be an array or "
                f"null, got {type(b).__name__}"
            )
    return out


def _check_access_model(value: Any) -> list[str]:
    """Validate access_model shape for repair feedback."""
    if value is None:
        return []
    if not isinstance(value, Mapping):
        return [
            "access_model: must be an object or null, "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    kind = value.get("kind")
    if kind not in _VALID_ACCESS_MODEL_KINDS:
        out.append(
            f"access_model.kind={kind!r}: must be one of "
            f"{sorted(_VALID_ACCESS_MODEL_KINDS)}"
        )
    for key in ("read_requires_pointer", "direct_read_after_write"):
        flag = value.get(key)
        if flag is not None and not isinstance(flag, bool):
            out.append(f"access_model.{key}: must be boolean or null")
    address_bytes = value.get("address_bytes")
    if address_bytes is not None:
        if (
            not isinstance(address_bytes, int)
            or isinstance(address_bytes, bool)
            or address_bytes < 0
            or address_bytes > 4
        ):
            out.append("access_model.address_bytes: must be an integer in [0,4] or null")
    return out


def _read_channel_ids_from_output(output: Mapping[str, Any]) -> set[str]:
    channels = output.get("read_channels") or []
    ids: set[str] = set()
    if not isinstance(channels, list):
        return ids
    for ch in channels:
        cid = ""
        if isinstance(ch, Mapping):
            cid = str(ch.get("id") or "").strip()
        else:
            cid = str(ch or "").strip()
        if cid:
            ids.add(cid)
    return ids


def _check_flow_transaction(field: str, tx: Any) -> list[str]:
    if tx is None:
        return []
    if not isinstance(tx, Mapping):
        return [f"{field}: must be an object or null, got {type(tx).__name__}"]
    out: list[str] = []
    kind = tx.get("kind")
    if kind not in _VALID_TRANSACTION_KINDS:
        out.append(
            f"{field}.kind={kind!r}: must be one of "
            f"{sorted(_VALID_TRANSACTION_KINDS)}"
        )
    bytes_value = tx.get("bytes")
    if bytes_value is not None and not isinstance(bytes_value, list):
        out.append(
            f"{field}.bytes: must be an array or null, "
            f"got {type(bytes_value).__name__}"
        )
    return out


def _check_operation_flows(output: Mapping[str, Any]) -> list[str]:
    """Validate operation_flows shape for repair feedback."""
    flows = output.get("operation_flows")
    if flows is None:
        return []
    if not isinstance(flows, list):
        return [
            "operation_flows: must be a JSON array (list of flow objects), "
            f"got {type(flows).__name__}"
        ]

    out: list[str] = []
    known_channels = _read_channel_ids_from_output(output)
    flow_ids: set[str] = set()
    covered_channels: set[str] = set()

    for i, flow in enumerate(flows):
        prefix = f"operation_flows[{i}]"
        if not isinstance(flow, Mapping):
            out.append(f"{prefix}: must be an object, got {type(flow).__name__}")
            continue

        flow_id = str(flow.get("flow_id") or "").strip()
        if not flow_id:
            out.append(f"{prefix}.flow_id: must be a non-empty string")
        elif flow_id in flow_ids:
            out.append(f"{prefix}.flow_id={flow_id!r}: duplicate flow_id")
        else:
            flow_ids.add(flow_id)

        kind = flow.get("kind")
        if kind not in _VALID_FLOW_KINDS:
            out.append(
                f"{prefix}.kind={kind!r}: must be one of "
                f"{sorted(_VALID_FLOW_KINDS)}"
            )

        channels = flow.get("channels")
        if not isinstance(channels, list):
            out.append(f"{prefix}.channels: must be an array")
        else:
            for j, channel in enumerate(channels):
                cid = str(channel or "").strip()
                if cid and known_channels and cid not in known_channels:
                    out.append(f"{prefix}.channels[{j}]={cid!r}: unknown read_channel id")

        outputs = flow.get("outputs")
        if not isinstance(outputs, list):
            out.append(f"{prefix}.outputs: must be an array")
        else:
            for j, item in enumerate(outputs):
                if not isinstance(item, Mapping):
                    out.append(f"{prefix}.outputs[{j}]: must be an object")
                    continue
                cid = str(item.get("channel") or "").strip()
                if not cid:
                    out.append(f"{prefix}.outputs[{j}].channel: must be a non-empty string")
                    continue
                if known_channels and cid not in known_channels:
                    out.append(f"{prefix}.outputs[{j}].channel={cid!r}: unknown read_channel id")
                covered_channels.add(cid)

        requires_human = flow.get("requires_human")
        if requires_human is not None and not isinstance(requires_human, bool):
            out.append(f"{prefix}.requires_human: must be boolean or null")

        steps = flow.get("steps")
        has_producer_step = False
        if not isinstance(steps, list):
            out.append(f"{prefix}.steps: must be an array")
            continue
        for j, step in enumerate(steps):
            step_prefix = f"{prefix}.steps[{j}]"
            if not isinstance(step, Mapping):
                out.append(f"{step_prefix}: must be an object")
                continue
            op = step.get("op")
            if op not in _VALID_FLOW_STEP_OPS:
                out.append(
                    f"{step_prefix}.op={op!r}: must be one of "
                    f"{sorted(_VALID_FLOW_STEP_OPS)}"
                )

            tx = step.get("transaction")
            out.extend(_check_flow_transaction(f"{step_prefix}.transaction", tx))
            if isinstance(tx, Mapping) and tx.get("kind") in _VALID_TRANSACTION_KINDS:
                has_producer_step = True

            if op in _VALID_TRANSACTION_KINDS:
                if not isinstance(tx, Mapping):
                    out.append(f"{step_prefix}: op={op!r} requires a transaction object")
                elif tx.get("kind") != op:
                    out.append(
                        f"{step_prefix}: op={op!r} must match transaction.kind="
                        f"{tx.get('kind')!r}"
                    )
            elif op in {"poll_until", "clear", "select_page"}:
                register = str(step.get("register") or "").strip()
                if tx is None and not register:
                    out.append(f"{step_prefix}: op={op!r} requires transaction or register")
            elif op in _SIGNAL_FLOW_STEP_OPS:
                if not _step_has_signal_reference(step):
                    out.append(
                        f"{step_prefix}: op={op!r} requires signal, source_signal, "
                        "output_ref, or condition"
                    )
                else:
                    has_producer_step = True

        if kind in {"probe", "read", "calibration"} and not requires_human and not has_producer_step:
            out.append(f"{prefix}: {kind!r} flow must contain at least one bus/signal-producing step")
        if (
            kind == "init"
            and not requires_human
            and not has_producer_step
            and not _flow_declares_default_only_init(flow)
        ):
            out.append(
                f"{prefix}: init flow must contain at least one bus/signal-producing "
                "step or explicitly state default/no-bus initialization"
            )
        if kind == "read" and not requires_human and isinstance(channels, list):
            output_channels = {
                str(item.get("channel") or "").strip()
                for item in outputs
                if isinstance(item, Mapping)
            } if isinstance(outputs, list) else set()
            missing_outputs = [
                str(channel or "").strip()
                for channel in channels
                if str(channel or "").strip()
                and str(channel or "").strip() not in output_channels
            ]
            if missing_outputs:
                out.append(
                    f"{prefix}.outputs: read flow declares channel(s) "
                    f"{missing_outputs} but does not output them"
                )

    if known_channels:
        for ch in output.get("read_channels") or []:
            if not isinstance(ch, Mapping):
                continue
            cid = str(ch.get("id") or "").strip()
            if not cid:
                continue
            bound_flow = str(ch.get("flow_id") or "").strip()
            if bound_flow and bound_flow not in flow_ids:
                out.append(f"read_channels[{cid!r}].flow_id={bound_flow!r}: unknown flow_id")
            if cid not in covered_channels and (not bound_flow or bound_flow not in flow_ids):
                out.append(
                    f"read_channels[{cid!r}]: operation_flows must output this channel "
                    "or read_channels[].flow_id must reference a declared flow"
                )
    return out


def _step_has_signal_reference(step: Mapping[str, Any]) -> bool:
    return any(
        isinstance(step.get(key), str) and str(step.get(key)).strip()
        for key in ("signal", "source_signal", "output_ref", "condition")
    )


def _flow_declares_default_only_init(flow: Mapping[str, Any]) -> bool:
    parts: list[str] = [
        str(flow.get("flow_id") or ""),
        str(flow.get("notes") or ""),
    ]
    for step in flow.get("steps") or []:
        if isinstance(step, Mapping):
            parts.extend(
                str(step.get(key) or "")
                for key in ("role", "condition", "notes")
            )
    return bool(_DEFAULT_ONLY_INIT_RE.search(" ".join(parts)))


def _check_timing_constraints(value: Any) -> list[str]:
    """Validate timing_constraints shape (list of {name, value, unit, ...})."""
    if value is None:
        return []
    if not isinstance(value, list):
        return [
            "timing_constraints: must be a JSON array (list of objects), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for i, item in enumerate(value):
        if not isinstance(item, Mapping):
            out.append(
                f"timing_constraints[{i}]: must be an object, "
                f"got {type(item).__name__}"
            )
            continue
        for key in ("name", "value", "unit"):
            if key not in item:
                out.append(
                    f"timing_constraints[{i}]: missing required key {key!r}"
                )
        v = item.get("value")
        if v is not None and (
            isinstance(v, bool) or not isinstance(v, (int, float))
        ):
            out.append(
                f"timing_constraints[{i}].value={v!r}: must be a JSON number "
                "(no string/range/units baked in)"
            )
        unit = item.get("unit")
        if isinstance(unit, str) and unit not in _VALID_TIMING_UNITS:
            out.append(
                f"timing_constraints[{i}].unit={unit!r}: not in allowed "
                f"set {sorted(_VALID_TIMING_UNITS)} (extend _VALID_TIMING_UNITS "
                "if a new unit is genuinely needed)"
            )
    return out


# Float literal pattern: catches "0.125", ".5", "1e-3", etc., but tolerates
# things like "0x123" (hex) and "100" (integer literal).  We deliberately keep
# this simple — false negatives are fine, false positives are not.
_FLOAT_LITERAL_RE = re.compile(
    r"""
    (?<![A-Za-z0-9_])              # not part of an identifier
    (?:
        \d+\.\d*                    # 1. / 1.5 / 1.
      | \.\d+                       # .5
      | \d+(?:\.\d+)?[eE][+-]?\d+   # 1e-3 / 1.5e10
    )
    """,
    re.VERBOSE,
)


def _check_conversion_formulae(value: Any) -> list[str]:
    """Validate conversion_formulae shape and integer-only expressions."""
    if value is None:
        return []
    if not isinstance(value, list):
        return [
            "conversion_formulae: must be a JSON array (list of objects), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for i, item in enumerate(value):
        if not isinstance(item, Mapping):
            out.append(
                f"conversion_formulae[{i}]: must be an object, "
                f"got {type(item).__name__}"
            )
            continue
        expr_block = item.get("integer_approximation_expression")
        if expr_block is None:
            out.append(
                f"conversion_formulae[{i}]: missing required key "
                "'integer_approximation_expression'"
            )
            continue
        if not isinstance(expr_block, Mapping):
            out.append(
                f"conversion_formulae[{i}].integer_approximation_expression: "
                f"must be an object, got {type(expr_block).__name__}"
            )
            continue
        for key in ("expression", "inputs", "output"):
            if key not in expr_block:
                out.append(
                    f"conversion_formulae[{i}]"
                    f".integer_approximation_expression: missing required "
                    f"key {key!r}"
                )
        expr = expr_block.get("expression")
        if isinstance(expr, str) and _FLOAT_LITERAL_RE.search(expr):
            out.append(
                f"conversion_formulae[{i}]"
                f".integer_approximation_expression.expression={expr!r}: "
                "contains a float literal — rewrite as integer arithmetic "
                "(e.g. multiply by 1000 instead of 0.001) so the codegen "
                "pipeline can emit C integer code"
            )
    return out


def _check_raw_encoding(value: Any) -> list[str]:
    """Validate raw_encoding shape (single object with structured fields)."""
    if value is None:
        return []
    if not isinstance(value, Mapping):
        return [
            "raw_encoding: must be a single JSON object (or null), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for key in ("byte_order", "bit_width", "signed"):
        if key not in value:
            out.append(f"raw_encoding: missing required key {key!r}")
    bo = value.get("byte_order")
    if isinstance(bo, str) and bo not in _VALID_BYTE_ORDERS:
        out.append(
            f"raw_encoding.byte_order={bo!r}: must be one of "
            f"{sorted(_VALID_BYTE_ORDERS)}"
        )
    bw = value.get("bit_width")
    if bw is not None and (
        isinstance(bw, bool)
        or not isinstance(bw, int)
        or bw < 1
        or bw > 64
    ):
        out.append(
            f"raw_encoding.bit_width={bw!r}: must be an integer in [1,64]"
        )
    signed = value.get("signed")
    if signed is not None and not isinstance(signed, bool):
        out.append(
            f"raw_encoding.signed={signed!r}: must be a JSON boolean"
        )
    return out


_AMBIGUITY_TOKENS_LOWERCASE: tuple[str, ...] = (
    "unclear",
    "please confirm",
    "please verify",
    "ambiguous",
    "ambiguity",
    "uncertain",
    "not stated",
    "datasheet does not state",
    "needs human review",
    "to be confirmed",
    "tbd",
)


def _check_error_conditions_scope(value: Any) -> list[str]:
    """Rule 10 - error_conditions must contain runtime-actionable faults only."""
    if value is None:
        return []
    if not isinstance(value, list):
        return [
            "error_conditions: must be a JSON array (list of fault objects), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for i, item in enumerate(value):
        if not isinstance(item, Mapping):
            out.append(
                f"error_conditions[{i}]: must be an object with runtime "
                f"fault fields, got {type(item).__name__}"
            )
            continue
        has_actionable = any(
            (item.get(k) or "")
            for k in ("condition", "name", "driver_action", "detection")
        )
        if not has_actionable:
            out.append(
                f"error_conditions[{i}]: must populate at least one of "
                "{'condition','name','driver_action','detection'} — runtime "
                "faults need an actionable identifier"
            )
        text_blob = " ".join(
            str(item.get(k) or "")
            for k in (
                "condition",
                "name",
                "description",
                "notes",
                "driver_action",
                "handling",
                "response",
                "detection",
            )
        ).lower()
        for token in _AMBIGUITY_TOKENS_LOWERCASE:
            if token in text_blob:
                out.append(
                    f"error_conditions[{i}]: text contains "
                    f"{token!r} — that reads like an ambiguity-flagging "
                    "note (datasheet unclear, needs review). Move it to "
                    "requires_human; error_conditions is for runtime-"
                    "detectable faults the driver branches on (Rule 10)."
                )
                break
    return out


def _check_evidence_spans(value: Any) -> list[str]:
    """Validate evidence_spans shape (list of {source_id, page, snippet})."""
    if value is None:
        return []
    if not isinstance(value, list):
        return [
            "evidence_spans: must be a JSON array (list of citation objects), "
            f"got {type(value).__name__}"
        ]
    out: list[str] = []
    for i, item in enumerate(value):
        if not isinstance(item, Mapping):
            out.append(
                f"evidence_spans[{i}]: must be an object, "
                f"got {type(item).__name__}"
            )
            continue
        for key in ("source_id", "page", "snippet"):
            if key not in item:
                out.append(
                    f"evidence_spans[{i}]: missing required key {key!r}"
                )
        page = item.get("page")
        if page is not None and (
            isinstance(page, bool) or not isinstance(page, int)
        ):
            out.append(
                f"evidence_spans[{i}].page={page!r}: must be a JSON integer"
            )
        snippet = item.get("snippet")
        if isinstance(snippet, str) and len(snippet) > 200:
            out.append(
                f"evidence_spans[{i}].snippet: {len(snippet)} chars > 200 — "
                "trim to a focused citation (verbatim phrase, not the whole "
                "section)"
            )
    return out


def _to_int(value: Any) -> int | None:
    """Best-effort coerce hex string ('0x48') / decimal string / int → int."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        s = value.strip()
        try:
            if s.lower().startswith("0x"):
                return int(s, 16)
            return int(s)
        except ValueError:
            return None
    return None


# ---------------------------------------------------------------------------
# Prompt builders
# ---------------------------------------------------------------------------

_USER_PREAMBLE = (
    "Target device: {device_id}\n"
    "Target bus/interface: {target_bus_type}\n\n"
    "Below is the structured datasheet content (Markdown headings + tables). "
    "Read it carefully and emit the IR JSON object."
)


# Caps applied to extraction_plan / extraction_notes hints before they are
# spliced into the user prompt. This limits prompt size and strips control
# characters.
EXTRACTION_HINT_MAX_CHARS = 1024
_EXTRACTION_HINT_TRUNCATION_MARKER = "... [truncated]"
CANDIDATE_FACT_SUMMARY_MAX_CHARS = 12000


FACT_CANDIDATE_ROLE = (
    "You are a broad datasheet fact extractor for embedded device drivers. "
    "Your job is to preserve candidate facts before a later pass compresses "
    "them into executable Device IR. Extract datasheet-derived candidates only; "
    "do not use reference drivers, evaluation vectors, or common driver lore."
)

FACT_CANDIDATE_RULES: Tuple[str, ...] = (
    "Output exactly one JSON object matching the provided schema. Use null when "
    "a field is not stated. Keep evidence snippets short and copied from the "
    "datasheet content.",
    "Be inclusive: keep alternate addresses, identity registers, status "
    "registers, result/data registers, configuration/control/trigger registers, "
    "calibration/coefficient registers, thresholds, timing constraints, and "
    "formulas when they are relevant to driving or validating the device.",
    "Keep alternatives instead of choosing too early. For example, preserve both "
    "reset/default configuration facts and optional non-default modes when the "
    "datasheet states them.",
    "Separate facts from decisions. A candidate register or operation can be "
    "optional; do not force it into an init/read flow unless the datasheet states "
    "that the driver must perform it.",
    "Do not extract package pin descriptions, marketing text, electrical limits, "
    "or application examples unless they directly affect bus addressing, timing, "
    "transactions, formulas, or runtime error handling.",
    "If the datasheet describes a probe/identity check, keep the identity "
    "register and expected value. If it only lists an ID register without saying "
    "the driver must read it, preserve it as a candidate fact rather than "
    "inventing a mandatory probe flow.",
    "For multi-byte values, preserve byte names, high/low/MSB/LSB wording, "
    "non-contiguous addresses, source_bytes/source byte bindings, sign bits, "
    "shifts, and ignored padding bits. For non-byte measurements, preserve "
    "source_signal facts such as pulse width, edge timing, interrupt/ready "
    "signals, analog sample source, packet field, or stream sample source.",
    "For operations, preserve ordered steps: command/register write, delay, "
    "status polling, result read, clear/ack, calibration reads, page selects, "
    "write/program/erase completion waits, conversion/data-ready waits, "
    "CRC/checksum/PEC/parity integrity bytes, and default-only/no-bus "
    "initialization notes.",
)


_FACT_CANDIDATE_USER_PREAMBLE = (
    "Extract broad candidate facts for device '{device_id}'. The output is "
    "not a driver; it is a datasheet-derived checklist for the later IR "
    "extractor.\n"
)


def _sanitize_extraction_hint(
    value: str,
    max_chars: int = EXTRACTION_HINT_MAX_CHARS,
) -> str:
    """Return a cleaned, length-capped version of ``value`` suitable for splicing into the IR extraction user prompt."""
    if not value:
        return ""
    text = str(value).replace("\r\n", "\n").replace("\r", "\n")
    cleaned: list[str] = []
    for ch in text:
        if ch in ("\n", "\t"):
            cleaned.append(ch)
            continue
        if ord(ch) < 0x20 or ord(ch) == 0x7F:
            cleaned.append(" ")
            continue
        cleaned.append(ch)
    text = "".join(cleaned).strip()
    if len(text) > max_chars:
        budget = max_chars - len(_EXTRACTION_HINT_TRUNCATION_MARKER)
        if budget < 1:
            return _EXTRACTION_HINT_TRUNCATION_MARKER[:max_chars]
        text = text[:budget].rstrip() + _EXTRACTION_HINT_TRUNCATION_MARKER
    return text


def _sanitize_candidate_fact_summary(value: str) -> str:
    return _sanitize_extraction_hint(value, max_chars=CANDIDATE_FACT_SUMMARY_MAX_CHARS)


def build_fact_candidate_prompt(
    device_id: str,
    structured_content: str,
    *,
    extraction_plan: str = "",
    extraction_notes: str = "",
) -> Tuple[str, str]:
    """Return prompts for broad candidate-fact extraction."""

    system = "\n\n".join([FACT_CANDIDATE_ROLE, *FACT_CANDIDATE_RULES])
    hint_block = ""
    cleaned_plan = _sanitize_extraction_hint(extraction_plan)
    if cleaned_plan:
        hint_block += f"Extraction plan: {cleaned_plan}\n"
    cleaned_notes = _sanitize_extraction_hint(extraction_notes)
    if cleaned_notes:
        hint_block += f"Notes: {cleaned_notes}\n"
    user = (
        _FACT_CANDIDATE_USER_PREAMBLE.format(device_id=device_id)
        + "\n"
        + (hint_block + "\n" if hint_block else "")
        + structured_content
    )
    return system, user


def build_ir_prompt(
    device_id: str,
    structured_content: str,
    *,
    extraction_plan: str = "",
    extraction_notes: str = "",
    candidate_fact_summary: str = "",
    target_bus_type: str = "",
) -> Tuple[str, str]:
    """Return ``(system_prompt, user_prompt)`` for IR extraction."""
    system_parts: list[str] = [ROLE, OUTPUT_FORMAT]
    system_parts.extend(rules_for_target_bus(target_bus_type))
    system = "\n\n".join(system_parts)

    hint_block = ""
    cleaned_plan = _sanitize_extraction_hint(extraction_plan)
    if cleaned_plan:
        hint_block += f"Extraction plan: {cleaned_plan}\n"
    cleaned_notes = _sanitize_extraction_hint(extraction_notes)
    if cleaned_notes:
        hint_block += f"Notes: {cleaned_notes}\n"
    cleaned_candidates = _sanitize_candidate_fact_summary(candidate_fact_summary)
    if cleaned_candidates:
        hint_block += (
            "Candidate fact checklist (datasheet-derived, not a reference driver):\n"
            f"{cleaned_candidates}\n"
            "Use this checklist to avoid dropping relevant facts, but resolve "
            "conflicts against the structured datasheet content below.\n"
        )

    user = (
        _USER_PREAMBLE.format(
            device_id=device_id,
            target_bus_type=target_bus_type or "unknown",
        )
        + "\n\n"
        + (hint_block + "\n" if hint_block else "")
        + structured_content
    )
    return system, user


__all__ = [
    "ROLE",
    "OUTPUT_FORMAT",
    "HARD_RULES",
    "EXTRACTION_HINT_MAX_CHARS",
    "CANDIDATE_FACT_SUMMARY_MAX_CHARS",
    "_sanitize_extraction_hint",
    "_sanitize_candidate_fact_summary",
    "PRIMARY_INTERFACE_RULE",
    "ROUTED_OPERATION_FLOW_RULE",
    "BUS_SPECIFIC_RULES",
    "rules_for_target_bus",
    "detect_violations",
    "build_fact_candidate_prompt",
    "build_ir_prompt",
]
