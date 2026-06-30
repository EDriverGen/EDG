"""Device IR fact-bank projection and lightweight coverage hints."""
from __future__ import annotations

from collections import defaultdict
import json
import re
from typing import Any, Iterable, Mapping

from .models import ValidationIssue, ValidationResult


FACT_BANK_VERSION = "2026-05-05.fact-bank-p1-signals"

BUS_STEP_OPS = frozenset({"write", "read", "write_then_read"})

ROLE_PATTERNS: dict[str, tuple[str, ...]] = {
    "identity": (
        "who am i",
        "whoami",
        "chip id",
        "device id",
        "part id",
        "product id",
        "revision id",
        "rev id",
        "prod id",
        "id",
    ),
    "status": (
        "status",
        "ready",
        "busy",
        "done",
        "complete",
        "valid",
        "interrupt status",
        "int status",
        "fifo status",
    ),
    "result": (
        "result",
        "output",
        "out",
        "measurement",
        "measure",
        "range",
        "distance",
        "temperature",
        "temp",
        "pressure",
        "humidity",
        "accelerometer",
        "gyro",
        "mag",
    ),
    "data": (
        "data",
        "raw",
        "msb",
        "lsb",
        "high byte",
        "low byte",
        "high",
        "low",
    ),
    "config": (
        "config",
        "configuration",
        "cfg",
        "conf",
        "mode",
        "rate",
        "resolution",
        "odr",
        "osr",
        "oversampling",
        "filter",
        "gain",
        "setup",
    ),
    "control": (
        "control",
        "ctrl",
        "enable",
        "disable",
        "power",
        "pwr",
        "standby",
    ),
    "coefficient": (
        "coefficient",
        "coefficients",
        "coef",
        "calibration",
        "calib",
        "trim",
        "compensation",
    ),
    "trigger": (
        "trigger",
        "start",
        "single shot",
        "one shot",
        "oneshot",
        "command",
        "conversion start",
        "measurement start",
    ),
    "clear": (
        "clear",
        "ack",
        "acknowledge",
        "latched",
    ),
    "threshold": (
        "threshold",
        "thresh",
        "alarm",
        "limit",
        "hyst",
        "tos",
        "thyst",
    ),
}

BYTE_PART_PATTERNS: dict[str, tuple[str, ...]] = {
    "high": ("high byte", "msb", "upper byte", "high"),
    "low": ("low byte", "lsb", "lower byte", "low"),
}

DEFAULT_INIT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(power[-\s]?up|reset)\s+defaults?\b", re.IGNORECASE),
    re.compile(r"\breset\s+(state|value)\b", re.IGNORECASE),
    re.compile(r"\bdefaults?\s+(select|configure|enable|set|use|leave)\b", re.IGNORECASE),
    re.compile(r"\b(no|without)\s+explicit\s+(setup|config|configuration|mode|write)\b", re.IGNORECASE),
    re.compile(r"\b(config|configuration|mode)\s+writes?\s+(is|are)\s+not\s+required\b", re.IGNORECASE),
    re.compile(r"\bno\s+bus\s+(init|initiali[sz]ation|write)\s+required\b", re.IGNORECASE),
)


