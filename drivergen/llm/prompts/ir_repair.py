"""Prompts for Device IR schema, evidence, and flow repair."""
from __future__ import annotations

import json
from typing import Any, Iterable, Mapping, Sequence, Tuple


# ---------------------------------------------------------------------------
# Schema repair — returns a full device IR, evidence_spans carry-over.
# ---------------------------------------------------------------------------

SCHEMA_ROLE = (
    "You repair SCHEMA / required-field violations in a previously extracted "
    "device IR. Evidence spans have already been validated and MUST NOT be "
    "rewritten — your job is to mutate only the top-level fields that the "
    "validation issues call out, while keeping every other field stable."
)

SCHEMA_OUTPUT_SHAPE = (
    "Return exactly 1 JSON object that conforms to the full device IR "
    "schema. Do not wrap in markdown, do not output a diff, do not return "
    "a partial document. UTF-8 only. The required top-level keys are: "
    "\"device_id\" (string), \"bus_type\" (string), \"registers_or_commands\" "
    "(array), \"read_sequence\" (array), \"init_sequence\" (array), "
    "\"timing_constraints\" (array), \"conversion_formulae\" (array), "
    "\"raw_encoding\" (object), \"evidence_spans\" (array). Carry the "
    "supplied evidence_spans array forward verbatim (same length, same "
    "entries, same order)."
)

SCHEMA_HARD_RULES: Tuple[str, ...] = (
    # Rule 1 — only touch flagged fields
    'Hard rule 1 — fix only what is flagged: For every entry in '
    '"validation_issues" you MUST mutate exactly the JSONPath the issue '
    'names (or the smallest enclosing field). All other top-level fields '
    'MUST be returned identical to the supplied current_device_ir. '
    'Counter-example: validation_issues mentions only device_ir.bus_type '
    'but the response also rewrites read_sequence (wrong — silent rewrites '
    'invalidate downstream caches and obscure what the repair actually did).',

    # Rule 2 — evidence_spans are frozen
    'Hard rule 2 — evidence_spans are frozen: The output evidence_spans '
    'array MUST equal the input evidence_spans array byte-for-byte. Same '
    'length. Same order. Same {section_id, page, snippet} per entry. Do '
    'not re-snippet, do not reorder, do not deduplicate, do not normalise '
    'whitespace. Counter-example: dropping an entry whose snippet looks '
    'redundant (wrong — that erases the audit trail validators depend on).',

    # Rule 3 — derive only from supplied evidence
    'Hard rule 3 — derive only from supplied evidence: When repairing a '
    'flagged field, ground the new value in the snippets already present '
    'in evidence_spans plus the structured datasheet content provided in '
    'the user prompt. Do not invent device facts the datasheet does not '
    'support. Counter-example: padding registers_or_commands with a '
    'fictitious "reset" register because the issue list complained about '
    'array length (wrong — fabricated registers fail the next-stage '
    'expected-transactions check anyway).',

    # Rule 4 — schema-correct shape per field
    'Hard rule 4 — keep field shape schema-correct: Every repaired field '
    'must keep the canonical shape used by Device IR extraction (see '
    'ir_extraction.HARD_RULES). registers_or_commands entries: '
    '{name,address,access,reset_value,bit_fields,description}. '
    'read_sequence entries: {step_index, action, target, value_or_args, '
    'expected_response, evidence_id}. timing_constraints entries: '
    '{name, value_us, source}. Counter-example: emitting '
    '{"address":"0x00","desc":"…"} only — wrong, "name" / "access" / '
    '"reset_value" / "bit_fields" are required even when their values are '
    'empty arrays or empty strings.',
)

SCHEMA_NEGATIVE_EXAMPLE = (
    "Negative example (DO NOT EMIT):\n"
    "  validation_issues: [{path: 'device_ir.bus_type', msg: 'must be one of "
    "i2c|spi|uart|gpio|adc|onewire'}]\n"
    "  current_device_ir.bus_type: 'i2c-bus'\n"
    "  evidence_spans: [{section_id: 'sec1', page: 4, snippet: 'I²C-bus interface'}]\n"
    "  --- bad response ---\n"
    "  { \"device_id\": \"example_device\", \"bus_type\": \"i2c\", "
    "    \"evidence_spans\": [{\"section_id\":\"sec1\",\"page\":4,"
    "    \"snippet\":\"I2C bus interface\"}], ... }\n"
    "  --- why bad ---\n"
    "  The snippet was silently normalised from 'I²C-bus interface' to "
    "'I2C bus interface'. Hard rule 2 forbids this — evidence is frozen."
)


