"""Deterministic readiness checks for flow-audited Device IRs."""
from __future__ import annotations

import re
from typing import Any, Mapping

from .models import ValidationIssue, ValidationResult

CRITICAL_CHECKS: frozenset[str] = frozenset({
    "trigger_or_mode",
    "wait_or_poll",
    "result_read",
    "clear_or_ack",
    "config_or_calibration",
    "byte_source",
})

UNCERTAIN_BLOCKING_CHECKS: frozenset[str] = frozenset({
    "trigger_or_mode",
    "result_read",
    "config_or_calibration",
    "byte_source",
})

EXTERNAL_KNOWLEDGE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bcommon\s+usage\b", re.IGNORECASE),
    re.compile(r"\bknown\s+register\s+sequence\b", re.IGNORECASE),
    re.compile(r"\btypical\s+driver\s+practice\b", re.IGNORECASE),
    re.compile(r"\bprior\s+knowledge\b", re.IGNORECASE),
    re.compile(r"\binferred\s+from\s+common\b", re.IGNORECASE),
)

REQUIRES_HUMAN_BLOCKING_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bcommon\s+usage\b", re.IGNORECASE),
    re.compile(r"\binferred\b", re.IGNORECASE),
    re.compile(r"\bneeds?\s+verification\b", re.IGNORECASE),
    re.compile(r"\bnot\s+detailed\b", re.IGNORECASE),
    re.compile(r"\bdoes\s+not\s+provide\b", re.IGNORECASE),
    re.compile(r"\bnot\s+provided\b", re.IGNORECASE),
    re.compile(r"\brequires?\s+api\b", re.IGNORECASE),
)

REQUIRES_HUMAN_FLOW_CONTEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(register|opcode|command|sequence|flow)\b", re.IGNORECASE),
    re.compile(r"\b(init|initiali[sz]ation|calibration|config)\b", re.IGNORECASE),
    re.compile(r"\b(trigger|start|mode|poll|ready|status)\b", re.IGNORECASE),
    re.compile(r"\b(result|read|write|byte|source|window|fifo|clear|ack)\b", re.IGNORECASE),
)

REQUIRES_HUMAN_OPTIONAL_NONBLOCKING_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bleft\s+to\s+software\b", re.IGNORECASE),
    re.compile(r"\bnot\s+(used|enabled|selected)\b", re.IGNORECASE),
    re.compile(r"\boptional\b", re.IGNORECASE),
    re.compile(r"\bdisabled\b", re.IGNORECASE),
    re.compile(r"\bspi\s+modes?\b.*\b(supports?|supported|allowed|valid)\b", re.IGNORECASE),
)

CONFIG_REGISTER_KEYWORDS: frozenset[str] = frozenset({
    "cfg",
    "conf",
    "config",
    "control",
    "ctrl",
    "enable",
    "gain",
    "mode",
    "odr",
    "power",
    "rate",
    "setup",
})

CONFIG_REGISTER_EXCLUDE_KEYWORDS: frozenset[str] = frozenset({
    "data",
    "fifo",
    "id",
    "interrupt",
    "result",
    "status",
    "threshold",
})

DEFAULT_CONFIG_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(power[-\s]?up|reset)\s+defaults?\b", re.IGNORECASE),
    re.compile(r"\breset\s+(state|value)\b", re.IGNORECASE),
    re.compile(r"\bfactory\s+defaults?\b", re.IGNORECASE),
    re.compile(r"\bdefaults?\s+(select|configure|enable|set|use|leave)\b", re.IGNORECASE),
    re.compile(r"\b(no|without)\s+explicit\s+(setup|config|configuration|mode)\b", re.IGNORECASE),
    re.compile(r"\b(config|configuration|mode)\s+(write|writes)\s+(is|are)\s+not\s+required\b", re.IGNORECASE),
)

DOCUMENTED_DEFAULT_CONFIG_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(default|defaults)\s+(is|are|to|value|state|setting|resolution)\b", re.IGNORECASE),
    re.compile(r"\b(power[-\s]?up|reset|factory)\s+defaults?\b", re.IGNORECASE),
    re.compile(r"\b(reset|factory)\s+(state|value|setting)\b", re.IGNORECASE),
)

DEFAULT_CONFIG_RELIANCE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\brel(?:y|ies|ying)\s+on\s+(the\s+)?(documented\s+)?default\b", re.IGNORECASE),
    re.compile(r"\buses?\s+(the\s+)?(documented\s+)?default\b", re.IGNORECASE),
    re.compile(r"\bleaves?\s+.*\b(default|reset)\b", re.IGNORECASE),
    re.compile(r"\b(no|without)\s+explicit\s+(setup|config|configuration|mode)\b", re.IGNORECASE),
    re.compile(r"\b(config|configuration|mode)\s+(write|writes)\s+(is|are)?\s*not\s+(needed|required)\b", re.IGNORECASE),
)

NON_DEFAULT_OPTIONAL_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(non[-\s]?default|custom|alternate)\b.*\b(if|optional|desired|needed|required)\b", re.IGNORECASE),
    re.compile(r"\bif\b.*\b(non[-\s]?default|custom|alternate)\b", re.IGNORECASE),
)

I2C_ADDRESS_BYTE_CONTEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(device|slave|wire|control)\s+(address\s+)?(byte|word)\b", re.IGNORECASE),
    re.compile(r"\b(read|write)\s+(slave|device|wire|control)?\s*address\s+(byte|word)\b", re.IGNORECASE),
    re.compile(r"\bR\s*/?\s*W\b", re.IGNORECASE),
    re.compile(r"\bdata\s+direction\s+bit\b", re.IGNORECASE),
    re.compile(r"\b8[-\s]?bit\s+address\b", re.IGNORECASE),
)

WRITE_COMPLETION_CONTEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\back\s+poll(?:ing)?\b", re.IGNORECASE),
    re.compile(r"\bwrite\s+cycle\b", re.IGNORECASE),
    re.compile(r"\bself[-\s]?timed\b", re.IGNORECASE),
    re.compile(r"\binternally[-\s]?timed\b", re.IGNORECASE),
    re.compile(r"\bt[_\s-]*wr\b", re.IGNORECASE),
    re.compile(r"\bnon[-\s]?volatile\b", re.IGNORECASE),
    re.compile(r"\b(program|erase)\s+(time|cycle|busy)\b", re.IGNORECASE),
    re.compile(r"\b(inputs?|commands?)\s+(are\s+)?disabled\s+during\b", re.IGNORECASE),
)

MEASUREMENT_COMPLETION_CONTEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bconversion\s+time\b", re.IGNORECASE),
    re.compile(r"\bmeasurement\s+time\b", re.IGNORECASE),
    re.compile(r"\bt[_\s-]*(conv|meas)\b", re.IGNORECASE),
    re.compile(r"\bdata\s+ready\b", re.IGNORECASE),
    re.compile(r"\bready\s+(bit|flag|pin|signal)\b", re.IGNORECASE),
    re.compile(r"\bbusy\s+(bit|flag|pin|signal)\b", re.IGNORECASE),
    re.compile(r"\bend\s+of\s+conversion\b", re.IGNORECASE),
    re.compile(r"\beoc\b", re.IGNORECASE),
    re.compile(r"\bdrdy\b", re.IGNORECASE),
)

INTEGRITY_CONTEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bcrc(?:[-\s]?\d+)?\b", re.IGNORECASE),
    re.compile(r"\bchecksum\b", re.IGNORECASE),
    re.compile(r"\bpec\b", re.IGNORECASE),
    re.compile(r"\bparity\b", re.IGNORECASE),
)

