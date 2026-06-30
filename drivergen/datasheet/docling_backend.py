"""Docling backend: parse datasheets locally with Docling."""
from __future__ import annotations

import hashlib
import json
import logging
import os
import re
import tempfile
from contextlib import contextmanager
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Iterator

from ._docling_parser import parse_with_docling

logger = logging.getLogger(__name__)


@dataclass
class DoclingConfig:
    """Configuration for local Docling parsing."""

    use_vlm: bool = False
    hf_offline: bool = True  # isolate HF network usage to this call
    accelerator_device: str = field(
        default_factory=lambda: os.environ.get(
            "DRIVERGEN_DOCLING_DEVICE",
            os.environ.get("DOCLING_ACCELERATOR_DEVICE", "cuda"),
        )
    )
    accelerator_num_threads: int = 4
    # Default structured-document budget; callers can override for smaller models.
    stage_c_max_chars: int = 120000

    def fingerprint(self) -> str:
        """Return a short stable hash of the Docling-relevant fields."""
        payload = {
            k: v
            for k, v in asdict(self).items()
            if k not in {"stage_c_max_chars", "accelerator_device", "accelerator_num_threads"}
        }
        serialized = json.dumps(payload, sort_keys=True, ensure_ascii=False)
        return hashlib.sha256(serialized.encode("utf-8")).hexdigest()[:16]


@contextmanager
def _scoped_env(**overrides: str | None) -> Iterator[None]:
    """Temporarily set environment variables and restore the prior values."""
    previous: dict[str, str | None] = {}
    try:
        for key, value in overrides.items():
            previous[key] = os.environ.get(key)
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        yield
    finally:
        for key, old in previous.items():
            if old is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = old


def parse_pdf(
    pdf_path: Path,
    config: DoclingConfig | None = None,
    output_dir: Path | None = None,
) -> dict:
    """Parse a PDF locally with Docling and return structured document + outline."""
    if config is None:
        config = DoclingConfig()

    hf_offline_value = "1" if config.hf_offline else None

    logger.info(
        "Parsing %s with Docling (vlm=%s, hf_offline=%s, accelerator=%s)...",
        pdf_path,
        config.use_vlm,
        config.hf_offline,
        config.accelerator_device,
    )
    with _scoped_env(HF_HUB_OFFLINE=hf_offline_value):
        result = parse_with_docling(
            str(pdf_path),
            use_vlm=config.use_vlm,
            accelerator_device=config.accelerator_device,
            accelerator_num_threads=config.accelerator_num_threads,
        )

    structured_doc = result["structured_document"]
    outline = result["document_outline"]
    parse_metadata = result.get("parse_metadata", {})

    if output_dir is None:
        output_dir = Path(tempfile.mkdtemp(prefix="docling_"))
    output_dir.mkdir(parents=True, exist_ok=True)

    doc_path = output_dir / "structured_document.json"
    outline_path = output_dir / "document_outline.json"
    metadata_path = output_dir / "docling_parse_metadata.json"

    with open(doc_path, "w", encoding="utf-8") as f:
        json.dump(structured_doc, f, indent=2, ensure_ascii=False)
    with open(outline_path, "w", encoding="utf-8") as f:
        json.dump(outline, f, indent=2, ensure_ascii=False)
    with open(metadata_path, "w", encoding="utf-8") as f:
        json.dump(parse_metadata, f, indent=2, ensure_ascii=False)

    total_sections = structured_doc.get("total_sections", 0)
    total_tables = structured_doc.get("total_tables", 0)
    logger.info("Docling output saved to %s (%d sections, %d tables)",
                output_dir, total_sections, total_tables)

    if total_sections == 0:
        logger.warning(
            "Docling returned 0 sections for %s — the document may be scanned, "
            "image-only, or not parseable.", pdf_path,
        )
    elif total_tables == 0:
        logger.warning(
            "Docling returned 0 tables for %s — register/timing tables will be "
            "missing from structured extraction.", pdf_path,
        )

    return {
        "structured_document": structured_doc,
        "document_outline": outline,
        "parse_metadata": parse_metadata,
        "local_output_dir": str(output_dir),
    }