def build_schema_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: Mapping[str, Any],
    validation_issues: Sequence[Mapping[str, Any]],
) -> Tuple[str, str]:
    """Construct the (system, user) prompt for schema-repair."""
    system_prompt = "\n\n".join((
        SCHEMA_ROLE,
        SCHEMA_OUTPUT_SHAPE,
        "Hard rules:\n" + "\n".join(f"- {r}" for r in SCHEMA_HARD_RULES),
        SCHEMA_NEGATIVE_EXAMPLE,
    ))
    user_prompt = (
        f"Target device: {device_id}\n\n"
        "Current device IR (the evidence_spans array MUST be carried over "
        "unchanged — same length, same entries, same order):\n"
        f"{json.dumps(dict(current_device_ir), ensure_ascii=False, indent=2)}\n\n"
        "Validation issues to fix (each one names the field you must "
        "mutate; any field NOT in this list MUST be returned identical):\n"
        f"{json.dumps(list(validation_issues), ensure_ascii=False, indent=2)}\n\n"
        "Structured datasheet content (reference only — do not paste from "
        "this into the IR; it is here so you can ground a repaired field "
        "in the same source the original extraction used):\n"
        f"{structured_content}"
    )
    return system_prompt, user_prompt


# ---------------------------------------------------------------------------
# Flow audit repair - returns a full device IR, but only flow fields may change.
# ---------------------------------------------------------------------------

FLOW_AUDIT_ROLE = (
    "You audit a datasheet-derived device IR for executable operation-flow "
    "completeness. Use only the supplied structured datasheet content and "
    "the current IR. Do not use reference drivers, evaluation vectors, test "
    "oracles, or prior knowledge of a specific sample."
)

FLOW_AUDIT_OUTPUT_SHAPE = (
    "Return exactly 1 JSON object with two keys: \"audit_findings\" and "
    "\"device_ir\". Do not output markdown, comments, a diff, or a partial "
    "object. \"audit_findings\" is a checklist explaining what you checked. "
    "\"device_ir\" is the full repaired Device IR. Keep the device identity "
    "and non-flow facts stable; this task is only allowed to repair "
    "flow-related fields."
)

