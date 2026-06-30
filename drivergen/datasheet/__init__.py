"""Datasheet parsing, structured extraction, and relevance helpers."""

from .docling_backend import (
    DoclingConfig,
    format_sections_for_prompt,
    parse_pdf,
)
from .relevance import (
    SAFE_TO_FILTER_CATEGORIES,
    SECTION_CATEGORIES,
    TARGET_FIELDS,
    apply_relevance_filter,
    assess_relevance,
    build_relevance_prompt,
    get_sections_for_task,
    grouped_to_section_ids_by_relevance,
    select_extraction_section_ids,
)

__all__ = [
    # Docling structured backend
    "DoclingConfig",
    "format_sections_for_prompt",
    "parse_pdf",
    # relevance / routing
    "SAFE_TO_FILTER_CATEGORIES",
    "SECTION_CATEGORIES",
    "TARGET_FIELDS",
    "apply_relevance_filter",
    "assess_relevance",
    "build_relevance_prompt",
    "get_sections_for_task",
    "grouped_to_section_ids_by_relevance",
    "select_extraction_section_ids",
]