COMPLETION_STEP_OPS: frozenset[str] = frozenset({
    "delay",
    "poll_until",
    "wait_until_ready",
    "wait_signal",
    "measure_pulse",
})

WRITE_LIKE_FLOW_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(byte|page|block|sector|word)\s+write\b", re.IGNORECASE),
    re.compile(r"\bwrite_(byte|page|block|sector|word)\b", re.IGNORECASE),
    re.compile(r"\b(program|erase|store)\b", re.IGNORECASE),
    re.compile(r"\b(non[-\s]?volatile|eeprom|flash|memory)\b.*\bwrite\b", re.IGNORECASE),
    re.compile(r"\bwrite\b.*\b(non[-\s]?volatile|eeprom|flash|memory)\b", re.IGNORECASE),
)

TRIGGER_LIKE_STEP_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(start|trigger|initiate|begin|one[-\s]?shot)\b", re.IGNORECASE),
    re.compile(r"\b(measure|measurement|convert|conversion)\b", re.IGNORECASE),
)

CHIP_SELECT_SIGNAL_NAMES: frozenset[str] = frozenset({
    "cs",
    "ncs",
    "csb",
    "nss",
    "ss",
    "chip_select",
    "chipselect",
    "slave_select",
    "select",
})

SET_SIGNAL_TRIGGER_SIGNAL_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\btrig(?:ger)?\b", re.IGNORECASE),
    re.compile(r"\bstart\b", re.IGNORECASE),
    re.compile(r"\bconv(?:ert|ersion)?\b", re.IGNORECASE),
    re.compile(r"\bmeasure(?:ment)?\b", re.IGNORECASE),
)

SET_SIGNAL_TRIGGER_TEXT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\b(start|trigger|initiate|begin)\s+(a\s+)?(measurement|conversion|ranging|sample)\b", re.IGNORECASE),
    re.compile(r"\b(one[-\s]?shot|single[-\s]?shot)\b", re.IGNORECASE),
)


def assess_ir_flow_risk(
    device_ir: Mapping[str, Any],
    flow_audit_receipt: Mapping[str, Any] | None = None,
    *,
    source_context: str | None = None,
) -> ValidationResult:
    """Return a generation-readiness report for flow-level IR quality."""

    issues: list[ValidationIssue] = []
    if not isinstance(device_ir, Mapping):
        return ValidationResult(
            ok=False,
            issues=[
                ValidationIssue(
                    "error",
                    "device_ir.flow_risk.payload",
                    "device_ir must be an object before flow-risk assessment.",
                )
            ],
        )

    _check_audit_findings(
        flow_audit_receipt,
        issues,
        operation_flow_ids=_operation_flow_ids(device_ir),
    )
    _check_external_knowledge_in_ir(device_ir, issues)
    _check_requires_human_notes(device_ir, issues)
    _check_formula_flow_dependencies(device_ir, issues)
    _check_formula_input_bindings(device_ir, issues)
    _check_read_flow_outputs(device_ir, issues)
    _check_i2c_address_byte_payloads(device_ir, issues)
    _check_non_contiguous_result_byte_reads(device_ir, issues, source_context)
    _check_config_register_coverage(device_ir, flow_audit_receipt, issues)
    _check_completion_condition_coverage(device_ir, issues, source_context)
    _check_integrity_condition_coverage(device_ir, issues, source_context)

    has_error = any(issue.level == "error" for issue in issues)
    return ValidationResult(ok=not has_error, issues=issues)


def repair_formula_flow_dependencies(device_ir: dict[str, Any]) -> list[dict[str, Any]]:
    """Deterministically declare cross-channel formula flow dependencies."""

    if not isinstance(device_ir, dict):
        return []
    flows = device_ir.get("operation_flows")
    channels = device_ir.get("read_channels")
    if not isinstance(flows, list) or not isinstance(channels, list):
        return []

    formulas_by_name = _formula_map(device_ir)
    source_to_channel = _channel_source_map(device_ir)
    flows_by_channel = _flows_by_channel(device_ir)
    all_channel_ids = {
        str(channel.get("id") or "").strip()
        for channel in channels
        if isinstance(channel, Mapping) and str(channel.get("id") or "").strip()
    }
    changes: list[dict[str, Any]] = []
    dependencies_by_flow: dict[str, set[str]] = {}

    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        formula_id = str(channel.get("formula_id") or "").strip()
        if not channel_id or not formula_id:
            continue
        formula = formulas_by_name.get(formula_id)
        target_flow = flows_by_channel.get(channel_id)
        target_flow_id = _flow_id(target_flow)
        if not formula or not isinstance(target_flow, dict) or not target_flow_id:
            continue
        dependencies = _formula_channel_dependencies(
            formula,
            source_to_channel,
            all_channel_ids,
        )
        dependencies.discard(channel_id)
        if not dependencies:
            continue
        for dep_channel in sorted(dependencies):
            dep_flow = flows_by_channel.get(dep_channel)
            dep_flow_id = _flow_id(dep_flow)
            if not dep_flow_id or dep_flow_id == target_flow_id:
                continue
            dependencies_by_flow.setdefault(target_flow_id, set()).add(dep_flow_id)
            flow_text = _normalise(" ".join(_flow_strings(target_flow)))
            if dep_channel in flow_text or f"read_{dep_channel}" in flow_text:
                continue
            precondition = (
                f"{dep_flow_id} completed before {channel_id} formula evaluation"
            )
            existing = target_flow.get("preconditions")
            if not isinstance(existing, list):
                existing = []
                target_flow["preconditions"] = existing
            if precondition not in existing:
                existing.append(precondition)
                changes.append({
                    "kind": "precondition_added",
                    "channel": channel_id,
                    "depends_on_channel": dep_channel,
                    "target_flow_id": target_flow_id,
                    "dependency_flow_id": dep_flow_id,
                    "precondition": precondition,
                })

    if _reorder_flows_for_dependencies(device_ir, dependencies_by_flow):
        changes.append({
            "kind": "flow_order_updated",
            "reason": "formula producer flows moved before dependent consumer flows",
        })
    return changes