FLOW_AUDIT_HARD_RULES: Tuple[str, ...] = (
    'Hard rule 1 - allowed edit surface: You may edit only "access_model", '
    '"operation_flows", "read_channels" flow/source/formula bindings, '
    '"init_sequence", "read_sequence", "timing_constraints", '
    '"conversion_formulae" byte-source bindings, "requires_human", and '
    '"evidence_spans". Preserve device_id, bus_type, address_rule, '
    'register_map, registers_or_commands, raw_encoding, bitfields, '
    'error_conditions, and power_states unless the user prompt explicitly '
    'lists them inside "allowed_edits". Counter-example: changing the I2C '
    'address while auditing a missing poll step is wrong.',

    'Hard rule 2 - public operation lifecycle: For every public read or '
    'measurement channel, scan the datasheet for generic lifecycle evidence: '
    'start/trigger/mode writes, required delay or conversion time, ready or '
    'status polling, result register/window/FIFO reads, clear/acknowledge '
    'writes, page/bank selects, and postprocess/compensation steps. If the '
    'datasheet states such a step is required, put it in the same ordered '
    'read/probe/init/calibration flow. Counter-example: a flow that reads '
    'output bytes but omits the stated measurement trigger or ready check is '
    'incomplete.',

    'Hard rule 3 - explicit checklist: For each read/probe/init/calibration '
    'flow, emit audit_findings[*].checks for exactly these generic names: '
    '"trigger_or_mode", "wait_or_poll", "result_read", "clear_or_ack", '
    '"config_or_calibration", and "byte_source". The historical check name '
    '"byte_source" means source binding: byte_source for byte-oriented '
    'buses, or source_signal for GPIO pulse, interrupt, analog, packet, or '
    'stream-derived measurements. Each check status is one of '
    '"present", "not_required", "missing_required", or "uncertain". If a '
    'required lifecycle piece is missing and the datasheet gives the concrete '
    'register/opcode/timing, repair device_ir.operation_flows. If the '
    'datasheet implies the piece but does not expose enough detail in the '
    'provided content, add a requires_human note. Counter-example: returning '
    'an empty audit_findings array while a public read flow exists is wrong.',

    'Hard rule 4 - no invention: Add a flow step only when the structured '
    'datasheet content supports it. If the datasheet implies a lifecycle '
    'piece but the concrete register/opcode/timing is not visible in the '
    'provided content, keep the flow conservative and add a requires_human '
    'note naming the missing fact. Counter-example: inventing a reset opcode '
    'because many sensors have one is wrong.',

    'Hard rule 4b - no common-usage evidence: audit_findings[*].evidence '
    'must cite supplied datasheet content, not "common usage", "typical '
    'driver practice", "known register sequence", or unstated prior '
    'knowledge. If the current IR already contains a step whose only support '
    'is common usage or an inference outside the datasheet, mark the relevant '
    'check "uncertain", set the flow requires_human flag when appropriate, '
    'and add a top-level requires_human note. Counter-example: claiming a '
    'trigger register is present because it is common in existing drivers is '
    'wrong unless the supplied datasheet content states that register.',

    'Hard rule 5 - access model consistency: A register-pointer read writes '
    'a register address then reads data and must use op/kind '
    '"write_then_read". A command-then-direct-read device writes a command or '
    'mode, waits if needed, then performs a plain read with no pointer '
    'prefix. A stream or packet device should represent framing/streaming '
    'without pretending it has register pointers. Counter-example: converting '
    'a command-mode direct read into a pointer read changes bus semantics.',

    'Hard rule 6 - source-binding precision: Every operation_flows[*].outputs '
    'entry must identify which returned bytes, registers, signal, pulse '
    'width, packet field, stream sample, or default/config value feeds the '
    'channel. For non-contiguous bytes, list each source separately and do '
    'not assume auto-increment unless the datasheet states consecutive/'
    'block-read behavior. For GPIO/timing devices, bind outputs and formula '
    'inputs with source_signal rather than inventing source_bytes. '
    'Counter-example: treating high and low bytes from separate register '
    'addresses as one contiguous 2-byte read without evidence is wrong; '
    'treating an echo pulse width as fake bus bytes is also wrong.',

    'Hard rule 7 - calibration and configuration: If measuring a channel '
    'requires coefficient reads, calibration data, oversampling/rate config, '
    'mode selection, or sensor-enable writes, represent those as init/probe/'
    'calibration flow steps before the read flow consumes the data. '
    'If the current IR register table already contains writable setup/config/'
    'control/mode registers, each chosen executable mode must either reference '
    'the relevant register/opcode in init/probe/read steps or explicitly state '
    'that power-up defaults make the write unnecessary. Counter-example: using '
    'a compensation formula but leaving the coefficient read flow absent is '
    'incomplete; listing a CONFIG register but never saying whether the driver '
    'writes it or relies on defaults is also incomplete. If a flow claims a '
    'non-default precision, rate, resolution, filter, or oversampling mode, '
    'the flow must show the setup write(s) selecting that mode; otherwise keep '
    'the flow on explicit reset/default settings and align timing/formula '
    'scale assumptions with those defaults.',

    'Hard rule 7b - completion and integrity are protocol-generic: If the '
    'datasheet states a write/program/erase cycle, busy/ready flag or pin, '
    'ACK polling, conversion time, measurement time, data-ready flag, '
    'end-of-conversion flag, reset/startup wait, or any equivalent completion '
    'condition, represent that condition as an ordered delay/poll/wait/signal '
    'step in the relevant flow. If returned data includes CRC, checksum, PEC, '
    'parity, or an integrity field, keep the returned field visible and add a '
    'runtime error_conditions entry for mismatch handling. These rules apply '
    'to I2C, SPI, UART, one-wire, GPIO timing, analog, memory, packet, stream, '
    'and display-style devices. Counter-example: writing an EEPROM page and '
    'immediately returning success while the datasheet says an internal write '
    'cycle or ACK polling is required is incomplete; modeling ACK polling by '
    'putting an 8-bit I2C control/address byte in transaction.bytes is also '
    'wrong for normal RTOS I2C APIs because the bus address belongs in '
    'address_rule/API parameters; dropping a protocol CRC byte from a read '
    'result without an error condition is also incomplete.',

    'Hard rule 8 - evidence and traceability: Keep existing good evidence '
    'spans. When adding new flow facts, append short exact snippets from the '
    'structured datasheet content when possible. Do not paraphrase snippets. '
    'If exact grounding is not available in the supplied content, put the '
    'uncertainty in requires_human instead of fabricating evidence.',
)