def build_device_ir_fact_bank(
    device_ir: Mapping[str, Any],
    candidate_fact_bank: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    """Build a deterministic sidecar containing retained IR facts."""

    if not isinstance(device_ir, Mapping):
        device_ir = {}

    fact_bank: dict[str, Any] = {
        "fact_bank_version": FACT_BANK_VERSION,
        "source": "device_ir_projection_v1",
        "device_id": str(device_ir.get("device_id") or ""),
        "bus_type": str(device_ir.get("bus_type") or ""),
        "ir_schema_version": device_ir.get("ir_schema_version"),
        "addresses": _extract_addresses(device_ir.get("address_rule")),
        "registers": _extract_registers(device_ir),
        "bitfields": _extract_bitfields(device_ir),
        "operation_steps": _extract_operation_steps(device_ir),
        "channels": _extract_channels(device_ir),
        "formula_inputs": _extract_formula_inputs(device_ir),
        "evidence_spans": _as_list(device_ir.get("evidence_spans")),
    }
    fact_bank["coverage_hints"] = _build_coverage_hints(device_ir, fact_bank)
    fact_bank["coverage_issues"] = assess_ir_fact_coverage(device_ir, fact_bank).to_dict()["issues"]
    if isinstance(candidate_fact_bank, Mapping):
        fact_bank["candidate_projection"] = compare_candidate_facts_to_projection(
            candidate_fact_bank,
            fact_bank,
        )
    fact_bank["summary"] = summarize_fact_bank(fact_bank)
    return fact_bank


def summarize_fact_bank(fact_bank: Mapping[str, Any]) -> dict[str, Any]:
    """Return compact fact-bank counts for reporting."""

    coverage_issues = _as_list(fact_bank.get("coverage_issues"))
    return {
        "fact_bank_version": fact_bank.get("fact_bank_version"),
        "address_count": len(_as_list(fact_bank.get("addresses"))),
        "register_count": len(_as_list(fact_bank.get("registers"))),
        "bitfield_count": len(_as_list(fact_bank.get("bitfields"))),
        "operation_step_count": len(_as_list(fact_bank.get("operation_steps"))),
        "channel_count": len(_as_list(fact_bank.get("channels"))),
        "formula_input_count": len(_as_list(fact_bank.get("formula_inputs"))),
        "evidence_span_count": len(_as_list(fact_bank.get("evidence_spans"))),
        "coverage_issue_count": len(coverage_issues),
        "coverage_warning_count": sum(1 for issue in coverage_issues if issue.get("level") == "warning"),
        "coverage_error_count": sum(1 for issue in coverage_issues if issue.get("level") == "error"),
    }


def summarize_candidate_fact_bank(candidate_fact_bank: Mapping[str, Any]) -> dict[str, Any]:
    """Return compact counts for candidate facts."""

    if not isinstance(candidate_fact_bank, Mapping):
        candidate_fact_bank = {}
    return {
        "device_id": candidate_fact_bank.get("device_id"),
        "bus_type": candidate_fact_bank.get("bus_type"),
        "address_count": len(_as_list(candidate_fact_bank.get("candidate_addresses"))),
        "register_count": len(_as_list(candidate_fact_bank.get("candidate_registers"))),
        "bitfield_count": len(_as_list(candidate_fact_bank.get("candidate_bitfields"))),
        "operation_count": len(_as_list(candidate_fact_bank.get("candidate_operations"))),
        "channel_count": len(_as_list(candidate_fact_bank.get("candidate_channels"))),
        "formula_count": len(_as_list(candidate_fact_bank.get("candidate_formulae"))),
        "timing_count": len(_as_list(candidate_fact_bank.get("candidate_timing_constraints"))),
        "evidence_span_count": len(_as_list(candidate_fact_bank.get("evidence_spans"))),
        "requires_human_count": len(_as_list(candidate_fact_bank.get("requires_human"))),
    }


def format_candidate_fact_summary_for_ir_prompt(
    candidate_fact_bank: Mapping[str, Any] | None,
    *,
    max_chars: int = 12000,
) -> str:
    """Format candidate facts as a bounded checklist."""

    if not isinstance(candidate_fact_bank, Mapping):
        return ""

    lines: list[str] = []
    summary = summarize_candidate_fact_bank(candidate_fact_bank)
    lines.append("Summary: " + json.dumps(summary, sort_keys=True, ensure_ascii=False))
    _append_candidate_rows(
        lines,
        "Addresses",
        candidate_fact_bank.get("candidate_addresses"),
        lambda item: (
            f"{item.get('value')} form={item.get('addressing_form')} "
            f"default={item.get('is_default')} desc={_short(item.get('description'))}"
        ),
        limit=12,
    )
    _append_candidate_rows(
        lines,
        "Registers",
        candidate_fact_bank.get("candidate_registers"),
        lambda item: (
            f"{item.get('name')}@{item.get('value')} access={item.get('access')} "
            f"bits={item.get('size_bits')} roles={','.join(_as_str_list(item.get('semantic_roles')))} "
            f"desc={_short(item.get('description'))}"
        ),
        limit=48,
    )
    _append_candidate_rows(
        lines,
        "Bitfields",
        candidate_fact_bank.get("candidate_bitfields"),
        lambda item: (
            f"{item.get('register')}.{item.get('name')} bits={item.get('bit_range')} "
            f"roles={','.join(_as_str_list(item.get('semantic_roles')))} "
            f"desc={_short(item.get('description'))}"
        ),
        limit=48,
    )
    _append_candidate_rows(
        lines,
        "Operations",
        candidate_fact_bank.get("candidate_operations"),
        _format_candidate_operation,
        limit=24,
    )
    _append_candidate_rows(
        lines,
        "Channels",
        candidate_fact_bank.get("candidate_channels"),
        lambda item: (
            f"{item.get('id')} source={item.get('source')} formula={item.get('formula_id')} "
            f"desc={_short(item.get('description'))}"
        ),
        limit=16,
    )
    _append_candidate_rows(
        lines,
        "Formulae",
        candidate_fact_bank.get("candidate_formulae"),
        lambda item: (
            f"{item.get('name')} inputs={','.join(_as_str_list(item.get('inputs')))} "
            f"output={item.get('output')} formula={_short(item.get('formula'), 180)}"
        ),
        limit=16,
    )
    _append_candidate_rows(
        lines,
        "Timing",
        candidate_fact_bank.get("candidate_timing_constraints"),
        lambda item: (
            f"{item.get('name')}={item.get('value')} {item.get('unit')} "
            f"when={_short(item.get('condition'))}"
        ),
        limit=16,
    )
    _append_candidate_rows(
        lines,
        "RequiresHuman",
        candidate_fact_bank.get("requires_human"),
        lambda item: _short(item, 180),
        limit=12,
    )
    text = "\n".join(lines)
    if len(text) <= max_chars:
        return text
    marker = "\n... [candidate fact summary truncated]"
    return text[: max(0, max_chars - len(marker))].rstrip() + marker


def compare_candidate_facts_to_projection(
    candidate_fact_bank: Mapping[str, Any],
    projected_fact_bank: Mapping[str, Any],
) -> dict[str, Any]:
    """Compare candidate facts with the final projected fact bank."""

    projected_registers = _as_list(projected_fact_bank.get("registers"))
    projected_addresses = _as_list(projected_fact_bank.get("addresses"))
    projected_channels = _as_list(projected_fact_bank.get("channels"))
    projected_formula_inputs = _as_list(projected_fact_bank.get("formula_inputs"))
    projected_step_text = _operation_text_blob(_as_list(projected_fact_bank.get("operation_steps")))

    projected_register_keys = _register_key_set(projected_registers, name_key="name", value_key="address")
    projected_address_values = {_normalise_hex(item.get("address")) for item in projected_addresses if isinstance(item, Mapping)}
    projected_channel_ids = {
        _compact_text(item.get("id"))
        for item in projected_channels
        if isinstance(item, Mapping) and str(item.get("id") or "").strip()
    }
    projected_formula_names = {
        _compact_text(item.get("formula_id"))
        for item in projected_formula_inputs
        if isinstance(item, Mapping) and str(item.get("formula_id") or "").strip()
    }

    missing_addresses = []
    for item in _as_list(candidate_fact_bank.get("candidate_addresses")):
        if not isinstance(item, Mapping):
            continue
        value = _normalise_hex(item.get("value"))
        if value and value not in projected_address_values:
            missing_addresses.append(_candidate_address_ref(item))

    missing_registers = []
    for item in _as_list(candidate_fact_bank.get("candidate_registers")):
        if not isinstance(item, Mapping):
            continue
        roles = set(_as_str_list(item.get("semantic_roles")))
        if not roles.intersection({"identity", "status", "result", "data", "config", "control", "coefficient", "trigger", "threshold"}):
            continue
        if not _candidate_register_is_projected(item, projected_register_keys):
            missing_registers.append(_candidate_register_ref(item))

    missing_operations = []
    for item in _as_list(candidate_fact_bank.get("candidate_operations")):
        if not isinstance(item, Mapping):
            continue
        if not _candidate_operation_is_projected(item, projected_step_text):
            missing_operations.append(_candidate_operation_ref(item))

    missing_channels = []
    for item in _as_list(candidate_fact_bank.get("candidate_channels")):
        if not isinstance(item, Mapping):
            continue
        channel_id = _compact_text(item.get("id"))
        if channel_id and channel_id not in projected_channel_ids:
            missing_channels.append({"id": item.get("id"), "description": item.get("description")})

    missing_formulae = []
    for item in _as_list(candidate_fact_bank.get("candidate_formulae")):
        if not isinstance(item, Mapping):
            continue
        name = _compact_text(item.get("name"))
        if name and name not in projected_formula_names:
            missing_formulae.append({"name": item.get("name"), "output": item.get("output")})

    return {
        "candidate_summary": summarize_candidate_fact_bank(candidate_fact_bank),
        "missing_candidate_addresses": missing_addresses,
        "missing_candidate_registers": missing_registers,
        "missing_candidate_operations": missing_operations,
        "missing_candidate_channels": missing_channels,
        "missing_candidate_formulae": missing_formulae,
        "missing_candidate_counts": {
            "addresses": len(missing_addresses),
            "registers": len(missing_registers),
            "operations": len(missing_operations),
            "channels": len(missing_channels),
            "formulae": len(missing_formulae),
        },
    }


def assess_ir_fact_coverage(
    device_ir: Mapping[str, Any],
    fact_bank: Mapping[str, Any] | None = None,
) -> ValidationResult:
    """Assess whether key retained facts are connected to executable flows."""

    if fact_bank is None:
        fact_bank = {
            "registers": _extract_registers(device_ir),
            "operation_steps": _extract_operation_steps(device_ir),
            "channels": _extract_channels(device_ir),
            "formula_inputs": _extract_formula_inputs(device_ir),
        }

    hints = _build_coverage_hints(device_ir, fact_bank)
    issues: list[ValidationIssue] = []

    for register in hints["identity_registers_not_referenced_by_flows"]:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.fact_bank.identity_register_unreferenced",
                (
                    "Readable identity register is present but no operation flow "
                    f"references it: {_format_register_ref(register)}."
                ),
            )
        )

    for register in hints["config_registers_not_referenced_by_flows"]:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.fact_bank.config_register_unreferenced",
                (
                    "Writable configuration/control register is present but no "
                    f"operation flow references it: {_format_register_ref(register)}."
                ),
            )
        )

    for channel in hints["channels_without_source_bytes"]:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.fact_bank.channel_missing_source_bytes",
                f"Read channel lacks source_bytes/source_signal binding: {channel.get('id') or '<unnamed>'}.",
            )
        )

    for formula_input in hints["formula_inputs_without_byte_source"]:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.fact_bank.formula_input_missing_byte_source",
                (
                    "Formula input lacks byte_source/source_signal/default/config binding: "
                    f"{formula_input.get('formula_id') or '<formula>'}.{formula_input.get('name') or '<input>'}."
                ),
            )
        )

    for hint in hints["non_contiguous_result_hints"]:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.fact_bank.non_contiguous_result_bytes",
                (
                    "High/low result byte registers appear non-contiguous and need "
                    f"explicit flow/source-byte handling: {hint.get('family_key') or '<family>'}."
                ),
            )
        )

    return ValidationResult(ok=True, issues=issues)


