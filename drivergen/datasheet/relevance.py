"""Relevance assessment for datasheet sections."""
from __future__ import annotations

import json
import logging
from typing import Any, Mapping

logger = logging.getLogger(__name__)

# Categories that are safe to filter when marked low relevance.
SAFE_TO_FILTER_CATEGORIES: frozenset[str] = frozenset({
    "ordering_information",
    "package_mechanical",
})

# All valid categories. Kept as a tuple so downstream code can treat it as an
# immutable ordered list (for prompt text and JSON-schema enums).
SECTION_CATEGORIES: tuple[str, ...] = (
    "register_definition",
    "communication_protocol",
    "electrical_characteristics",
    "functional_description",
    "initialization_procedure",
    "timing_specification",
    "pin_configuration",
    "application_notes",
    "ordering_information",
    "package_mechanical",
    "other",
)

# Device IR fields that can be targeted.
TARGET_FIELDS: tuple[str, ...] = (
    "registers_or_commands",
    "bitfields",
    "address_rule",
    "access_model",
    "operation_flows",
    "init_sequence",
    "read_sequence",
    "timing_constraints",
    "conversion_formulae",
    "error_conditions",
    "power_states",
)

RELEVANCE_MAP_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["relevance_map", "extraction_plan", "extraction_notes"],
    "properties": {
        "relevance_map": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["section_id", "relevance", "category", "target_fields", "reason"],
                "properties": {
                    "section_id": {"type": "string"},
                    "relevance": {"type": "string", "enum": ["high", "medium", "low"]},
                    "category": {"type": "string", "enum": list(SECTION_CATEGORIES)},
                    "target_fields": {
                        "type": "array",
                        "items": {"type": "string", "enum": list(TARGET_FIELDS)},
                    },
                    "reason": {"type": "string"},
                },
            },
        },
        "extraction_plan": {"type": "string"},
        "extraction_notes": {"type": "string"},
    },
}


ROLE = (
    "You triage every section in a hardware-datasheet outline so a "
    "downstream driver-generation pipeline only spends its prompt budget "
    "on content that helps write the driver. You score each section, "
    "classify its content category, and tag which device_ir fields it "
    "can populate."
)


def _output_format_block() -> str:
    """Render the OUTPUT_FORMAT block (lazy so the whitelists stay in sync)."""
    return (
        "Output exactly 1 JSON object — no markdown fences, no prose, no "
        "trailing commas. The schema is:\n"
        "{\n"
        "  \"relevance_map\":   [<one entry per outline section>],\n"
        "  \"extraction_plan\": \"<<= 240 chars; describes the driver-extraction strategy>\",\n"
        "  \"extraction_notes\": \"<<= 240 chars; caveats / open questions>\"\n"
        "}\n"
        "Each `relevance_map` entry has shape:\n"
        "{\n"
        "  \"section_id\":    \"<exact id from outline.sections[].section_id>\",\n"
        "  \"relevance\":     \"high\" | \"medium\" | \"low\",\n"
        f"  \"category\":      one of {json.dumps(list(SECTION_CATEGORIES))},\n"
        f"  \"target_fields\": subset of {json.dumps(list(TARGET_FIELDS))},\n"
        "  \"reason\":        \"<<= 200 chars>\"\n"
        "}\n"
        "All keys are required. UTF-8 only."
    )


HARD_RULES: tuple[str, ...] = (
    'Rule 1 — score every outline section: "relevance_map" must contain '
    'exactly one entry for each item in `outline.sections[]`.',
    'Rule 2 — use exact section identifiers: every "section_id" must match '
    'an `outline.sections[].section_id` value character for character.',
    'Rule 3 — be conservative when uncertain: return "low" only for clearly '
    'off-topic material such as ordering, packaging, marketing, or revision '
    'history. Driver-relevant context should be "medium" or "high".',
    'Rule 4 — protect structured facts: sections with tables are at least '
    '"medium", and register, bit-field, address, command, or timing tables '
    'should be "high".',
    f'Rule 5 — use only allowed enum values: "category" in '
    f'{json.dumps(list(SECTION_CATEGORIES))}, "relevance" in '
    f'["high", "medium", "low"], and "target_fields" values in '
    f'{json.dumps(list(TARGET_FIELDS))}.',
    'Rule 6 — keep each "reason" brief and factual, at most 200 characters.',
)


def build_system_prompt() -> str:
    """Assemble the relevance system prompt from ROLE / OUTPUT_FORMAT / HARD_RULES."""
    return "\n\n".join((ROLE, _output_format_block(), *HARD_RULES))