def build_flow_audit_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: Mapping[str, Any],
    *,
    extraction_plan: str = "",
    extraction_notes: str = "",
    allowed_edits: Sequence[str] | None = None,
) -> Tuple[str, str]:
    """Construct the flow-audit repair prompt."""

    allowed = list(allowed_edits or ())
    system_prompt = "\n\n".join((
        FLOW_AUDIT_ROLE,
        FLOW_AUDIT_OUTPUT_SHAPE,
        "Hard rules:\n" + "\n".join(f"- {r}" for r in FLOW_AUDIT_HARD_RULES),
    ))
    user_prompt = (
        f"Target device: {device_id}\n\n"
        "Allowed edits enforced by the orchestrator:\n"
        f"{json.dumps(allowed, ensure_ascii=False, indent=2)}\n\n"
        "Stage-B extraction plan and notes, if any:\n"
        f"{json.dumps({'extraction_plan': extraction_plan, 'extraction_notes': extraction_notes}, ensure_ascii=False, indent=2)}\n\n"
        "Current device IR:\n"
        f"{json.dumps(dict(current_device_ir), ensure_ascii=False, indent=2)}\n\n"
        "Structured datasheet content to audit against:\n"
        f"{structured_content}"
    )
    return system_prompt, user_prompt


# ---------------------------------------------------------------------------
# Evidence repair — returns ``{ "evidence_spans": [...] }`` only.
# ---------------------------------------------------------------------------

FLOW_RISK_REPAIR_ROLE = (
    "You repair a datasheet-derived device IR only where a deterministic "
    "flow-risk gate found generation-critical operation-flow problems. Use "
    "only the supplied structured datasheet content, current IR, audit "
    "receipt, and risk issues. Do not use reference drivers, evaluation "
    "vectors, test oracles, or prior knowledge of a specific sample."
)

FLOW_RISK_REPAIR_OUTPUT_SHAPE = FLOW_AUDIT_OUTPUT_SHAPE

