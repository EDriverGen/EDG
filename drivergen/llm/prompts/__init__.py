"""Prompt module facade."""
from __future__ import annotations

from . import ir_extraction, ir_repair, synthesis  # noqa: F401  (re-export)

# ---------------------------------------------------------------------------
# Re-exports; everything below is a thin alias of the canonical implementation
# in ``ir_extraction``.
# ---------------------------------------------------------------------------

EXTRACTION_HINT_MAX_CHARS = ir_extraction.EXTRACTION_HINT_MAX_CHARS
_sanitize_extraction_hint = ir_extraction._sanitize_extraction_hint


# Derived prompt-guideline block retained for compatibility with callers that
# import the aggregate extraction rules.
IR_EXTRACTION_GUIDELINES = (
    ir_extraction.OUTPUT_FORMAT
    + "\n\n"
    + "\n\n".join(ir_extraction.HARD_RULES)
)


def build_ir_prompt(
    device_id: str,
    pages_text: str,
    target_bus_type: str = "",
) -> tuple[str, str]:
    """Thin shim around :func:`ir_extraction.build_ir_prompt`."""
    return ir_extraction.build_ir_prompt(
        device_id=device_id,
        structured_content=pages_text,
        target_bus_type=target_bus_type,
    )


def build_structured_ir_prompt(
    device_id: str,
    structured_content: str,
    extraction_plan: str = "",
    extraction_notes: str = "",
    candidate_fact_summary: str = "",
    target_bus_type: str = "",
) -> tuple[str, str]:
    """Thin shim around :func:`ir_extraction.build_ir_prompt`."""
    return ir_extraction.build_ir_prompt(
        device_id=device_id,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        candidate_fact_summary=candidate_fact_summary,
        target_bus_type=target_bus_type,
    )


def build_device_ir_fact_candidate_prompt(
    device_id: str,
    structured_content: str,
    extraction_plan: str = "",
    extraction_notes: str = "",
) -> tuple[str, str]:
    """Broad candidate-fact prompt."""
    return ir_extraction.build_fact_candidate_prompt(
        device_id=device_id,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
    )


def build_device_ir_schema_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: dict,
    validation_issues: list[dict],
) -> tuple[str, str]:
    """Schema-repair prompt."""
    return ir_repair.build_schema_repair_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=current_device_ir,
        validation_issues=validation_issues,
    )


def build_device_ir_evidence_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: dict,
    validation_issues: list[dict],
) -> tuple[str, str]:
    """Evidence-repair prompt."""
    return ir_repair.build_evidence_repair_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=current_device_ir,
        validation_issues=validation_issues,
    )


def build_device_ir_flow_audit_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: dict,
    *,
    extraction_plan: str = "",
    extraction_notes: str = "",
    allowed_edits: list[str] | tuple[str, ...] | None = None,
) -> tuple[str, str]:
    """Flow-audit prompt for repairing operation lifecycle coverage."""
    return ir_repair.build_flow_audit_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=current_device_ir,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        allowed_edits=allowed_edits,
    )


def build_device_ir_flow_risk_repair_prompt(
    device_id: str,
    structured_content: str,
    current_device_ir: dict,
    *,
    flow_audit_receipt: dict | None,
    flow_risk_issues: list[dict],
    extraction_plan: str = "",
    extraction_notes: str = "",
    allowed_edits: list[str] | tuple[str, ...] | None = None,
) -> tuple[str, str]:
    """Risk-directed flow repair prompt."""
    return ir_repair.build_flow_risk_repair_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=current_device_ir,
        flow_audit_receipt=flow_audit_receipt,
        flow_risk_issues=flow_risk_issues,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        allowed_edits=allowed_edits,
    )


__all__ = [
    "EXTRACTION_HINT_MAX_CHARS",
    "_sanitize_extraction_hint",
    "IR_EXTRACTION_GUIDELINES",
    "build_ir_prompt",
    "build_structured_ir_prompt",
    "build_device_ir_fact_candidate_prompt",
    "build_device_ir_schema_repair_prompt",
    "build_device_ir_evidence_repair_prompt",
    "build_device_ir_flow_audit_prompt",
    "build_device_ir_flow_risk_repair_prompt",
    "ir_extraction",
    "ir_repair",
    "synthesis",
]