def build_relevance_prompt(
    document_outline: dict,
    device_id: str,
    bus_type: str,
) -> tuple[str, str]:
    """Build system + user prompt for relevance assessment."""
    bus_label = bus_type.strip() or "unknown"
    system_prompt = build_system_prompt()
    user_prompt = (
        f"Device: {device_id}\n"
        f"Target bus: {bus_label}\n"
        f"Target driver type: {bus_label} polling-mode sensor driver\n\n"
        "Document Outline:\n"
        f"{json.dumps(document_outline, indent=2, ensure_ascii=False)}\n\n"
        "Assess the relevance of each section for driver code generation and "
        "return the result as a single json object."
    )
    return system_prompt, user_prompt


def detect_violations(
    payload: Mapping[str, Any] | None,
    *,
    document_outline: Mapping[str, Any] | None = None,
) -> list[str]:
    """Return human-readable violation messages; empty list = compliant."""
    issues: list[str] = []
    if not isinstance(payload, Mapping):
        return ["Output must be a JSON object"]

    rmap = payload.get("relevance_map")
    if not isinstance(rmap, list):
        issues.append("relevance_map must be a list")
        rmap_list: list[Mapping[str, Any]] = []
    else:
        rmap_list = [r for r in rmap if isinstance(r, Mapping)]
        if len(rmap_list) != len(rmap):
            issues.append(
                "relevance_map entries must be objects"
            )

    outline_ids: set[str] = set()
    if isinstance(document_outline, Mapping):
        for sec in document_outline.get("sections", []) or []:
            if isinstance(sec, Mapping):
                sid = sec.get("section_id")
                if isinstance(sid, str) and sid:
                    outline_ids.add(sid)

    seen_ids: set[str] = set()
    for entry in rmap_list:
        sid = entry.get("section_id")
        if not isinstance(sid, str) or not sid:
            issues.append(
                "entry has empty/missing section_id"
            )
            continue
        if sid in seen_ids:
            issues.append(
                f"duplicate section_id {sid!r}"
            )
        seen_ids.add(sid)
        if outline_ids and sid not in outline_ids:
            issues.append(
                f"section_id not in outline: {sid!r}"
            )

        relevance = entry.get("relevance")
        if relevance not in {"high", "medium", "low"}:
            issues.append(
                "relevance must be one of "
                f"high/medium/low, got {relevance!r}"
            )

        category = entry.get("category")
        if category not in SECTION_CATEGORIES:
            issues.append(
                "category not in whitelist: "
                f"{category!r}"
            )

        target_fields = entry.get("target_fields")
        if not isinstance(target_fields, list):
            issues.append(
                "target_fields must be a list"
            )
        else:
            unknown = [f for f in target_fields if f not in TARGET_FIELDS]
            if unknown:
                issues.append(
                    "target_fields entries not in "
                    f"whitelist: {unknown!r}"
                )

        reason = entry.get("reason")
        if not isinstance(reason, str):
            issues.append("reason is not a string")
        elif len(reason) > 240:
            issues.append(
                f"reason exceeds 240 chars ({len(reason)})"
            )

    if outline_ids:
        missing = outline_ids - seen_ids
        if missing:
            issues.append(
                "outline section(s) not scored: "
                f"{sorted(missing)!r}"
            )

    plan = payload.get("extraction_plan")
    if not isinstance(plan, str):
        issues.append("extraction_plan must be a string")
    elif len(plan) > 320:
        issues.append(
            f"extraction_plan exceeds 320 chars ({len(plan)})"
        )

    notes = payload.get("extraction_notes")
    if not isinstance(notes, str):
        issues.append("extraction_notes must be a string")
    elif len(notes) > 320:
        issues.append(
            f"extraction_notes exceeds 320 chars ({len(notes)})"
        )

    return issues


STAGE_B_MODE_MODEL = "model"
STAGE_B_MODE_FALLBACK_RULES = "fallback_rules"

# Cache fingerprint for the relevance prompt and schema.
RELEVANCE_PROMPT_TEMPLATE_VERSION = "2026-06-30"


_CATEGORY_FIELD_DEFAULTS: dict[str, list[str]] = {
    "register_definition": ["registers_or_commands", "bitfields", "address_rule"],
    "communication_protocol": ["access_model", "operation_flows", "read_sequence"],
    "functional_description": ["operation_flows", "read_sequence", "power_states"],
    "initialization_procedure": ["init_sequence", "timing_constraints"],
    "timing_specification": ["timing_constraints", "read_sequence"],
    "electrical_characteristics": ["error_conditions", "power_states"],
    "pin_configuration": ["address_rule", "access_model"],
    "application_notes": ["operation_flows", "init_sequence"],
}