def _check_audit_findings(
    receipt: Mapping[str, Any] | None,
    issues: list[ValidationIssue],
    *,
    operation_flow_ids: set[str],
) -> None:
    if not isinstance(receipt, Mapping):
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.flow_risk.audit_missing",
                "No device_ir_flow_audit.json receipt found; flow readiness is not fully audited.",
            )
        )
        return

    findings = receipt.get("audit_findings")
    if not isinstance(findings, list) or not findings:
        issues.append(
            ValidationIssue(
                "warning",
                "device_ir.flow_risk.audit_findings",
                "Flow audit did not return audit_findings; lifecycle coverage is not traceable.",
            )
        )
        return

    for flow_index, finding in enumerate(findings):
        if not isinstance(finding, Mapping):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.audit_findings[{flow_index}]",
                    "audit finding must be an object.",
                )
            )
            continue
        flow_id = str(finding.get("flow_id") or f"index_{flow_index}")
        checks = finding.get("checks")
        if not isinstance(checks, list) or not checks:
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.{flow_id}.checks",
                    "flow audit finding has no checklist entries.",
                )
            )
            continue
        seen: set[str] = set()
        for check_index, check in enumerate(checks):
            if not isinstance(check, Mapping):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.flow_risk.{flow_id}.checks[{check_index}]",
                        "check entry must be an object.",
                    )
                )
                continue
            name = str(check.get("name") or "").strip()
            status = str(check.get("status") or "").strip()
            evidence = str(check.get("evidence") or "")
            action = str(check.get("action") or "")
            seen.add(name)
            if name in CRITICAL_CHECKS and status == "missing_required":
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.flow_risk.{flow_id}.{name}",
                        f"flow audit marks required lifecycle check as missing: {evidence}",
                    )
                )
            elif name in UNCERTAIN_BLOCKING_CHECKS and status == "uncertain":
                level = "warning" if _uncertain_audit_check_is_nonblocking(name, evidence, action) else "error"
                if level == "warning":
                    message = (
                        "flow audit marks a critical lifecycle check as uncertain, "
                        "but the evidence says the current flow intentionally relies "
                        f"on a documented default and only non-default setup is optional: {evidence}"
                    )
                else:
                    message = f"flow audit marks critical lifecycle check as uncertain: {evidence}"
                issues.append(
                    ValidationIssue(
                        level,
                        f"device_ir.flow_risk.{flow_id}.{name}",
                        message,
                    )
                )
            elif name in CRITICAL_CHECKS and status == "uncertain":
                issues.append(
                    ValidationIssue(
                        "warning",
                        f"device_ir.flow_risk.{flow_id}.{name}",
                        f"flow audit marks lifecycle check as uncertain: {evidence}",
                    )
                )
            if _contains_external_knowledge(evidence):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.flow_risk.{flow_id}.{name}.evidence",
                        "audit evidence relies on common usage/prior knowledge instead of supplied datasheet content.",
                    )
                )
        missing_checks = CRITICAL_CHECKS - seen
        if missing_checks:
            if _audit_finding_requires_complete_checklist(flow_id, operation_flow_ids):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.flow_risk.{flow_id}.checks",
                        f"flow audit checklist is incomplete; missing {sorted(missing_checks)}.",
                    )
                )
            else:
                issues.append(
                    ValidationIssue(
                        "warning",
                        f"device_ir.flow_risk.{flow_id}.checks",
                        "aggregate flow audit finding has a partial checklist; "
                        "complete lifecycle checks are enforced only for concrete operation_flows.",
                    )
                )


def _audit_finding_requires_complete_checklist(
    flow_id: str,
    operation_flow_ids: set[str],
) -> bool:
    if flow_id in operation_flow_ids:
        return True
    return not _is_aggregate_audit_finding_id(flow_id)


def _is_aggregate_audit_finding_id(flow_id: str) -> bool:
    normalized = _normalise(flow_id)
    if not normalized:
        return False
    return (
        normalized.endswith("_coverage")
        or normalized.startswith("coverage_")
        or normalized.startswith("global_")
        or normalized.startswith("cross_flow_")
        or normalized in {"coverage", "summary", "global_summary"}
    )


def _uncertain_audit_check_is_nonblocking(
    name: str,
    evidence: str,
    action: str,
) -> bool:
    if name != "config_or_calibration":
        return False
    text = f"{evidence} {action}"
    if _matches_any(text, REQUIRES_HUMAN_BLOCKING_PATTERNS) and not _matches_any(
        text,
        NON_DEFAULT_OPTIONAL_PATTERNS,
    ):
        return False
    has_documented_default = _matches_any(text, DOCUMENTED_DEFAULT_CONFIG_PATTERNS)
    intentionally_uses_default = _matches_any(text, DEFAULT_CONFIG_RELIANCE_PATTERNS)
    non_default_is_optional = _matches_any(text, NON_DEFAULT_OPTIONAL_PATTERNS)
    return has_documented_default and (intentionally_uses_default or non_default_is_optional)


def _check_external_knowledge_in_ir(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    for path, value in _walk_strings(device_ir):
        if _contains_external_knowledge(value):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.external_knowledge.{path}",
                    (
                        "IR text says a flow fact came from common usage/prior "
                        "knowledge; mark it requires_human instead of using it "
                        "as grounded input."
                    ),
                )
            )


def _check_requires_human_notes(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    notes = device_ir.get("requires_human")
    if not isinstance(notes, list):
        return
    for index, note in enumerate(notes):
        text = str(note or "")
        if _matches_any(text, REQUIRES_HUMAN_BLOCKING_PATTERNS):
            if _matches_any(text, REQUIRES_HUMAN_OPTIONAL_NONBLOCKING_PATTERNS):
                issues.append(
                    ValidationIssue(
                        "warning",
                        f"device_ir.flow_risk.requires_human[{index}]",
                        "requires_human records unresolved optional or software-handled "
                        f"information; not blocking flow readiness: {text}",
                    )
                )
                continue
            if not _matches_any(text, REQUIRES_HUMAN_FLOW_CONTEXT_PATTERNS):
                issues.append(
                    ValidationIssue(
                        "warning",
                        f"device_ir.flow_risk.requires_human[{index}]",
                        "requires_human records unresolved non-flow information; "
                        f"not blocking flow readiness: {text}",
                    )
                )
                continue
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.requires_human[{index}]",
                    "requires_human records unresolved generation-critical flow information: "
                    f"{text}",
                )
            )


def _check_formula_flow_dependencies(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    formulas_by_name = _formula_map(device_ir)
    source_to_channel = _channel_source_map(device_ir)
    flows_by_channel = _flows_by_channel(device_ir)
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return
    all_channel_ids = {
        str(channel.get("id") or "").strip()
        for channel in channels
        if isinstance(channel, Mapping) and str(channel.get("id") or "").strip()
    }

    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        formula_id = str(channel.get("formula_id") or "").strip()
        if not channel_id or not formula_id:
            continue
        formula = formulas_by_name.get(formula_id)
        if not formula:
            continue
        dependencies = _formula_channel_dependencies(
            formula,
            source_to_channel,
            all_channel_ids,
        )
        dependencies.discard(channel_id)
        if not dependencies:
            continue
        flow = flows_by_channel.get(channel_id)
        flow_text = _normalise(" ".join(_flow_strings(flow))) if isinstance(flow, Mapping) else ""
        missing = [
            dep for dep in sorted(dependencies)
            if dep not in flow_text and f"read_{dep}" not in flow_text
        ]
        if missing:
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.formula_dependency.{channel_id}",
                    "formula "
                    f"{formula_id!r} depends on data from channel(s) {missing}, "
                    "but the channel's operation flow does not declare/read those producer flows.",
                )
            )