def _relevance_bucket(section_id: str, section_ids_by_relevance: dict[str, list[str]]) -> int:
    """Return priority 0 (high) / 1 (medium) / 2 (low_retained) / 3 (unknown)."""
    for priority, key in enumerate(("high", "medium", "low_retained")):
        if section_id in section_ids_by_relevance.get(key, ()):
            return priority
    return 3


# P3-2: within a relevance bucket, prefer sections that are likely to carry
# real driver-actionable facts (tables, register tokens, meaningful prose
# length) over page headers / marketing copy.
_REGISTER_LIKE_PATTERN = re.compile(
    r"\b0x[0-9A-Fa-f]{1,4}\b|\b(?:REG|CMD|CTRL|CFG|CONFIG|STATUS|INT|DATA)_[A-Z0-9_]+",
)


def _section_quality_score(section: dict) -> int:
    """Return an in-bucket priority score (higher = more informative)."""
    score = 2 * int(section.get("table_count", 0) or 0)

    heading = str(section.get("heading", "") or "")
    if _REGISTER_LIKE_PATTERN.search(heading):
        score += 1
    else:
        text_peek = ""
        for el in section.get("elements", []):
            if isinstance(el, dict) and el.get("type") == "text":
                text_peek += str(el.get("content", "") or "") + " "
                if len(text_peek) >= 2000:
                    break
        if _REGISTER_LIKE_PATTERN.search(text_peek):
            score += 1

    if int(section.get("text_length_chars", 0) or 0) >= 200:
        score += 1

    return score


def format_sections_for_prompt(
    structured_document: dict,
    section_ids: list[str] | None = None,
    max_chars: int = 60000,
    section_ids_by_relevance: dict[str, list[str]] | None = None,
    drop_sections_without_page: bool = False,
    return_stats: bool = False,
):
    """Format selected sections into a readable prompt string."""
    sections = structured_document.get("sections", [])
    if section_ids is not None:
        id_set = set(section_ids)
        sections = [s for s in sections if s["section_id"] in id_set]

    if section_ids_by_relevance:
        sections = sorted(
            sections,
            # Composite key: (relevance bucket asc, quality score desc, original order asc).
            # Within a bucket we prefer sections with more tables / register tokens /
            # meaningful prose — see ``_section_quality_score`` for the rubric (P3-2).
            key=lambda s: (
                _relevance_bucket(s["section_id"], section_ids_by_relevance),
                -_section_quality_score(s),
            ),
        )

    total_sections = len(sections)
    any_has_page = any(
        isinstance(s.get("page_range"), list) and len(s.get("page_range")) == 2
        for s in sections
    )

    chunks: list[str] = []
    total = 0
    dropped_over_budget = 0
    skipped_no_page = 0
    for sec in sections:
        page_range = sec.get("page_range", [])
        has_valid_page = isinstance(page_range, list) and len(page_range) == 2

        if drop_sections_without_page and any_has_page and not has_valid_page:
            skipped_no_page += 1
            continue

        heading = sec.get("heading", "(Untitled)")
        page_info = f"pages {page_range[0]}-{page_range[1]}" if has_valid_page else ""

        block = f"\n## {heading} [{page_info}]\n\n"
        for elem in sec.get("elements", []):
            if elem["type"] == "table":
                block += f"{elem['content']}\n\n"
            elif elem["type"] == "formula":
                block += f"$$\n{elem['content']}\n$$\n\n"
            else:
                block += f"{elem['content']}\n\n"

        if total + len(block) > max_chars and chunks:
            dropped_over_budget += 1
            continue
        chunks.append(block)
        total += len(block)

    if dropped_over_budget:
        logger.warning(
            "format_sections_for_prompt dropped %d section(s) to stay within "
            "max_chars=%d (kept %d).", dropped_over_budget, max_chars, len(chunks),
        )
    if skipped_no_page:
        logger.warning(
            "format_sections_for_prompt skipped %d section(s) that lacked "
            "page_range — these would not ground during evidence validation.",
            skipped_no_page,
        )

    content = "".join(chunks)
    if return_stats:
        stats = {
            "total_sections": total_sections,
            "sections_kept": len(chunks),
            "sections_dropped_over_budget": dropped_over_budget,
            "sections_skipped_no_page": skipped_no_page,
            "max_chars": max_chars,
            "total_chars": total,
        }
        return content, stats
    return content