def _outline_text(section: Mapping[str, Any]) -> str:
    parts = [
        section.get("heading", ""),
        " ".join(str(v) for v in section.get("content_keywords", []) or []),
        " ".join(str(v) for v in section.get("table_headers_preview", []) or []),
    ]
    return " ".join(str(part).lower() for part in parts if part)


def _fallback_entry(section: Mapping[str, Any]) -> dict:
    text = _outline_text(section)
    table_count = int(section.get("table_count", 0) or 0)

    category = "other"
    relevance = "medium"
    if any(token in text for token in ("register", "bit", "address", "command")):
        category = "register_definition"
        relevance = "high"
    elif any(token in text for token in ("i2c", "spi", "uart", "bus", "protocol", "transaction")):
        category = "communication_protocol"
        relevance = "high"
    elif any(token in text for token in ("init", "startup", "configuration", "power-on")):
        category = "initialization_procedure"
        relevance = "high"
    elif any(token in text for token in ("timing", "delay", "conversion", "sample rate", "odr")):
        category = "timing_specification"
        relevance = "high" if table_count else "medium"
    elif any(token in text for token in ("pin", "package pin", "interrupt")):
        category = "pin_configuration"
    elif any(token in text for token in ("electrical", "absolute maximum", "supply", "voltage", "current")):
        category = "electrical_characteristics"
    elif any(token in text for token in ("application", "example", "operation", "measurement")):
        category = "application_notes"
    elif any(token in text for token in ("ordering", "part number", "package", "mechanical", "footprint")):
        category = "ordering_information" if "ordering" in text else "package_mechanical"
        relevance = "low"

    if table_count and relevance == "low":
        relevance = "medium"
    fields = _CATEGORY_FIELD_DEFAULTS.get(category, [])
    return {
        "section_id": section["section_id"],
        "relevance": relevance,
        "category": category,
        "target_fields": fields,
        "reason": "Rule-based relevance fallback.",
    }


def _fallback_rules(outline: dict, *, reason: str) -> dict:
    """Apply deterministic relevance scoring when model scoring is unavailable."""
    logger.warning("Using rule-based relevance fallback (%s).", reason)
    sections = outline.get("sections", [])
    return {
        "stage_b_mode": STAGE_B_MODE_FALLBACK_RULES,
        "fallback_reason": reason,
        "relevance_map": [_fallback_entry(sec) for sec in sections],
        "extraction_plan": "Use rule-ranked sections for extraction.",
        "extraction_notes": "Rule-based relevance fallback was used.",
    }


def assess_relevance(
    document_outline: dict,
    device_id: str,
    llm_provider: Any,
    bus_type: str,
) -> dict:
    """Assess relevance of each section using the configured model provider."""
    system_prompt, user_prompt = build_relevance_prompt(document_outline, device_id, bus_type)

    try:
        relevance_result = llm_provider.generate_json(
            task_name="assess_section_relevance",
            schema=RELEVANCE_MAP_SCHEMA,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata={"device_id": device_id, "bus_type": bus_type},
        )
    except Exception as exc:
        logger.warning("Relevance assessment failed: %s (type: %s)", exc, type(exc).__name__)
        return _fallback_rules(document_outline, reason="provider_error")

    if "relevance_map" not in relevance_result:
        return _fallback_rules(document_outline, reason="response missing relevance_map")

    rmap = relevance_result["relevance_map"]

    high_or_medium = sum(1 for r in rmap if r.get("relevance") in ("high", "medium"))
    if high_or_medium == 0:
        return _fallback_rules(
            document_outline,
            reason="no high/medium sections in response",
        )

    relevance_result.setdefault("stage_b_mode", STAGE_B_MODE_MODEL)
    return relevance_result