def _check_formula_input_bindings(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    formulas = device_ir.get("conversion_formulae")
    if not isinstance(formulas, list):
        return
    for formula_index, formula in enumerate(formulas):
        if not isinstance(formula, Mapping):
            continue
        formula_id = str(formula.get("name") or f"formula_{formula_index}")
        expr = formula.get("integer_approximation_expression")
        if not isinstance(expr, Mapping):
            continue
        inputs = expr.get("inputs")
        if not isinstance(inputs, list):
            continue
        for input_index, item in enumerate(inputs):
            if not isinstance(item, Mapping):
                continue
            if _formula_input_has_source_binding(item):
                continue
            input_name = str(item.get("name") or f"input_{input_index}")
            issues.append(
                ValidationIssue(
                    "warning",
                    f"device_ir.flow_risk.formula_input_binding.{formula_id}.{input_name}",
                    "formula input has no byte_source, source_signal, default_value, "
                    "or config_source binding; downstream codegen may not know how "
                    "to supply this value.",
                )
            )


def _check_read_flow_outputs(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return
    for flow_index, flow in enumerate(flows):
        if not isinstance(flow, Mapping):
            continue
        if str(flow.get("kind") or "") != "read" or flow.get("requires_human") is True:
            continue
        channels = [
            str(channel or "").strip()
            for channel in flow.get("channels", [])
            if str(channel or "").strip()
        ] if isinstance(flow.get("channels"), list) else []
        if not channels:
            continue
        output_channels = {
            str(output.get("channel") or "").strip()
            for output in flow.get("outputs", [])
            if isinstance(output, Mapping)
        } if isinstance(flow.get("outputs"), list) else set()
        missing = [channel for channel in channels if channel not in output_channels]
        if not missing:
            continue
        flow_id = str(flow.get("flow_id") or f"index_{flow_index}")
        issues.append(
            ValidationIssue(
                "error",
                f"device_ir.flow_risk.{flow_id}.read_outputs",
                "read flow declares channel(s) but does not produce outputs: "
                f"{missing}. Command-only start/config flows should use kind "
                "'write' or 'other', or the read flow should bind concrete outputs.",
            )
        )


def _check_i2c_address_byte_payloads(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
) -> None:
    if _normalise(str(device_ir.get("bus_type") or "")) != "i2c":
        return
    address_bytes = _derived_i2c_address_bytes(device_ir)
    if not address_bytes:
        return
    for location, transaction, context in _iter_transactions(device_ir):
        byte_values = _transaction_byte_values(transaction)
        if not byte_values:
            continue
        for pos, value in enumerate(byte_values):
            if value not in address_bytes:
                continue
            derived = address_bytes[value]
            level = "error" if _looks_like_i2c_address_byte_context(context) else "warning"
            issues.append(
                ValidationIssue(
                    level,
                    f"device_ir.flow_risk.i2c_address_byte_payload.{location}.bytes[{pos}]",
                    "transaction bytes contain "
                    f"0x{value:02X}, which equals the 8-bit I2C wire/control "
                    f"byte derived from 7-bit address {derived}. For normal "
                    "RTOS I2C APIs, transaction.bytes should contain only "
                    "payload/register/memory-address/command bytes, not the "
                    "slave address byte.",
                )
            )


def _check_config_register_coverage(
    device_ir: Mapping[str, Any],
    flow_audit_receipt: Mapping[str, Any] | None,
    issues: list[ValidationIssue],
) -> None:
    config_registers = _measurement_config_registers(device_ir)
    if not config_registers:
        return

    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list) or not flows:
        return

    flow_text_raw = " ".join(
        _strings_from_fields(
            device_ir,
            (
                "access_model",
                "init_sequence",
                "read_sequence",
                "operation_flows",
            ),
        )
    )
    if isinstance(flow_audit_receipt, Mapping):
        flow_text_raw += " " + " ".join(
            text
            for _path, text in _walk_strings(flow_audit_receipt.get("audit_findings"))
        )
    flow_text = _normalise(flow_text_raw)

    if _has_explicit_default_config_statement(flow_text_raw):
        return

    missing: list[Mapping[str, str]] = []
    for register in config_registers:
        if not _config_register_is_referenced(register, flow_text):
            missing.append(register)

    if not missing:
        return

    labels = [
        f"{item.get('name') or '<unnamed>'}"
        + (f" ({item.get('value')})" if item.get("value") else "")
        for item in missing
    ]
    issues.append(
        ValidationIssue(
            "error",
            "device_ir.flow_risk.config_register_coverage",
            "writable setup/config/control/mode register(s) are listed but "
            "not covered by init/read operation flows and no explicit "
            f"default-configuration note is present: {', '.join(labels)}.",
        )
    )


def _check_non_contiguous_result_byte_reads(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
    source_context: str | None,
) -> None:
    pairs = _non_contiguous_result_byte_pairs(device_ir)
    if not pairs:
        return

    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return
    channels = _read_channels(device_ir)

    for pair in pairs:
        channel_ids = _channels_using_byte_pair(channels, pair)
        if not channel_ids:
            continue
        candidate_flows = [
            flow
            for flow in flows
            if isinstance(flow, Mapping)
            and _flow_matches_channels_or_pair(flow, channel_ids, pair)
        ]
        for flow in candidate_flows:
            if not _flow_is_channel_result_read(flow, channel_ids, pair):
                continue
            flow_id = str(flow.get("flow_id") or "<unnamed>")
            if _flow_explicitly_touches_register(flow, pair["low"]):
                continue
            if not _flow_explicitly_touches_register(flow, pair["high"]):
                continue
            if _source_context_allows_high_address_two_byte_read(source_context, pair):
                continue
            issue_detail = (
                f"{pair['high'].get('name')}@{pair['high'].get('address')} and "
                f"{pair['low'].get('name')}@{pair['low'].get('address')}"
            )
            if _flow_has_multi_byte_read_from_register(flow, pair["high"]):
                reason = (
                    "uses a multi-byte read starting at the high-byte register but "
                    "does not explicitly address the non-contiguous low-byte register"
                )
            else:
                reason = (
                    "touches the high-byte register but does not explicitly address "
                    "the non-contiguous low-byte register"
                )
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.non_contiguous_result_bytes.{flow_id}",
                    (
                        "operation flow for channel(s) "
                        f"{sorted(channel_ids)} {reason}: {issue_detail}. "
                        "For non-contiguous high/low result registers, model separate "
                        "pointer reads or another datasheet-grounded transaction that "
                        "explicitly selects the low-byte register."
                    ),
                )
            )


def _check_completion_condition_coverage(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
    source_context: str | None,
) -> None:
    """Flag executable flows that omit datasheet-stated completion gates."""

    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return

    ir_completion_text = " ".join(
        _strings_from_fields(
            device_ir,
            (
                "access_model",
                "operation_flows",
                "init_sequence",
                "read_sequence",
                "timing_constraints",
                "error_conditions",
            ),
        )
    )
    context = f"{ir_completion_text} {source_context or ''}"
    has_write_completion_fact = _matches_any(context, WRITE_COMPLETION_CONTEXT_PATTERNS)
    has_measurement_completion_fact = _matches_any(
        context,
        MEASUREMENT_COMPLETION_CONTEXT_PATTERNS,
    )
    if not has_write_completion_fact and not has_measurement_completion_fact:
        return

    for flow_index, flow in enumerate(flows):
        if not isinstance(flow, Mapping) or flow.get("requires_human") is True:
            continue
        flow_id = str(flow.get("flow_id") or f"index_{flow_index}")
        flow_text = " ".join(_flow_strings(flow))
        steps = flow.get("steps")
        if not isinstance(steps, list):
            continue

        if (
            has_write_completion_fact
            and _flow_looks_like_nonvolatile_write(flow)
            and _flow_has_write_without_readback(steps)
            and not _flow_has_completion_step_after_write(steps)
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.completion_condition.{flow_id}",
                    "flow appears to perform a write/program/erase operation while "
                    "the datasheet context describes a write/program completion "
                    "condition such as tWR, busy time, ready state, or ACK polling, "
                    "but the flow has no delay/poll_until/wait_until_ready step. "
                    "Model the completion wait/poll generically before treating "
                    "the write as complete.",
                )
            )

        if (
            has_measurement_completion_fact
            and _flow_has_trigger_before_result_read(steps)
            and _matches_any(flow_text, TRIGGER_LIKE_STEP_PATTERNS)
            and not _flow_has_completion_step_between_trigger_and_read(steps)
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.flow_risk.completion_condition.{flow_id}",
                    "flow starts or triggers a conversion/measurement and then "
                    "reads data, while the datasheet context describes conversion "
                    "time, data-ready, busy, or ready signalling. Add a concrete "
                    "delay/poll_until/wait_until_ready/wait_signal step between "
                    "trigger and result read, or mark the flow requires_human if "
                    "the supplied content lacks the exact condition.",
                )
            )