def _extract_addresses(address_rule: Any) -> list[dict[str, Any]]:
    if not isinstance(address_rule, Mapping):
        return []
    records: list[dict[str, Any]] = []
    addresses = address_rule.get("addresses")
    if isinstance(addresses, list):
        for index, entry in enumerate(addresses):
            if isinstance(entry, Mapping):
                records.append(
                    {
                        "index": index,
                        "address": _normalise_hex(entry.get("address")),
                        "addressing_form": entry.get("addressing_form") or address_rule.get("addressing_form"),
                        "is_default": entry.get("is_default"),
                        "description": entry.get("description") or entry.get("notes"),
                        "raw_record": dict(entry),
                    }
                )
            else:
                records.append(
                    {
                        "index": index,
                        "address": _normalise_hex(entry),
                        "addressing_form": address_rule.get("addressing_form"),
                        "is_default": None,
                        "description": None,
                        "raw_record": entry,
                    }
                )
    else:
        address = _first_hex_from_mapping(address_rule, ("address", "default", "value"))
        if address:
            records.append(
                {
                    "index": 0,
                    "address": address,
                    "addressing_form": address_rule.get("addressing_form"),
                    "is_default": True,
                    "description": address_rule.get("description") or address_rule.get("notes"),
                    "raw_record": dict(address_rule),
                }
            )
    return records