def apply_relevance_filter(
    structured_document: dict,
    relevance_result: dict,
) -> dict:
    """Apply relevance filtering and group sections by effective priority."""
    sections_by_id = {s["section_id"]: s for s in structured_document.get("sections", [])}
    rmap = relevance_result.get("relevance_map", [])

    grouped: dict[str, Any] = {
        "high": [],
        "medium": [],
        "low_retained": [],
        "filtered": [],
    }
    field_to_sections: dict[str, list[str]] = {f: [] for f in TARGET_FIELDS}

    for raw_entry in rmap:
        sid = raw_entry.get("section_id", "")
        section = sections_by_id.get(sid)
        if section is None:
            continue

        entry = dict(raw_entry)
        relevance = entry.get("relevance", "medium")
        category = entry.get("category", "other")

        if section.get("table_count", 0) > 0 and relevance == "low":
            logger.info(
                "Section %s contains %d table(s); promoting low -> medium to protect structured data.",
                sid, section.get("table_count", 0),
            )
            relevance = "medium"
            entry["relevance"] = "medium"
            entry["reason"] = (
                f"{entry.get('reason', '').rstrip('.')}. "
                "Auto-promoted because section contains tables."
            ).strip()

        if relevance == "low" and category in SAFE_TO_FILTER_CATEGORIES:
            grouped["filtered"].append((entry, section))
        elif relevance == "low":
            grouped["low_retained"].append((entry, section))
        elif relevance == "high":
            grouped["high"].append((entry, section))
        else:
            grouped["medium"].append((entry, section))

        for field in entry.get("target_fields", []):
            if field in field_to_sections:
                field_to_sections[field].append(sid)

    # Keep unscored sections available without letting them dominate the prompt.
    assessed_ids = {e.get("section_id") for e in rmap}
    unseen_count = 0
    for sid, section in sections_by_id.items():
        if sid not in assessed_ids:
            unseen_count += 1
            logger.info(
                "Section %s was not scored; retaining at low priority.",
                sid,
            )
            entry = {
                "section_id": sid,
                "relevance": "low",
                "category": "other",
                "target_fields": [],
                "reason": "Not scored; retained at low priority.",
                "fallback_reason": "unseen_by_stage_b",
            }
            grouped["low_retained"].append((entry, section))
    if unseen_count:
        total = len(sections_by_id)
        logger.warning(
            "Relevance scoring covered %d/%d section(s); %d unscored section(s) retained at low priority.",
            total - unseen_count, total, unseen_count,
        )

    grouped["field_to_sections"] = field_to_sections
    return grouped


def get_sections_for_task(
    grouped: dict,
    target_categories: list[str],
) -> list[str]:
    """Return section_ids matching ``target_categories`` across retained groups."""
    section_ids: list[str] = []
    for group_key in ("high", "medium", "low_retained"):
        for entry, section in grouped.get(group_key, []):
            if entry.get("category") in target_categories:
                section_ids.append(section["section_id"])
    return section_ids


def grouped_to_section_ids_by_relevance(grouped: dict) -> dict[str, list[str]]:
    """Convenience: flatten ``apply_relevance_filter`` output into section_id lists."""
    result: dict[str, list[str]] = {}
    for key in ("high", "medium", "low_retained"):
        result[key] = [section["section_id"] for _entry, section in grouped.get(key, [])]
    return result


# Minimum retained content before low-priority sections are added back.
_LOW_RETAINED_FALLBACK_CHAR_FLOOR: int = 4000


def _bucket_chars(grouped: dict, bucket: str) -> int:
    total = 0
    for _entry, sec in grouped.get(bucket, ()):
        for el in sec.get("elements", ()):
            total += len(str(el.get("content", "") or ""))
    return total


def select_extraction_section_ids(
    grouped: dict,
    *,
    char_floor: int = _LOW_RETAINED_FALLBACK_CHAR_FLOOR,
) -> tuple[list[str], dict[str, Any]]:
    """Pick which section_ids should be fed into the Stage C extraction prompt."""
    high_plus_medium_chars = _bucket_chars(grouped, "high") + _bucket_chars(grouped, "medium")
    low_retained_chars = _bucket_chars(grouped, "low_retained")

    buckets: list[str] = ["high", "medium"]
    info: dict[str, Any] = {
        "high_plus_medium_chars": high_plus_medium_chars,
        "low_retained_chars": low_retained_chars,
        "char_floor": char_floor,
    }

    if high_plus_medium_chars < char_floor and low_retained_chars > 0:
        buckets.append("low_retained")
        info["low_retained_included"] = True
        info["low_retained_skip_reason"] = ""
        info["low_retained_skipped_chars"] = 0
        logger.warning(
            "High+medium sections contributed %d chars (< %d); "
            "including low-priority sections (%d chars, %d sections).",
            high_plus_medium_chars,
            char_floor,
            low_retained_chars,
            len(grouped.get("low_retained", [])),
        )
    else:
        info["low_retained_included"] = False
        info["low_retained_skip_reason"] = (
            "high+medium content is sufficient"
        )
        info["low_retained_skipped_chars"] = low_retained_chars
        if low_retained_chars:
            logger.info(
                "Skipping %d low-priority section(s) (%d chars); high+medium has %d chars.",
                len(grouped.get("low_retained", [])),
                low_retained_chars,
                high_plus_medium_chars,
            )

    info["buckets_included"] = list(buckets)

    ids: list[str] = []
    for bucket in buckets:
        for _entry, sec in grouped.get(bucket, []):
            ids.append(sec["section_id"])
    return ids, info