def _check_integrity_condition_coverage(
    device_ir: Mapping[str, Any],
    issues: list[ValidationIssue],
    source_context: str | None,
) -> None:
    context = " ".join(
        _strings_from_fields(
            device_ir,
            (
                "operation_flows",
                "read_sequence",
                "read_channels",
                "raw_encoding",
                "timing_constraints",
                "error_conditions",
                "requires_human",
            ),
        )
    )
    full_context = f"{context} {source_context or ''}"
    if not _matches_any(full_context, INTEGRITY_CONTEXT_PATTERNS):
        return

    error_text = " ".join(
        _strings_from_fields(device_ir, ("error_conditions",))
    )
    if _matches_any(error_text, INTEGRITY_CONTEXT_PATTERNS):
        return
    if _has_requires_human_note_for_integrity(device_ir):
        return
    if not _has_public_read_flow(device_ir):
        return

    issues.append(
        ValidationIssue(
            "warning",
            "device_ir.flow_risk.integrity_condition",
            "datasheet/IR context mentions CRC/checksum/PEC/parity, but "
            "error_conditions does not describe the runtime integrity check. "
            "If the check protects returned data, preserve it as a driver "
            "runtime error condition rather than dropping the byte/bit from IR.",
        )
    )


def _measurement_config_registers(device_ir: Mapping[str, Any]) -> list[Mapping[str, str]]:
    registers = device_ir.get("registers_or_commands")
    if not isinstance(registers, list):
        return []
    out: list[Mapping[str, str]] = []
    for register in registers:
        if not isinstance(register, Mapping):
            continue
        access = str(register.get("access") or "")
        if not _access_allows_write(access):
            continue
        name = str(register.get("name") or "")
        value = str(register.get("value") or "")
        description = str(register.get("description") or "")
        if _looks_like_measurement_config_register(name, description):
            out.append({"name": name, "value": value, "description": description})
    return out


def _access_allows_write(access: str) -> bool:
    normalized = _normalise(access)
    if not normalized or normalized in {"ro", "read_only", "readonly", "read"}:
        return False
    return (
        normalized in {"rw", "wo", "w", "write", "write_only", "read_write"}
        or "write" in normalized
        or normalized.endswith("_w")
        or "_w_" in f"_{normalized}_"
    )


def _looks_like_measurement_config_register(name: str, description: str) -> bool:
    text = _normalise(f"{name} {description}")
    tokens = set(token for token in text.split("_") if token)
    if not tokens:
        return False
    keyword_hits = tokens & CONFIG_REGISTER_KEYWORDS
    if not keyword_hits:
        return False
    exclude_hits = tokens & CONFIG_REGISTER_EXCLUDE_KEYWORDS
    strong_config_hits = keyword_hits - {"gain", "power", "rate"}
    if exclude_hits and not strong_config_hits:
        return False
    return True


def _config_register_is_referenced(register: Mapping[str, str], flow_text: str) -> bool:
    name = _normalise(str(register.get("name") or ""))
    if name and _contains_normalized_token(flow_text, name):
        return True
    for token in _name_reference_tokens(name):
        if _contains_normalized_token(flow_text, token):
            return True
    return False


def _name_reference_tokens(name: str) -> set[str]:
    tokens = set(token for token in name.split("_") if len(token) >= 4)
    stopwords = {
        "cfg",
        "conf",
        "config",
        "control",
        "ctrl",
        "mode",
        "pressure",
        "register",
        "temperature",
    }
    return tokens - stopwords


def _has_explicit_default_config_statement(text: str) -> bool:
    return any(pattern.search(text or "") for pattern in DEFAULT_CONFIG_PATTERNS)


def _flow_looks_like_nonvolatile_write(flow: Mapping[str, Any]) -> bool:
    text = " ".join(_flow_strings(flow))
    kind = _normalise(str(flow.get("kind") or ""))
    if kind in {"write", "init", "calibration"} and _matches_any(
        text,
        WRITE_LIKE_FLOW_PATTERNS,
    ):
        return True
    return _matches_any(text, WRITE_LIKE_FLOW_PATTERNS)


def _flow_has_write_without_readback(steps: list[Any]) -> bool:
    has_write = False
    has_readback = False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        op = str(step.get("op") or "")
        tx = step.get("transaction")
        tx_kind = str(tx.get("kind") or "") if isinstance(tx, Mapping) else ""
        if op in {"write", "set_signal"} or tx_kind == "write":
            has_write = True
        if op in {"read", "write_then_read"} or tx_kind in {"read", "write_then_read"}:
            has_readback = True
    return has_write and not has_readback


def _flow_has_completion_step_after_write(steps: list[Any]) -> bool:
    seen_write = False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        op = str(step.get("op") or "")
        tx = step.get("transaction")
        tx_kind = str(tx.get("kind") or "") if isinstance(tx, Mapping) else ""
        if op == "write" or tx_kind == "write":
            seen_write = True
            continue
        if seen_write and op in COMPLETION_STEP_OPS:
            return True
    return False


def _flow_has_trigger_before_result_read(steps: list[Any]) -> bool:
    seen_trigger = False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        op = str(step.get("op") or "")
        if op in COMPLETION_STEP_OPS:
            continue
        if op == "write" and _matches_any(" ".join(_flow_strings(step)), TRIGGER_LIKE_STEP_PATTERNS):
            seen_trigger = True
            continue
        if op == "set_signal" and _set_signal_step_starts_measurement(step):
            seen_trigger = True
            continue
        if seen_trigger and op in {"read", "write_then_read", "sample_signal", "measure_pulse"}:
            return True
        tx = step.get("transaction")
        tx_kind = str(tx.get("kind") or "") if isinstance(tx, Mapping) else ""
        if seen_trigger and tx_kind in {"read", "write_then_read"}:
            return True
    return False


def _flow_has_completion_step_between_trigger_and_read(steps: list[Any]) -> bool:
    seen_trigger = False
    seen_completion = False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        op = str(step.get("op") or "")
        if op == "write" and _matches_any(" ".join(_flow_strings(step)), TRIGGER_LIKE_STEP_PATTERNS):
            seen_trigger = True
            seen_completion = False
            continue
        if op == "set_signal" and _set_signal_step_starts_measurement(step):
            seen_trigger = True
            seen_completion = False
            continue
        if not seen_trigger:
            continue
        if op in COMPLETION_STEP_OPS:
            seen_completion = True
            continue
        tx = step.get("transaction")
        tx_kind = str(tx.get("kind") or "") if isinstance(tx, Mapping) else ""
        if op in {"read", "write_then_read", "sample_signal", "measure_pulse"} or tx_kind in {"read", "write_then_read"}:
            return seen_completion
    return False


def _set_signal_step_starts_measurement(step: Mapping[str, Any]) -> bool:
    """Return true only when a signal toggle clearly starts acquisition."""
    signal = _normalise(step.get("signal") or step.get("name") or step.get("pin") or "")
    text = " ".join(_flow_strings(step))
    if signal in CHIP_SELECT_SIGNAL_NAMES:
        return _matches_any(text, SET_SIGNAL_TRIGGER_TEXT_PATTERNS)
    if _matches_any(signal, SET_SIGNAL_TRIGGER_SIGNAL_PATTERNS):
        return True
    return _matches_any(text, SET_SIGNAL_TRIGGER_TEXT_PATTERNS)