FLOW_RISK_REPAIR_HARD_RULES: Tuple[str, ...] = (
    'Hard rule 1 - risk issues are not evidence: Treat "flow_risk_issues" '
    'as pointers to fields that need checking, not as factual answers. '
    'Repair a flow only when the structured datasheet content gives concrete '
    'registers, opcodes, byte windows, timing, status bits, clear/ack writes, '
    'or formula dependencies. Counter-example: adding a status register '
    'because the risk issue says "result_read uncertain" is wrong unless the '
    'datasheet text supplied in this prompt names that register/window.',

    'Hard rule 2 - reduce the cited risk without hiding it: For every repaired '
    'issue, update audit_findings with the same six checks used by flow audit: '
    '"trigger_or_mode", "wait_or_poll", "result_read", "clear_or_ack", '
    '"config_or_calibration", and "byte_source". The check named '
    '"byte_source" covers either byte_source or source_signal depending on '
    'the bus/access model. Do not make a check '
    '"present" or "not_required" unless the evidence field cites supplied '
    'datasheet content or an explicit relationship already present in the IR. '
    'Counter-example: deleting a finding or changing "uncertain" to "present" '
    'with evidence "common usage" is wrong.',

    'Hard rule 3 - allowed edit surface: You may edit only "access_model", '
    '"operation_flows", "read_channels" flow/source/formula bindings, '
    '"init_sequence", "read_sequence", "timing_constraints", '
    '"conversion_formulae" byte-source bindings, "requires_human", and '
    '"evidence_spans". Preserve device_id, bus_type, address_rule, '
    'register_map, registers_or_commands, raw_encoding, bitfields, '
    'error_conditions, and power_states. Counter-example: changing an I2C '
    'address while repairing a missing poll step is wrong.',

    'Hard rule 4 - no invention and no common-usage evidence: Do not use '
    '"common usage", "typical driver practice", "known register sequence", '
    'or unstated prior knowledge as support. If the supplied content does not '
    'contain enough detail, keep the flow conservative and leave a precise '
    'requires_human note. Counter-example: fabricating a clear-interrupt write '
    'because many sensors need one is wrong.',

    'Hard rule 5 - formula dependencies: If a formula for channel A consumes '
    'source bytes from channel B and the current IR already has a producer '
    'flow for B, declare that producer as a precondition or place the producer '
    'flow before the consumer flow. Do not invent a new B read transaction; use '
    'only existing flow steps unless the datasheet content gives concrete '
    'additional operations. Counter-example: adding a new temperature register '
    'address not present in the IR or datasheet is wrong.',

    'Hard rule 6 - output must stay traceable: Keep existing good evidence '
    'spans. When adding or changing flow facts, append short exact snippets '
    'from structured_content when possible. If exact grounding is unavailable, '
    'record the uncertainty in requires_human instead of fabricating evidence.',

    'Hard rule 7 - config register coverage: When a flow-risk issue says '
    'writable setup/config/control/mode registers are uncovered, inspect the '
    'supplied datasheet content for generic setup facts: mode selection, '
    'conversion/background/one-shot mode, channel enable, oversampling, rate, '
    'resolution, filter, power state, or calibration configuration. Add only '
    'datasheet-supported setup steps or an explicit default-configuration '
    'precondition; do not guess a preferred mode or value from common driver '
    'practice. If the current flow claims a non-default precision/rate/filter/'
    'oversampling mode but lacks setup writes, either add the datasheet-stated '
    'writes selecting that mode or change the flow to explicit reset/default '
    'settings and make the timing/formula scale assumptions consistent with '
    'those defaults. Counter-example: adding a CONFIG write with a plausible '
    'value because the issue mentions a config register is wrong unless the '
    'supplied datasheet content states that value or default relationship.',

    'Hard rule 8 - completion and integrity repair: When a flow-risk issue '
    'mentions missing completion, waiting, polling, ready/busy handling, or '
    'write-cycle handling, inspect the supplied content for generic completion '
    'facts and add only supported delay/poll_until/wait_until_ready/'
    'wait_signal steps. When content mentions CRC, checksum, PEC, parity, or '
    'other integrity fields protecting returned data, keep those fields visible '
    'in the read/source binding and add a runtime error_conditions entry. This '
    'is bus-agnostic; do not assume the device is I2C or register-mapped. '
    'Counter-example: adding ACK polling to a non-I2C device because one sample '
    'uses EEPROM is wrong; use ACK polling only when the supplied datasheet '
    'content states ACK polling or an equivalent bus-level ready mechanism, '
    'and for normal RTOS I2C APIs model it as a poll/wait condition on address '
    'ACK rather than as an 8-bit control byte payload.',
)


def build_flow_risk_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: Mapping[str, Any],
    *,
    flow_audit_receipt: Mapping[str, Any] | None,
    flow_risk_issues: Sequence[Mapping[str, Any]],
    extraction_plan: str = "",
    extraction_notes: str = "",
    allowed_edits: Sequence[str] | None = None,
) -> Tuple[str, str]:
    """Construct a risk-directed flow repair prompt."""

    allowed = list(allowed_edits or ())
    system_prompt = "\n\n".join((
        FLOW_RISK_REPAIR_ROLE,
        FLOW_RISK_REPAIR_OUTPUT_SHAPE,
        "Hard rules:\n" + "\n".join(f"- {r}" for r in FLOW_RISK_REPAIR_HARD_RULES),
    ))
    user_prompt = (
        f"Target device: {device_id}\n\n"
        "Allowed edits enforced by the orchestrator:\n"
        f"{json.dumps(allowed, ensure_ascii=False, indent=2)}\n\n"
        "Stage-B extraction plan and notes, if any:\n"
        f"{json.dumps({'extraction_plan': extraction_plan, 'extraction_notes': extraction_notes}, ensure_ascii=False, indent=2)}\n\n"
        "Flow-risk issues to repair when datasheet evidence supports them:\n"
        f"{json.dumps(list(flow_risk_issues), ensure_ascii=False, indent=2)}\n\n"
        "Current flow-audit receipt:\n"
        f"{json.dumps(dict(flow_audit_receipt or {}), ensure_ascii=False, indent=2)}\n\n"
        "Current device IR:\n"
        f"{json.dumps(dict(current_device_ir), ensure_ascii=False, indent=2)}\n\n"
        "Structured datasheet content to repair against:\n"
        f"{structured_content}"
    )
    return system_prompt, user_prompt


