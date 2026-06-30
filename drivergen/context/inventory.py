"""Context inventory: board context loading and source lookup assembly.

Builds a unified evidence lookup table from datasheet pages and RTOS context
sources.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Mapping


def load_board_context(path: Path) -> dict:
    """Load a board context JSON file used to constrain code generation."""
    return json.loads(path.read_text(encoding="utf-8"))


def build_context_source_lookup(context_sources: Mapping[str, Path]) -> dict:
    """Load full source text for later evidence validation."""
    lookup: dict[str, object] = {}
    for source_id, path in context_sources.items():
        lookup[source_id] = path.read_text(encoding="utf-8", errors="replace")
    return lookup


def _datasheet_lines(text: str) -> list[str]:
    return [line.strip() for line in str(text or "").splitlines() if line.strip()]


def _datasheet_blocks(text: str) -> list[str]:
    raw_text = str(text or "").replace("\r\n", "\n").replace("\r", "\n")
    blocks = [block.strip() for block in re.split(r"\n\s*\n", raw_text) if block.strip()]
    if blocks and not (len(blocks) == 1 and "\n" in raw_text):
        return blocks
    compact_lines = _datasheet_lines(raw_text)
    if not compact_lines:
        return []
    # Fall back to small line windows when the PDF extractor does not preserve
    # blank-line structure. This is especially helpful for tables and pin maps.
    windows: list[str] = list(compact_lines)
    for size in (2, 3, 4):
        for index in range(0, len(compact_lines) - size + 1):
            windows.append(" ".join(compact_lines[index : index + size]))
    return windows[:120]


def _empty_datasheet_page_record(page_number: int, text: str = "") -> dict:
    return {
        "page_number": int(page_number),
        "text": text,
        "lines": _datasheet_lines(text),
        "blocks": _datasheet_blocks(text),
        "section_headings": [],
        "section_summaries": [],
        "table_texts": [],
        "element_texts": [],
    }


def _build_datasheet_page_record(page: Mapping[str, object]) -> dict:
    return _empty_datasheet_page_record(
        int(page.get("page_number", 0) or 0),
        str(page.get("text", "") or ""),
    )


def _merge_structured_document(page_records: dict[int, dict], structured_document: Mapping[str, object] | None) -> None:
    if not structured_document:
        return

    sections = structured_document.get("sections", [])
    if not isinstance(sections, list):
        return

    for section in sections:
        if not isinstance(section, Mapping):
            continue
        page_range = section.get("page_range", [])
        if (
            not isinstance(page_range, list)
            or len(page_range) != 2
            or not all(isinstance(value, int) for value in page_range)
        ):
            continue

        heading = str(section.get("heading", "") or "").strip()
        content_keywords = [
            str(keyword)
            for keyword in section.get("content_keywords", [])
            if str(keyword).strip()
        ]
        summary_parts = [heading] if heading else []
        if content_keywords:
            summary_parts.append("keywords: " + ", ".join(content_keywords[:8]))
        section_summary = " | ".join(summary_parts).strip()

        element_texts: list[str] = []
        table_texts: list[str] = []
        for element in section.get("elements", []):
            if not isinstance(element, Mapping):
                continue
            element_type = str(element.get("type", "") or "").strip().lower()
            content = str(element.get("content", "") or "").strip()
            if not content:
                continue
            if element_type == "table":
                table_texts.append(content)
            else:
                element_texts.append(content)

        for page_number in range(page_range[0], page_range[1] + 1):
            page_record = page_records.setdefault(
                page_number,
                _empty_datasheet_page_record(page_number),
            )
            if heading and heading not in page_record["section_headings"]:
                page_record["section_headings"].append(heading)
            if section_summary and section_summary not in page_record["section_summaries"]:
                page_record["section_summaries"].append(section_summary)
            for text in element_texts:
                if text not in page_record["element_texts"]:
                    page_record["element_texts"].append(text)
            for table_text in table_texts:
                if table_text not in page_record["table_texts"]:
                    page_record["table_texts"].append(table_text)


def build_source_lookup(
    device_id: str,
    pages: list[dict],
    context_sources: Mapping[str, Path],
    structured_document: Mapping[str, object] | None = None,
    total_pages: int | None = None,
) -> dict:
    """Merge datasheet pages and RTOS context into one evidence lookup table."""
    page_records = {
        record["page_number"]: record
        for record in (_build_datasheet_page_record(page) for page in pages)
        if record["page_number"] > 0
    }
    _merge_structured_document(page_records, structured_document)
    resolved_total: int | None
    if isinstance(total_pages, int) and not isinstance(total_pages, bool) and total_pages > 0:
        resolved_total = int(total_pages)
    elif page_records:
        resolved_total = max(page_records.keys())
    else:
        resolved_total = None
    lookup: dict[str, object] = {
        device_id: {
            "kind": "datasheet",
            "pages": page_records,
            "total_pages": resolved_total,
        }
    }
    lookup.update(build_context_source_lookup(context_sources))
    return lookup