def _has_requires_human_note_for_integrity(device_ir: Mapping[str, Any]) -> bool:
    notes = device_ir.get("requires_human")
    if not isinstance(notes, list):
        return False
    return any(
        _matches_any(str(note or ""), INTEGRITY_CONTEXT_PATTERNS)
        for note in notes
    )


def _has_public_read_flow(device_ir: Mapping[str, Any]) -> bool:
    flows = device_ir.get("operation_flows")
    if isinstance(flows, list):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            if str(flow.get("kind") or "") == "read" and flow.get("requires_human") is not True:
                return True
    channels = device_ir.get("read_channels")
    return isinstance(channels, list) and any(isinstance(channel, Mapping) for channel in channels)


def _non_contiguous_result_byte_pairs(
    device_ir: Mapping[str, Any],
) -> list[dict[str, Mapping[str, Any]]]:
    registers = device_ir.get("registers_or_commands")
    if not isinstance(registers, list):
        return []

    grouped: dict[str, dict[str, list[Mapping[str, Any]]]] = {}
    for register in registers:
        if not isinstance(register, Mapping):
            continue
        part = _infer_byte_part(register)
        if part not in {"high", "low"}:
            continue
        address = _register_address_int(register)
        if address is None:
            continue
        family = _byte_pair_family_key(register)
        if not family:
            continue
        record = {
            "name": str(register.get("name") or ""),
            "address": _register_address_text(register),
            "address_int": address,
            "raw": register,
        }
        grouped.setdefault(family, {"high": [], "low": []})[part].append(record)

    pairs: list[dict[str, Mapping[str, Any]]] = []
    for family, parts in grouped.items():
        for high in parts["high"]:
            for low in parts["low"]:
                if abs(int(low["address_int"]) - int(high["address_int"])) <= 1:
                    continue
                pairs.append({"family": family, "high": high, "low": low})
    return pairs


def _read_channels(device_ir: Mapping[str, Any]) -> list[Mapping[str, Any]]:
    channels = device_ir.get("read_channels")
    return [channel for channel in channels if isinstance(channel, Mapping)] if isinstance(channels, list) else []


def _channels_using_byte_pair(
    channels: list[Mapping[str, Any]],
    pair: Mapping[str, Any],
) -> set[str]:
    high = pair.get("high")
    low = pair.get("low")
    if not isinstance(high, Mapping) or not isinstance(low, Mapping):
        return set()
    family = str(pair.get("family") or "")
    out: set[str] = set()
    for channel in channels:
        channel_id = str(channel.get("id") or "").strip()
        if not channel_id:
            continue
        text = _normalise(" ".join(str(value or "") for value in _channel_byte_binding_values(channel)))
        if (
            _register_reference_in_text(high, text)
            and _register_reference_in_text(low, text)
        ):
            out.add(channel_id)
            continue
        if (
            family
            and _family_tokens_match(family, text)
            and _contains_byte_part_token(text, "high")
            and _contains_byte_part_token(text, "low")
        ):
            out.add(channel_id)
    return out


def _channel_byte_binding_values(channel: Mapping[str, Any]) -> list[Any]:
    values: list[Any] = []
    source_bytes = channel.get("source_bytes")
    if isinstance(source_bytes, list):
        values.extend(source_bytes)
    else:
        values.append(source_bytes)
    values.append(channel.get("source_signal"))
    for key in ("notes", "raw_type", "formula_id"):
        values.append(channel.get(key))
    return values


def _flow_matches_channels_or_pair(
    flow: Mapping[str, Any],
    channel_ids: set[str],
    pair: Mapping[str, Any],
) -> bool:
    declared_channels = {
        str(channel or "").strip()
        for channel in flow.get("channels", [])
        if str(channel or "").strip()
    } if isinstance(flow.get("channels"), list) else set()
    if declared_channels & channel_ids:
        return True

    outputs = flow.get("outputs")
    if isinstance(outputs, list):
        for output in outputs:
            if isinstance(output, Mapping):
                channel = str(output.get("channel") or "").strip()
                if channel in channel_ids:
                    return True

    flow_text = _normalise(" ".join(_flow_strings(flow)))
    high = pair.get("high")
    low = pair.get("low")
    return (
        isinstance(high, Mapping)
        and isinstance(low, Mapping)
        and _register_reference_in_text(high, flow_text)
        and _register_reference_in_text(low, flow_text)
    )


def _flow_is_channel_result_read(
    flow: Mapping[str, Any],
    channel_ids: set[str],
    pair: Mapping[str, Any],
) -> bool:
    kind = _normalise(str(flow.get("kind") or ""))
    flow_id = _normalise(str(flow.get("flow_id") or ""))
    if "read" in kind or "read" in flow_id:
        return True
    high = pair.get("high")
    if isinstance(high, Mapping) and _flow_has_multi_byte_read_from_register(flow, high):
        return True
    outputs = flow.get("outputs")
    if not isinstance(outputs, list):
        return False
    for output in outputs:
        if not isinstance(output, Mapping):
            continue
        channel = str(output.get("channel") or "").strip()
        if channel not in channel_ids:
            continue
        output_text = _normalise(" ".join(str(value or "") for value in output.values()))
        if _contains_byte_part_token(output_text, "high") and _contains_byte_part_token(output_text, "low"):
            return True
    return False


def _flow_explicitly_touches_register(
    flow: Mapping[str, Any],
    register: Mapping[str, Any],
) -> bool:
    steps = flow.get("steps")
    if not isinstance(steps, list):
        return False
    address = str(register.get("address") or "")
    name = str(register.get("name") or "")
    address_int = register.get("address_int")
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        if _structured_target_matches_register(step, name, address, address_int):
            return True
        transaction = step.get("transaction")
        if isinstance(transaction, Mapping):
            if _structured_target_matches_register(transaction, name, address, address_int):
                return True
            if _transaction_bytes_contain_address(transaction, address_int):
                return True
    return False


def _flow_has_multi_byte_read_from_register(
    flow: Mapping[str, Any],
    register: Mapping[str, Any],
) -> bool:
    steps = flow.get("steps")
    if not isinstance(steps, list):
        return False
    address = str(register.get("address") or "")
    name = str(register.get("name") or "")
    address_int = register.get("address_int")
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        transaction = step.get("transaction")
        if not isinstance(transaction, Mapping):
            continue
        kind = _normalise(str(transaction.get("kind") or step.get("op") or ""))
        if "read" not in kind:
            continue
        length = _coerce_int(transaction.get("length") or step.get("length"))
        if length is None or length <= 1:
            continue
        if (
            _structured_target_matches_register(transaction, name, address, address_int)
            or _structured_target_matches_register(step, name, address, address_int)
            or _transaction_bytes_contain_address(transaction, address_int)
        ):
            return True
    return False


def _structured_target_matches_register(
    payload: Mapping[str, Any],
    name: str,
    address: str,
    address_int: Any,
) -> bool:
    target_values = [
        payload.get("register"),
        payload.get("pointer_target"),
        payload.get("target"),
        payload.get("address"),
        payload.get("command"),
    ]
    target_text = _normalise(" ".join(str(value or "") for value in target_values))
    if name and _contains_normalized_token(target_text, _normalise(name)):
        return True
    if address and _normalise(address) and _contains_normalized_token(target_text, _normalise(address)):
        return True
    for value in target_values:
        if _value_equals_address(value, address_int):
            return True
    return False