EVIDENCE_ROLE = (
    "You repair EVIDENCE SPANS for a previously extracted device IR. "
    "The device-level facts (registers, sequences, formulae, encoding) "
    "have already been validated and MUST NOT be touched. Your only job "
    "is to rewrite the evidence_spans array so every snippet is short, "
    "page-accurate, and copied near-verbatim from the structured datasheet "
    "content you are given."
)

EVIDENCE_OUTPUT_SHAPE = (
    "Return exactly 1 JSON object whose ONLY top-level key is "
    "\"evidence_spans\" (an array). Do not wrap in markdown. Do not echo "
    "the rest of the device IR. UTF-8 only. Each entry is "
    "{ \"section_id\": <string>, \"page\": <int>, \"snippet\": <string> }. "
    "The repaired array length MUST equal the supplied evidence_spans "
    "array length — repair entries 1-for-1 by index."
)

EVIDENCE_HARD_RULES: Tuple[str, ...] = (
    # Rule 1 — verbatim snippets only
    'Hard rule 1 — snippets are near-verbatim from structured_content: '
    'Each repaired snippet MUST be a contiguous fragment found in the '
    'structured datasheet content the user prompt supplies. Prefer exact '
    'sentences, exact table-row text, exact register/address lines, exact '
    'formula fragments. You may trim leading / trailing whitespace and '
    'page headers but you MUST NOT paraphrase or summarise. '
    'Counter-example: rewriting "Read Temp register (pointer 0x00)" as '
    '"reads the temperature pointer register" (wrong — that paraphrase '
    'breaks substring grounding and the validator will mark the entry '
    'as not-found).',

    # Rule 2 — page must be honest
    'Hard rule 2 — page numbers reflect the supplied content: The "page" '
    'field MUST be the page on which the snippet appears in the supplied '
    'structured content (1-based). If the page is unknown, keep the '
    'original entry\'s page rather than inventing a number. '
    'Counter-example: copying snippet text from page 12 but keeping the '
    'original page=4 (wrong — evidence validation cross-checks page+snippet and '
    'will fail the entry).',

    # Rule 3 — preserve count + indexing
    'Hard rule 3 — preserve count and entry order: Output exactly N '
    'evidence spans where N == len(input.evidence_spans). Repair entry i '
    'into output[i]; do not drop entries you cannot improve, just keep '
    'them as-is. Counter-example: returning 5 spans when 7 were supplied '
    '(wrong — validators consume spans by index when re-running validation '
    'and a length mismatch corrupts that mapping).',

    # Rule 4 — no synthetic table reconstruction
    'Hard rule 4 — never reconstruct synthetic table rows: Do not glue '
    'together cells from different rows / pages with invented separators '
    'like "|", "  ...  ", "ROW1 …" etc. If a single cell is the right '
    'evidence, snippet that cell only. Counter-example: producing '
    '"0x00 | Temp | 0xCC" by combining three cells separated by " | " '
    '(wrong — that exact string never appears in the structured content '
    'so substring grounding fails).',

    # Rule 5 — keep entries that are already good
    'Hard rule 5 — keep good evidence as-is: If an input entry already '
    'satisfies the validator (correct page, exact snippet, present in '
    'structured content), output it byte-for-byte unchanged. Repair only '
    'the entries flagged in validation_issues. Counter-example: '
    'reformatting whitespace inside a snippet that was already passing '
    '(wrong — even cosmetic changes can shift the substring index and '
    'fail re-validation downstream).',

    'Hard rule 6 - prefer short exact fragments for tables and timing: If '
    'the bad snippet is a synthesized sentence about a numeric value, unit, '
    'timing budget, delay, rate, register field, or table row, do not rewrite '
    'another sentence. Instead choose the shortest contiguous source fragment '
    'that contains the same label plus value/unit, such as an exact table cell, '
    'row fragment, heading fragment, or formula fragment. Counter-example: '
    'turning table cells "Timing budget" and "33 ms" into "The typical timing '
    'budget is 33 ms" is wrong because that sentence is not in the source; '
    'snippet "Timing budget" or "33 ms" from the actual table is safer.',
)