def _extract_registers(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for index, entry in enumerate(_as_list(device_ir.get("registers_or_commands"))):
        if not isinstance(entry, Mapping):
            continue
        name = str(entry.get("name") or entry.get("id") or entry.get("register") or f"register_{index}")
        description = _first_text(entry, ("description", "purpose", "notes", "summary"))
        access = _first_text(entry, ("access", "rw", "direction"))
        address = _first_hex_from_mapping(
            entry,
            ("address", "value", "opcode", "command", "register", "offset", "pointer"),
        )
        size_bits = entry.get("size_bits") or entry.get("width_bits") or entry.get("bits")
        roles = _infer_roles([name, description, access, json.dumps(entry, sort_keys=True, default=str)])
        if _access_text_is_writable(access) and _pattern_matches(
            "reset",
            _normalise_text(f"{name} {description}"),
            _compact_text(f"{name} {description}"),
            set(re.findall(r"[a-z0-9]+", _normalise_text(f"{name} {description}"))),
        ):
            roles = _append_role(roles, "control")
        if not _access_text_is_writable(access) and any(role in roles for role in ("identity", "status", "result", "data")):
            roles = [
                role for role in roles
                if role not in {"config", "control", "trigger"}
            ]
        records.append(
            {
                "index": index,
                "name": name,
                "address": address,
                "access": access,
                "size_bits": size_bits,
                "description": description,
                "semantic_roles": roles,
                "byte_part": _infer_byte_part([name, description]),
                "raw_record": dict(entry),
            }
        )
    return records


def _extract_bitfields(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for index, entry in enumerate(_as_list(device_ir.get("bitfields"))):
        if not isinstance(entry, Mapping):
            continue
        name = str(entry.get("name") or entry.get("id") or f"bitfield_{index}")
        register = entry.get("register") or entry.get("parent") or entry.get("register_name")
        description = _first_text(entry, ("description", "purpose", "notes", "summary"))
        roles = _infer_roles([name, str(register or ""), description, json.dumps(entry, sort_keys=True, default=str)])
        records.append(
            {
                "index": index,
                "name": name,
                "register": register,
                "bit_range": entry.get("bit_range") or entry.get("bits") or entry.get("position"),
                "reset_value": entry.get("reset_value") or entry.get("default"),
                "description": description,
                "semantic_roles": roles,
                "raw_record": dict(entry),
            }
        )
    return records


def _extract_operation_steps(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for flow_index, flow in enumerate(_as_list(device_ir.get("operation_flows"))):
        if not isinstance(flow, Mapping):
            continue
        flow_id = str(flow.get("flow_id") or f"flow_{flow_index}")
        flow_kind = str(flow.get("kind") or "")
        for step_index, step in enumerate(_as_list(flow.get("steps"))):
            if not isinstance(step, Mapping):
                continue
            record = _operation_step_record(
                source="operation_flows",
                flow_id=flow_id,
                flow_kind=flow_kind,
                step_index=step_index,
                step=step,
            )
            records.append(record)

    for sequence_name, flow_kind in (("init_sequence", "init"), ("read_sequence", "read")):
        for step_index, step in enumerate(_as_list(device_ir.get(sequence_name))):
            if not isinstance(step, Mapping):
                continue
            record = _operation_step_record(
                source=sequence_name,
                flow_id=sequence_name,
                flow_kind=flow_kind,
                step_index=step_index,
                step=step,
            )
            records.append(record)
    return records


def _operation_step_record(
    *,
    source: str,
    flow_id: str,
    flow_kind: str,
    step_index: int,
    step: Mapping[str, Any],
) -> dict[str, Any]:
    transaction = step.get("transaction")
    transaction_kind = transaction.get("kind") if isinstance(transaction, Mapping) else None
    text = json.dumps(step, sort_keys=True, default=str)
    return {
        "source": source,
        "flow_id": flow_id,
        "flow_kind": flow_kind,
        "step_index": step_index,
        "op": step.get("op") or step.get("action") or step.get("kind"),
        "role": step.get("role"),
        "register": step.get("register") or step.get("pointer_target"),
        "transaction_kind": transaction_kind,
        "transaction": transaction if isinstance(transaction, Mapping) else None,
        "length": step.get("length") or (transaction.get("length") if isinstance(transaction, Mapping) else None),
        "notes": step.get("notes") or step.get("description"),
        "semantic_roles": _infer_roles([flow_id, flow_kind, text]),
        "raw_record": dict(step),
    }


def _extract_channels(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for index, channel in enumerate(_as_list(device_ir.get("read_channels"))):
        if not isinstance(channel, Mapping):
            continue
        records.append(
            {
                "index": index,
                "id": channel.get("id"),
                "raw_type": channel.get("raw_type"),
                "physical_unit": channel.get("physical_unit"),
                "flow_id": channel.get("flow_id"),
                "source_bytes": channel.get("source_bytes"),
                "source_signal": channel.get("source_signal"),
                "formula_id": channel.get("formula_id"),
                "notes": channel.get("notes"),
                "raw_record": dict(channel),
            }
        )
    return records


def _extract_formula_inputs(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for formula_index, formula in enumerate(_as_list(device_ir.get("conversion_formulae"))):
        if not isinstance(formula, Mapping):
            continue
        formula_id = str(formula.get("name") or formula.get("id") or f"formula_{formula_index}")
        expression = formula.get("integer_approximation_expression")
        if not isinstance(expression, Mapping):
            continue
        for input_index, item in enumerate(_as_list(expression.get("inputs"))):
            if not isinstance(item, Mapping):
                continue
            records.append(
                {
                    "formula_index": formula_index,
                    "formula_id": formula_id,
                    "input_index": input_index,
                    "name": item.get("name"),
                    "byte_source": item.get("byte_source"),
                    "source_signal": item.get("source_signal"),
                    "default_value": item.get("default_value"),
                    "config_source": item.get("config_source"),
                    "description": item.get("description"),
                    "expression": expression.get("expression"),
                    "output": expression.get("output"),
                    "raw_record": dict(item),
                }
            )
    return records


def _build_coverage_hints(
    device_ir: Mapping[str, Any],
    fact_bank: Mapping[str, Any],
) -> dict[str, Any]:
    registers = _as_list(fact_bank.get("registers"))
    channels = _as_list(fact_bank.get("channels"))
    formula_inputs = _as_list(fact_bank.get("formula_inputs"))
    operation_steps = _as_list(fact_bank.get("operation_steps"))
    flow_text = _operation_text_blob(operation_steps)
    default_only_init = _default_only_init_hints(device_ir, operation_steps)

    identity_registers = [reg for reg in registers if _has_role(reg, "identity")]
    config_registers = [
        reg for reg in registers
        if _has_any_role(reg, {"config", "control", "trigger"})
        and _is_writable_or_control_like(reg)
    ]
    result_registers = [
        reg for reg in registers
        if _has_any_role(reg, {"result", "data"}) and not _has_role(reg, "config")
    ]
    coefficient_registers = [reg for reg in registers if _has_role(reg, "coefficient")]

    return {
        "identity_registers": [_register_ref(reg) for reg in identity_registers],
        "identity_registers_not_referenced_by_flows": [
            _register_ref(reg) for reg in identity_registers if not _register_is_referenced(reg, flow_text)
        ],
        "config_registers": [_register_ref(reg) for reg in config_registers],
        "config_registers_not_referenced_by_flows": [
            _register_ref(reg)
            for reg in config_registers
            if not _register_is_referenced(reg, flow_text) and not default_only_init
        ],
        "result_registers": [_register_ref(reg) for reg in result_registers],
        "result_registers_not_referenced_by_flows": [
            _register_ref(reg) for reg in result_registers if not _register_is_referenced(reg, flow_text)
        ],
        "coefficient_registers": [_register_ref(reg) for reg in coefficient_registers],
        "channels_without_source_bytes": [
            _channel_ref(channel) for channel in channels if not _channel_has_source_binding(channel)
        ],
        "formula_inputs_without_byte_source": [
            _formula_input_ref(item) for item in formula_inputs if not _formula_input_has_source_binding(item)
        ],
        "non_contiguous_result_hints": _non_contiguous_byte_hints(result_registers),
        "default_only_init": default_only_init,
    }


def _default_only_init_hints(
    device_ir: Mapping[str, Any],
    operation_steps: list[Any],
) -> list[dict[str, Any]]:
    hints: list[dict[str, Any]] = []
    by_source: dict[tuple[str, str], list[Mapping[str, Any]]] = defaultdict(list)
    for step in operation_steps:
        if not isinstance(step, Mapping):
            continue
        if str(step.get("flow_kind") or "") != "init":
            continue
        by_source[(str(step.get("source") or ""), str(step.get("flow_id") or ""))].append(step)

    for (source, flow_id), steps in by_source.items():
        if any(_is_bus_step(step) for step in steps):
            continue
        text = " ".join(_stringify(step) for step in steps)
        if _text_matches_any(text, DEFAULT_INIT_PATTERNS):
            hints.append({"source": source, "flow_id": flow_id, "reason": "default_or_no_bus_init_note"})

    if not hints and not _as_list(device_ir.get("operation_flows")):
        init_steps = _as_list(device_ir.get("init_sequence"))
        if init_steps and not any(_step_has_bus_transaction(step) for step in init_steps if isinstance(step, Mapping)):
            text = " ".join(_stringify(step) for step in init_steps)
            if _text_matches_any(text, DEFAULT_INIT_PATTERNS):
                hints.append({"source": "init_sequence", "flow_id": "init_sequence", "reason": "default_or_no_bus_init_note"})
    return hints


def _non_contiguous_byte_hints(registers: list[Any]) -> list[dict[str, Any]]:
    grouped: dict[str, dict[str, list[dict[str, Any]]]] = defaultdict(lambda: {"high": [], "low": []})
    for reg in registers:
        if not isinstance(reg, Mapping):
            continue
        part = str(reg.get("byte_part") or "")
        if part not in {"high", "low"}:
            continue
        family = _byte_family_key(reg)
        if not family:
            continue
        grouped[family][part].append(reg)

    hints: list[dict[str, Any]] = []
    for family, parts in grouped.items():
        for high in parts["high"]:
            high_addr = _hex_to_int(high.get("address"))
            if high_addr is None:
                continue
            for low in parts["low"]:
                low_addr = _hex_to_int(low.get("address"))
                if low_addr is None:
                    continue
                if abs(low_addr - high_addr) > 1:
                    hints.append(
                        {
                            "family_key": family,
                            "high": _register_ref(high),
                            "low": _register_ref(low),
                            "address_delta": abs(low_addr - high_addr),
                        }
                    )
    return hints


def _infer_roles(parts: Iterable[Any]) -> list[str]:
    text = " ".join(str(part or "") for part in parts)
    normalised = _normalise_text(text)
    compact = _compact_text(normalised)
    tokens = set(re.findall(r"[a-z0-9]+", normalised))
    roles: list[str] = []
    for role, patterns in ROLE_PATTERNS.items():
        if any(_pattern_matches(pattern, normalised, compact, tokens) for pattern in patterns):
            roles.append(role)

    # A bare "id" token is too broad unless the record is read-only or already
    # looks like a register identifier field.
    if "identity" in roles and "id" in tokens:
        access_tokens = {"ro", "read", "readonly", "readable"}
        if not (tokens & access_tokens or "who" in tokens or "product" in tokens or "chip" in tokens or "device" in tokens):
            roles.remove("identity")
    return roles


def _infer_byte_part(parts: Iterable[Any]) -> str | None:
    text = " ".join(str(part or "") for part in parts)
    normalised = _normalise_text(text)
    compact = _compact_text(normalised)
    tokens = set(re.findall(r"[a-z0-9]+", normalised))
    for part, patterns in BYTE_PART_PATTERNS.items():
        if any(_pattern_matches(pattern, normalised, compact, tokens) for pattern in patterns):
            return part
    return None


def _pattern_matches(pattern: str, normalised: str, compact: str, tokens: set[str]) -> bool:
    pattern_norm = _normalise_text(pattern)
    pattern_compact = _compact_text(pattern_norm)
    if " " in pattern_norm:
        return pattern_norm in normalised or pattern_compact in compact
    if pattern_norm in tokens:
        return True
    # Compact aliases catch spellings such as WHO_AM_I -> whoami.  General
    # single-token substring matching is intentionally avoided because it turns
    # MODEL_ID into a false "mode" hit.
    return pattern_compact in {"whoami", "oneshot"} and pattern_compact in compact


def _register_is_referenced(register: Mapping[str, Any], flow_text: str) -> bool:
    name = str(register.get("name") or "").strip()
    address = str(register.get("address") or "").strip()
    if name and _register_name_is_referenced(name, flow_text):
        return True
    if address and address.lower() in flow_text:
        return True
    raw = register.get("raw_record")
    if isinstance(raw, Mapping):
        aliases = [
            raw.get("symbol"),
            raw.get("alias"),
            raw.get("short_name"),
            raw.get("register"),
        ]
        if any(alias and _register_name_is_referenced(str(alias), flow_text) for alias in aliases):
            return True
    return False


def _register_name_is_referenced(name: str, flow_text: str) -> bool:
    name_norm = _normalise_text(name)
    name_tokens = re.findall(r"[a-z0-9]+", name_norm)
    if not name_tokens:
        return False
    flow_norm = _normalise_text(flow_text)
    flow_tokens = set(re.findall(r"[a-z0-9]+", flow_norm))
    compact_name = _compact_text(name_norm)
    compact_flow = _compact_text(flow_norm)
    if len(name_tokens) == 1:
        token = name_tokens[0]
        if len(token) <= 4:
            return token in flow_tokens
        return token in flow_tokens or token in compact_flow
    return all(token in flow_tokens for token in name_tokens) or compact_name in compact_flow


def _operation_text_blob(operation_steps: list[Any]) -> str:
    parts: list[str] = []
    for step in operation_steps:
        if not isinstance(step, Mapping):
            continue
        for key in (
            "flow_id",
            "flow_kind",
            "op",
            "role",
            "register",
            "transaction_kind",
            "transaction",
            "notes",
            "semantic_roles",
        ):
            parts.append(_stringify(step.get(key)))
    return _normalise_text(" ".join(parts))


def _byte_family_key(register: Mapping[str, Any]) -> str:
    text = f"{register.get('name') or ''} {register.get('description') or ''}"
    tokens = re.findall(r"[a-z0-9]+", _normalise_text(text))
    stopwords = {
        "register",
        "reg",
        "byte",
        "bytes",
        "high",
        "low",
        "msb",
        "lsb",
        "upper",
        "lower",
        "data",
        "result",
        "output",
        "value",
        "raw",
        "rd",
        "read",
    }
    family = [token for token in tokens if token not in stopwords]
    return "_".join(family[:6])


def _is_bus_step(step: Mapping[str, Any]) -> bool:
    op = str(step.get("op") or "").strip()
    transaction_kind = str(step.get("transaction_kind") or "").strip()
    return op in BUS_STEP_OPS or transaction_kind in BUS_STEP_OPS


def _step_has_bus_transaction(step: Mapping[str, Any]) -> bool:
    transaction = step.get("transaction")
    if isinstance(transaction, Mapping) and transaction.get("kind") in BUS_STEP_OPS:
        return True
    op = str(step.get("op") or step.get("action") or step.get("kind") or "")
    return op in BUS_STEP_OPS


def _is_writable_or_control_like(register: Mapping[str, Any]) -> bool:
    access = _normalise_text(register.get("access") or "")
    if _access_text_is_writable(access):
        return True
    return _has_any_role(register, {"control", "trigger"})


def _access_text_is_writable(access: Any) -> bool:
    text = _normalise_text(access or "")
    tokens = set(re.findall(r"[a-z0-9]+", text))
    return bool(tokens & {"w", "wr", "rw", "write", "writable", "writeable"})


def _append_role(roles: list[str], role: str) -> list[str]:
    if role not in roles:
        roles.append(role)
    return roles


def _has_role(record: Mapping[str, Any], role: str) -> bool:
    roles = record.get("semantic_roles")
    return isinstance(roles, list) and role in roles


def _has_any_role(record: Mapping[str, Any], roles: set[str]) -> bool:
    current = record.get("semantic_roles")
    return isinstance(current, list) and any(role in roles for role in current)


def _register_ref(register: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "name": register.get("name"),
        "address": register.get("address"),
        "access": register.get("access"),
        "semantic_roles": register.get("semantic_roles") or [],
    }


def _channel_ref(channel: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "id": channel.get("id"),
        "flow_id": channel.get("flow_id"),
        "formula_id": channel.get("formula_id"),
        "raw_type": channel.get("raw_type"),
        "physical_unit": channel.get("physical_unit"),
        "source_signal": channel.get("source_signal"),
    }


def _formula_input_ref(item: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "formula_id": item.get("formula_id"),
        "name": item.get("name"),
        "description": item.get("description"),
        "source_signal": item.get("source_signal"),
        "default_value": item.get("default_value"),
        "config_source": item.get("config_source"),
    }


def _format_register_ref(register: Mapping[str, Any]) -> str:
    name = register.get("name") or "<unnamed>"
    address = register.get("address") or "<no address>"
    return f"{name}@{address}"


def _append_candidate_rows(
    lines: list[str],
    title: str,
    value: Any,
    formatter,
    *,
    limit: int,
) -> None:
    items = _as_list(value)
    if not items:
        return
    lines.append(f"{title}:")
    for item in items[:limit]:
        lines.append(f"- {formatter(item)}")
    if len(items) > limit:
        lines.append(f"- ... {len(items) - limit} more")


def _format_candidate_operation(item: Any) -> str:
    if not isinstance(item, Mapping):
        return _short(item)
    steps = []
    for step in _as_list(item.get("steps"))[:6]:
        if not isinstance(step, Mapping):
            continue
        steps.append(
            f"{step.get('op')}:{step.get('target')}:{_short(step.get('details'), 80)}"
        )
    return (
        f"{item.get('flow_id')} kind={item.get('kind')} "
        f"channels={','.join(_as_str_list(item.get('channels')))} "
        f"outputs={','.join(_as_str_list(item.get('outputs')))} "
        f"summary={_short(item.get('summary'))} steps=[{'; '.join(steps)}]"
    )


def _register_key_set(
    registers: list[Any],
    *,
    name_key: str,
    value_key: str,
) -> set[str]:
    keys: set[str] = set()
    for item in registers:
        if not isinstance(item, Mapping):
            continue
        name = _compact_text(item.get(name_key))
        value = _normalise_hex(item.get(value_key))
        if name:
            keys.add(f"name:{name}")
        if value:
            keys.add(f"value:{value}")
        if name and value:
            keys.add(f"pair:{name}:{value}")
    return keys


def _candidate_register_is_projected(
    candidate: Mapping[str, Any],
    projected_register_keys: set[str],
) -> bool:
    name = _compact_text(candidate.get("name"))
    value = _normalise_hex(candidate.get("value"))
    if name and value and f"pair:{name}:{value}" in projected_register_keys:
        return True
    if name and f"name:{name}" in projected_register_keys:
        return True
    if value and f"value:{value}" in projected_register_keys:
        return True
    return False


def _candidate_operation_is_projected(candidate: Mapping[str, Any], projected_step_text: str) -> bool:
    flow_id = _compact_text(candidate.get("flow_id"))
    if flow_id and flow_id in _compact_text(projected_step_text):
        return True
    for step in _as_list(candidate.get("steps")):
        if not isinstance(step, Mapping):
            continue
        target = str(step.get("target") or "").strip()
        if target and _register_name_is_referenced(target, projected_step_text):
            return True
    return False


def _candidate_address_ref(item: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "value": _normalise_hex(item.get("value")),
        "addressing_form": item.get("addressing_form"),
        "is_default": item.get("is_default"),
        "description": item.get("description"),
    }


def _candidate_register_ref(item: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "name": item.get("name"),
        "value": _normalise_hex(item.get("value")),
        "access": item.get("access"),
        "semantic_roles": _as_str_list(item.get("semantic_roles")),
        "description": item.get("description"),
    }


def _candidate_operation_ref(item: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "flow_id": item.get("flow_id"),
        "kind": item.get("kind"),
        "channels": _as_str_list(item.get("channels")),
        "summary": item.get("summary"),
    }


def _first_hex_from_mapping(mapping: Mapping[str, Any], keys: Iterable[str]) -> str | None:
    for key in keys:
        value = mapping.get(key)
        parsed = _normalise_hex(value)
        if parsed:
            return parsed
    parsed = _normalise_hex(_stringify(mapping))
    return parsed


def _normalise_hex(value: Any) -> str | None:
    if value is None:
        return None
    if isinstance(value, int):
        return f"0x{value:02X}" if 0 <= value <= 0xFF else f"0x{value:X}"
    text = str(value)
    match = re.search(r"0x[0-9A-Fa-f]+", text)
    if not match:
        return None
    number = int(match.group(0), 16)
    return f"0x{number:02X}" if 0 <= number <= 0xFF else f"0x{number:X}"


def _hex_to_int(value: Any) -> int | None:
    if value is None:
        return None
    text = str(value).strip()
    if not text:
        return None
    try:
        return int(text, 16) if text.lower().startswith("0x") else int(text)
    except ValueError:
        return None


def _first_text(mapping: Mapping[str, Any], keys: Iterable[str]) -> str:
    for key in keys:
        value = mapping.get(key)
        if value is not None and str(value).strip():
            return str(value)
    return ""


def _as_list(value: Any) -> list[Any]:
    return value if isinstance(value, list) else []


def _as_str_list(value: Any) -> list[str]:
    if not isinstance(value, list):
        return []
    return [str(item) for item in value if str(item or "").strip()]


def _non_empty_list(value: Any) -> bool:
    return isinstance(value, list) and any(str(item or "").strip() for item in value)


def _channel_has_source_binding(channel: Mapping[str, Any]) -> bool:
    if _non_empty_list(channel.get("source_bytes")):
        return True
    return bool(str(channel.get("source_signal") or "").strip())


def _formula_input_has_source_binding(item: Mapping[str, Any]) -> bool:
    if str(item.get("byte_source") or "").strip():
        return True
    if str(item.get("source_signal") or "").strip():
        return True
    if str(item.get("config_source") or "").strip():
        return True
    return item.get("default_value") is not None


def _text_matches_any(text: str, patterns: Iterable[re.Pattern[str]]) -> bool:
    return any(pattern.search(text or "") for pattern in patterns)


def _normalise_text(value: Any) -> str:
    text = str(value or "").lower()
    text = text.replace("_", " ")
    text = re.sub(r"\s+", " ", text)
    return text.strip()


def _compact_text(value: Any) -> str:
    return re.sub(r"[^a-z0-9]+", "", _normalise_text(value))


def _stringify(value: Any) -> str:
    if isinstance(value, (Mapping, list, tuple)):
        return json.dumps(value, sort_keys=True, default=str)
    return str(value or "")


def _short(value: Any, limit: int = 120) -> str:
    text = str(value or "").replace("\n", " ").strip()
    text = re.sub(r"\s+", " ", text)
    if len(text) <= limit:
        return text
    return text[: max(0, limit - 3)].rstrip() + "..."