def _transaction_bytes_contain_address(
    transaction: Mapping[str, Any],
    address_int: Any,
) -> bool:
    values = transaction.get("bytes")
    if not isinstance(values, list):
        return False
    return any(_value_equals_address(value, address_int) for value in values)


def _source_context_allows_high_address_two_byte_read(
    source_context: str | None,
    pair: Mapping[str, Any],
) -> bool:
    if not source_context:
        return False
    high = pair.get("high")
    if not isinstance(high, Mapping):
        return False
    address_int = high.get("address_int")
    if not isinstance(address_int, int):
        return False
    text = str(source_context or "").lower()
    address_variants = _address_text_variants(address_int)
    for match in re.finditer(r"two[-\s]?byte|2[-\s]?byte|16[-\s]?bit", text):
        window = text[max(0, match.start() - 800): match.end() + 1000]
        if not any(variant in window for variant in address_variants):
            continue
        if "read" not in window:
            continue
        if not (
            "single" in window
            or "command" in window
            or "compatible with two-byte read" in window
        ):
            continue
        if not ("low byte" in window or "followed by the low" in window):
            continue
        return True
    return False


def _address_text_variants(address_int: int) -> set[str]:
    return {
        f"0x{address_int:02x}",
        f"0x{address_int:x}",
        f"{address_int:02x}h",
        f"{address_int:x}h",
        f"address {address_int:02x}h",
        f"address {address_int:x}h",
    }


def _register_reference_in_text(register: Mapping[str, Any], text: str) -> bool:
    name = _normalise(str(register.get("name") or ""))
    if name and _contains_normalized_token(text, name):
        return True
    address = _normalise(str(register.get("address") or ""))
    if address and _contains_normalized_token(text, address):
        return True
    return False


def _family_tokens_match(family: str, text: str) -> bool:
    family_tokens = _semantic_tokens(family)
    text_tokens = _semantic_tokens(text)
    if not family_tokens or not text_tokens:
        return False
    return family_tokens.issubset(text_tokens)


def _contains_byte_part_token(text: str, part: str) -> bool:
    tokens = _semantic_tokens(text)
    if part == "high":
        return bool(tokens & {"high", "msb", "upper"})
    if part == "low":
        return bool(tokens & {"low", "lsb", "lower"})
    return False


def _semantic_tokens(text: str) -> set[str]:
    tokens: set[str] = set()
    for token in re.findall(r"[a-z]+|\d+", str(text or "").lower()):
        token = _canonical_semantic_token(token)
        if token:
            tokens.add(token)
    return tokens


def _canonical_semantic_token(token: str) -> str:
    aliases = {
        "temp": "temperature",
        "temps": "temperature",
        "msbyte": "msb",
        "lsbyte": "lsb",
    }
    return aliases.get(token, token)


def _infer_byte_part(register: Mapping[str, Any]) -> str | None:
    text = _normalise(
        f"{register.get('name') or ''} {register.get('description') or ''}"
    )
    tokens = set(token for token in text.split("_") if token)
    if {"high", "msb", "upper"} & tokens or "high_byte" in text:
        return "high"
    if {"low", "lsb", "lower"} & tokens or "low_byte" in text:
        return "low"
    return None


def _byte_pair_family_key(register: Mapping[str, Any]) -> str:
    name = _normalise(str(register.get("name") or ""))
    description = _normalise(str(register.get("description") or ""))
    for text in (name, description):
        tokens = [
            token
            for token in text.split("_")
            if token
            and token not in {
                "reg",
                "register",
                "byte",
                "bytes",
                "high",
                "low",
                "msb",
                "lsb",
                "upper",
                "lower",
            }
        ]
        if tokens:
            return "_".join(tokens)
    return ""


def _register_address_int(register: Mapping[str, Any]) -> int | None:
    for key in ("value", "address", "command"):
        parsed = _parse_address_int(register.get(key))
        if parsed is not None:
            return parsed
    return _parse_address_int(str(register))


def _register_address_text(register: Mapping[str, Any]) -> str:
    address = _register_address_int(register)
    return f"0x{address:02X}" if address is not None and 0 <= address <= 0xFF else (
        f"0x{address:X}" if address is not None else ""
    )


def _parse_address_int(value: Any) -> int | None:
    if value is None:
        return None
    if isinstance(value, int):
        return value
    text = str(value)
    match = re.search(r"0x[0-9a-fA-F]+|\b[0-9a-fA-F]{2}h\b|\b\d+\b", text)
    if not match:
        return None
    token = match.group(0)
    try:
        if token.lower().startswith("0x"):
            return int(token, 16)
        if token.lower().endswith("h"):
            return int(token[:-1], 16)
        return int(token, 10)
    except ValueError:
        return None


def _value_equals_address(value: Any, address_int: Any) -> bool:
    if not isinstance(address_int, int):
        return False
    parsed = _parse_address_int(value)
    return parsed == address_int


def _coerce_int(value: Any) -> int | None:
    if isinstance(value, int):
        return value
    if value is None:
        return None
    try:
        return int(str(value), 0)
    except ValueError:
        return None


def _strings_from_fields(
    payload: Mapping[str, Any],
    field_names: tuple[str, ...],
) -> list[str]:
    out: list[str] = []
    for field_name in field_names:
        for _path, text in _walk_strings(payload.get(field_name)):
            out.append(text)
    return out


def _formula_map(device_ir: Mapping[str, Any]) -> dict[str, Mapping[str, Any]]:
    formulas = device_ir.get("conversion_formulae")
    out: dict[str, Mapping[str, Any]] = {}
    if not isinstance(formulas, list):
        return out
    for formula in formulas:
        if not isinstance(formula, Mapping):
            continue
        name = str(formula.get("name") or "").strip()
        if name:
            out[name] = formula
    return out


def _channel_source_map(device_ir: Mapping[str, Any]) -> dict[str, str]:
    out: dict[str, str] = {}
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        cid = str(channel.get("id") or "").strip()
        if not cid:
            continue
        for token in _source_tokens(channel.get("source_bytes")):
            out[token] = cid
        for token in _source_tokens(channel.get("source_signal")):
            out[token] = cid
    return out


def _flows_by_channel(device_ir: Mapping[str, Any]) -> dict[str, Mapping[str, Any]]:
    out: dict[str, Mapping[str, Any]] = {}
    priorities: dict[str, int] = {}
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return out
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        priority = _flow_priority(flow)
        channels = flow.get("channels")
        if isinstance(channels, list):
            for channel in channels:
                cid = str(channel or "").strip()
                if cid and priority >= priorities.get(cid, -1):
                    out[cid] = flow
                    priorities[cid] = priority
        outputs = flow.get("outputs")
        if isinstance(outputs, list):
            for output in outputs:
                if isinstance(output, Mapping):
                    cid = str(output.get("channel") or "").strip()
                    if cid and priority >= priorities.get(cid, -1):
                        out[cid] = flow
                        priorities[cid] = priority
    return out


def _flow_priority(flow: Mapping[str, Any]) -> int:
    kind = str(flow.get("kind") or "").strip()
    if kind == "read":
        return 30
    if kind in {"probe", "init"}:
        return 20
    if kind == "calibration":
        return 10
    return 0


def _flow_id(flow: object) -> str:
    if not isinstance(flow, Mapping):
        return ""
    return str(flow.get("flow_id") or "").strip()