EVIDENCE_NEGATIVE_EXAMPLE = (
    "Negative example (DO NOT EMIT):\n"
    "  current_device_ir.evidence_spans = [\n"
    "    {section_id: 'sec1', page: 4, snippet: 'I²C-bus interface'},\n"
    "    {section_id: 'sec3', page: 7, snippet: 'pointer | 0x00 | Temp'}\n"
    "  ]\n"
    "  validation_issues: [{path: 'evidence_spans[1].snippet', msg: "
    "'snippet not found in structured content'}]\n"
    "  --- bad response ---\n"
    "  { \"evidence_spans\": [\n"
    "    {\"section_id\":\"sec1\",\"page\":4,\"snippet\":\"I2C bus interface\"},\n"
    "    {\"section_id\":\"sec3\",\"page\":7,"
    "     \"snippet\":\"pointer reg 0x00 = Temp\"}\n"
    "  ] }\n"
    "  --- why bad ---\n"
    "  Entry 0 was already good but got cosmetically rewritten (Hard rule 5). "
    "Entry 1 was paraphrased into prose that does not appear in the "
    "datasheet (Hard rule 1)."
)


def build_evidence_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: Mapping[str, Any],
    validation_issues: Sequence[Mapping[str, Any]],
) -> Tuple[str, str]:
    """Construct the (system, user) prompt for evidence-repair."""
    system_prompt = "\n\n".join((
        EVIDENCE_ROLE,
        EVIDENCE_OUTPUT_SHAPE,
        "Hard rules:\n" + "\n".join(f"- {r}" for r in EVIDENCE_HARD_RULES),
        EVIDENCE_NEGATIVE_EXAMPLE,
    ))
    user_prompt = (
        f"Target device: {device_id}\n\n"
        "Current device IR (do NOT modify any field other than "
        "evidence_spans; you are not asked to repeat the rest of the IR "
        "in your output):\n"
        f"{json.dumps(dict(current_device_ir), ensure_ascii=False, indent=2)}\n\n"
        "Validation issues that name the entries to repair (entries not "
        "named here MUST be returned byte-for-byte unchanged):\n"
        f"{json.dumps(list(validation_issues), ensure_ascii=False, indent=2)}\n\n"
        "Structured datasheet content (the ONLY source of truth for "
        "snippet text — every repaired snippet must be a contiguous "
        "fragment of this string):\n"
        f"{structured_content}"
    )
    return system_prompt, user_prompt


# ---------------------------------------------------------------------------
# Mechanical detectors.
# ---------------------------------------------------------------------------


def _coerce_evidence_list(value: Any) -> list[Mapping[str, Any]] | None:
    """Best-effort: return a list of mappings or None when malformed."""
    if not isinstance(value, list):
        return None
    out: list[Mapping[str, Any]] = []
    for item in value:
        if not isinstance(item, Mapping):
            return None
        out.append(item)
    return out


def _evidence_signature(entry: Mapping[str, Any]) -> tuple[str, Any, str]:
    """Identity tuple used for byte-equality checks of carry-over evidence."""
    return (
        str(entry.get("section_id", "")),
        entry.get("page"),
        str(entry.get("snippet", "")),
    )


def detect_schema_repair_violations(
    repaired_ir: Mapping[str, Any],
    *,
    original_ir: Mapping[str, Any],
) -> list[str]:
    """Validate a schema-repair response."""
    flagged: list[str] = []

    if not isinstance(repaired_ir, Mapping):
        flagged.append(
            f"device_ir: response root must be a JSON object "
            f"(got {type(repaired_ir).__name__})"
        )
        return flagged

    repaired_evidence = _coerce_evidence_list(
        repaired_ir.get("evidence_spans"),
    )
    original_evidence = _coerce_evidence_list(
        original_ir.get("evidence_spans"),
    )

    if repaired_evidence is None:
        flagged.append(
            "device_ir.evidence_spans: must be a JSON array of "
            "{section_id, page, snippet} objects"
        )
        return flagged
    if original_evidence is None:
        flagged.append(
            "device_ir.evidence_spans: original IR is missing or malformed; "
            "cannot verify carry-over"
        )
        return flagged

    if len(repaired_evidence) != len(original_evidence):
        flagged.append(
            f"device_ir.evidence_spans: length changed (was "
            f"{len(original_evidence)}, got {len(repaired_evidence)}) — "
            "evidence_spans must be carried over verbatim"
        )

    repaired_sigs = [_evidence_signature(e) for e in repaired_evidence]
    original_sigs = [_evidence_signature(e) for e in original_evidence]
    for idx, (rep, orig) in enumerate(zip(repaired_sigs, original_sigs)):
        if rep != orig:
            flagged.append(
                f"device_ir.evidence_spans[{idx}]: rewritten "
                f"(was section_id={orig[0]!r} page={orig[1]!r} "
                f"snippet={orig[2]!r}; got section_id={rep[0]!r} "
                f"page={rep[1]!r} snippet={rep[2]!r}) — schema repair "
                "must not touch evidence"
            )
    return flagged


def detect_evidence_repair_violations(
    repaired_payload: Mapping[str, Any],
    *,
    structured_content: str,
    expected_count: int,
) -> list[str]:
    """Validate an evidence-repair response."""
    flagged: list[str] = []

    if not isinstance(repaired_payload, Mapping):
        flagged.append(
            f"response: root must be a JSON object "
            f"(got {type(repaired_payload).__name__})"
        )
        return flagged

    extra_keys = sorted(set(repaired_payload.keys()) - {"evidence_spans"})
    if extra_keys:
        flagged.append(
            "response: only 'evidence_spans' is allowed at the top level; "
            f"got extra keys {extra_keys!r}"
        )

    spans = _coerce_evidence_list(repaired_payload.get("evidence_spans"))
    if spans is None:
        flagged.append(
            "response.evidence_spans: must be a JSON array of "
            "{section_id, page, snippet} objects"
        )
        return flagged

    if len(spans) != expected_count:
        flagged.append(
            f"response.evidence_spans: length must equal the input "
            f"(was {expected_count}, got {len(spans)}) — repair entry-by-"
            "entry, do not drop or add"
        )

    for idx, span in enumerate(spans):
        loc = f"response.evidence_spans[{idx}]"
        section_id = span.get("section_id")
        page = span.get("page")
        snippet = span.get("snippet")

        if not isinstance(section_id, str) or not section_id.strip():
            flagged.append(f"{loc}.section_id: must be a non-empty string")
        if not isinstance(page, int) or isinstance(page, bool) or page < 1:
            flagged.append(
                f"{loc}.page: must be an integer >= 1 (got {page!r})"
            )
        if not isinstance(snippet, str) or not snippet.strip():
            flagged.append(f"{loc}.snippet: must be a non-empty string")
            continue

        # Substring grounding — strip leading / trailing whitespace because
        # Hard rule 1 explicitly allows that trimming.
        needle = snippet.strip()
        if needle and needle not in structured_content:
            flagged.append(
                f"{loc}.snippet: not found in structured_content "
                f"(snippet={needle!r}) — Hard rule 1 requires "
                "near-verbatim copy from the supplied content"
            )

    return flagged


__all__ = [
    "SCHEMA_ROLE",
    "SCHEMA_OUTPUT_SHAPE",
    "SCHEMA_HARD_RULES",
    "SCHEMA_NEGATIVE_EXAMPLE",
    "EVIDENCE_ROLE",
    "EVIDENCE_OUTPUT_SHAPE",
    "EVIDENCE_HARD_RULES",
    "EVIDENCE_NEGATIVE_EXAMPLE",
    "build_schema_repair_prompt",
    "build_evidence_repair_prompt",
    "detect_schema_repair_violations",
    "detect_evidence_repair_violations",
]