def _operation_flow_ids(device_ir: Mapping[str, Any]) -> set[str]:
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return set()
    return {
        flow_id
        for flow_id in (_flow_id(flow) for flow in flows)
        if flow_id
    }


def _reorder_flows_for_dependencies(
    device_ir: dict[str, Any],
    dependencies_by_flow: Mapping[str, set[str]],
) -> bool:
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list) or not dependencies_by_flow:
        return False
    ids = [_flow_id(flow) for flow in flows]
    if any(not fid for fid in ids) or len(set(ids)) != len(ids):
        return False
    by_id = {
        fid: flow
        for fid, flow in zip(ids, flows)
        if isinstance(flow, Mapping)
    }
    ordered: list[str] = []
    visiting: set[str] = set()
    visited: set[str] = set()
    cycle = False

    def visit(fid: str) -> None:
        nonlocal cycle
        if fid in visited:
            return
        if fid in visiting:
            cycle = True
            return
        visiting.add(fid)
        deps = [dep for dep in dependencies_by_flow.get(fid, set()) if dep in by_id]
        for dep in sorted(deps, key=ids.index):
            visit(dep)
        visiting.remove(fid)
        visited.add(fid)
        ordered.append(fid)

    for fid in ids:
        visit(fid)
    if cycle or ordered == ids or set(ordered) != set(ids):
        return False
    device_ir["operation_flows"] = [by_id[fid] for fid in ordered]
    return True


def _formula_channel_dependencies(
    formula: Mapping[str, Any],
    source_to_channel: Mapping[str, str],
    channel_ids: set[str] | None = None,
) -> set[str]:
    dependencies: set[str] = set()
    expr = formula.get("integer_approximation_expression")
    if not isinstance(expr, Mapping):
        return dependencies
    inputs = expr.get("inputs")
    if not isinstance(inputs, list):
        return dependencies
    for item in inputs:
        if not isinstance(item, Mapping):
            continue
        source_text = _normalise(" ".join(
            str(item.get(key) or "")
            for key in ("byte_source", "source_signal", "config_source")
        ))
        for token, channel_id in source_to_channel.items():
            if token and token in source_text:
                dependencies.add(channel_id)
        item_text = _normalise(" ".join(
            str(item.get(key) or "")
            for key in ("name", "byte_source", "source_signal", "config_source", "description")
        ))
        for channel_id in channel_ids or set():
            channel_norm = _normalise(channel_id)
            if channel_norm and _contains_normalized_token(item_text, channel_norm):
                dependencies.add(channel_id)
    return dependencies


def _source_tokens(value: Any) -> set[str]:
    raw: list[str] = []
    if isinstance(value, list):
        raw.extend(str(item or "") for item in value)
    elif isinstance(value, str):
        raw.append(value)
    tokens: set[str] = set()
    for item in raw:
        norm = _normalise(item)
        if norm:
            tokens.add(norm)
    return tokens


def _formula_input_has_source_binding(item: Mapping[str, Any]) -> bool:
    if str(item.get("byte_source") or "").strip():
        return True
    if str(item.get("source_signal") or "").strip():
        return True
    if str(item.get("config_source") or "").strip():
        return True
    return item.get("default_value") is not None


def _derived_i2c_address_bytes(device_ir: Mapping[str, Any]) -> dict[int, str]:
    out: dict[int, str] = {}
    for address in _i2c_7bit_addresses(device_ir):
        if not (0 <= address <= 0x7F):
            continue
        out[(address << 1) & 0xFE] = f"0x{address:02X}"
        out[((address << 1) | 1) & 0xFF] = f"0x{address:02X}"
    return out


def _i2c_7bit_addresses(device_ir: Mapping[str, Any]) -> set[int]:
    address_rule = device_ir.get("address_rule")
    if not isinstance(address_rule, Mapping):
        return set()
    values: list[Any] = []
    raw_addresses = address_rule.get("addresses")
    if isinstance(raw_addresses, list):
        for item in raw_addresses:
            if isinstance(item, Mapping):
                values.extend([
                    item.get("address"),
                    item.get("value"),
                    item.get("default"),
                ])
            else:
                values.append(item)
    values.extend([
        address_rule.get("address"),
        address_rule.get("value"),
        address_rule.get("default"),
    ])
    out: set[int] = set()
    for value in values:
        parsed = _parse_address_int(value)
        if parsed is not None and 0 <= parsed <= 0x7F:
            out.add(parsed)
    return out


def _iter_transactions(device_ir: Mapping[str, Any]):
    flows = device_ir.get("operation_flows")
    if isinstance(flows, list):
        for flow_index, flow in enumerate(flows):
            if not isinstance(flow, Mapping):
                continue
            steps = flow.get("steps")
            if not isinstance(steps, list):
                continue
            for step_index, step in enumerate(steps):
                if not isinstance(step, Mapping):
                    continue
                transaction = step.get("transaction")
                if not isinstance(transaction, Mapping):
                    continue
                location = f"operation_flows[{flow_index}].steps[{step_index}]"
                context = " ".join(_flow_strings(flow) + _flow_strings(step))
                yield location, transaction, context
    for sequence_name in ("init_sequence", "read_sequence"):
        sequence = device_ir.get(sequence_name)
        if not isinstance(sequence, list):
            continue
        for step_index, step in enumerate(sequence):
            if not isinstance(step, Mapping):
                continue
            transaction = step.get("transaction")
            if not isinstance(transaction, Mapping):
                continue
            location = f"{sequence_name}[{step_index}]"
            context = " ".join(_flow_strings(step))
            yield location, transaction, context


def _transaction_byte_values(transaction: Mapping[str, Any]) -> list[int | None]:
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list):
        return []
    out: list[int | None] = []
    for value in bytes_value:
        if value is None:
            out.append(None)
            continue
        parsed = _parse_address_int(value)
        out.append(parsed if parsed is not None and 0 <= parsed <= 0xFF else None)
    return out


def _looks_like_i2c_address_byte_context(text: str) -> bool:
    return _matches_any(text or "", I2C_ADDRESS_BYTE_CONTEXT_PATTERNS)


def _walk_strings(value: Any, path: str = ""):
    if isinstance(value, str):
        yield path or "$", value
    elif isinstance(value, Mapping):
        for key, item in value.items():
            next_path = f"{path}.{key}" if path else str(key)
            yield from _walk_strings(item, next_path)
    elif isinstance(value, list):
        for index, item in enumerate(value):
            next_path = f"{path}[{index}]" if path else f"[{index}]"
            yield from _walk_strings(item, next_path)


def _flow_strings(flow: Mapping[str, Any] | None) -> list[str]:
    if not isinstance(flow, Mapping):
        return []
    return [text for _path, text in _walk_strings(flow)]


def _contains_external_knowledge(text: str) -> bool:
    return _matches_any(text, EXTERNAL_KNOWLEDGE_PATTERNS)


def _matches_any(text: str, patterns: tuple[re.Pattern[str], ...]) -> bool:
    return any(pattern.search(text or "") for pattern in patterns)


def _normalise(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", str(value or "").lower()).strip("_")


def _contains_normalized_token(text: str, token: str) -> bool:
    return f"_{token}_" in f"_{text}_"


__all__ = [
    "CRITICAL_CHECKS",
    "EXTERNAL_KNOWLEDGE_PATTERNS",
    "assess_ir_flow_risk",
    "repair_formula_flow_dependencies",
]
