"""In-process Docling PDF parser."""
from __future__ import annotations

import logging
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)

_STOP_WORDS = frozenset({"the", "and", "for", "are", "this", "that", "with", "from", "not", "but"})


def _extract_keywords(text: str, top_n: int = 5) -> list[str]:
    """Return the most frequent alphanumeric tokens of length >= 3."""
    words = re.findall(r"[A-Za-z0-9]{3,}", text.lower())
    filtered = [w for w in words if w not in _STOP_WORDS]
    return [w for w, _ in Counter(filtered).most_common(top_n)]


def _table_to_markdown(table: Any, doc: Any = None) -> str:
    """Render a Docling table element as a Markdown table string."""
    try:
        return table.export_to_markdown(doc=doc) if doc else table.export_to_markdown()
    except Exception as exc:  # pragma: no cover - depends on Docling internals
        logger.debug("export_to_markdown failed (%s); trying DataFrame fallback", exc)
    try:
        grid = table.export_to_dataframe(doc=doc) if doc else table.export_to_dataframe()
        return grid.to_markdown(index=False)
    except Exception as exc:  # pragma: no cover - depends on Docling internals
        logger.warning("Table could not be rendered via export_to_markdown or DataFrame: %s", exc)
        return str(table)


def _is_degenerate_header_tokens(tokens: list[str]) -> bool:
    """Return ``True`` when a list of potential header tokens looks like Docling's positional index fallback (``"0", "1", "2"``) instead of real."""
    if not tokens:
        return True
    stripped = [tok.strip() for tok in tokens]
    non_empty = [tok for tok in stripped if tok]
    if not non_empty:
        return True
    return all(tok.isdigit() for tok in non_empty)


def _get_table_header_preview(table: Any, doc: Any = None) -> str:
    """Return the first row of a table as a compact preview string."""
    cols: list[str] = []
    df = None
    try:
        df = table.export_to_dataframe(doc=doc) if doc else table.export_to_dataframe()
        cols = [str(c) for c in df.columns]
    except Exception:
        df = None

    if cols and not _is_degenerate_header_tokens(cols):
        return " | ".join(cols)

    if df is not None:
        try:
            if len(df) >= 1:
                row0 = [str(v) for v in df.iloc[0].tolist()]
                if not _is_degenerate_header_tokens(row0):
                    return " | ".join(row0)
        except Exception:
            pass

    try:
        md = _table_to_markdown(table, doc=doc)
        lines = [ln for ln in md.strip().split("\n") if ln.strip()]
        for line in lines:
            if "|" not in line:
                continue
            tokens = [seg.strip() for seg in line.strip().strip("|").split("|")]
            if not tokens:
                continue
            if all(set(tok) <= set("-:") for tok in tokens if tok):
                continue
            if _is_degenerate_header_tokens(tokens):
                continue
            return " | ".join(tokens)
    except Exception:
        pass

    return ""


def parse_with_docling(
    pdf_path: str,
    use_vlm: bool = False,
    accelerator_device: str = "cuda",
    accelerator_num_threads: int = 4,
) -> dict:
    """Parse ``pdf_path`` with Docling and return the structured + outline dict."""
    from docling.datamodel.accelerator_options import AcceleratorDevice, AcceleratorOptions
    from docling.datamodel.base_models import InputFormat
    from docling.datamodel.pipeline_options import PdfPipelineOptions
    from docling.document_converter import DocumentConverter, PdfFormatOption

    requested_device = _normalise_accelerator_device(accelerator_device)
    actual_device, accelerator_metadata = _select_accelerator_device(requested_device)
    pipeline_options = PdfPipelineOptions(
        accelerator_options=AcceleratorOptions(
            num_threads=accelerator_num_threads,
            device=actual_device,
        )
    )

    if use_vlm:
        try:
            converter = DocumentConverter(
                allowed_formats=[InputFormat.PDF],
                format_options={
                    InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
                },
            )
        except Exception as exc:
            logger.warning("VLM mode failed (%s); falling back to pipeline mode", exc)
            converter = DocumentConverter(
                allowed_formats=[InputFormat.PDF],
                format_options={
                    InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
                },
            )
    else:
        converter = DocumentConverter(
            allowed_formats=[InputFormat.PDF],
            format_options={
                InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
            },
        )

    result = converter.convert(pdf_path)
    doc = result.document
    output = _build_structured_output(doc, pdf_path)
    output["parse_metadata"] = {
        "requested_accelerator_device": requested_device.value,
        "actual_accelerator_device": actual_device.value
        if isinstance(actual_device, AcceleratorDevice)
        else str(actual_device),
        "accelerator_num_threads": accelerator_num_threads,
        "use_vlm": use_vlm,
        **accelerator_metadata,
    }
    return output


def _normalise_accelerator_device(value: str | None):
    from docling.datamodel.accelerator_options import AcceleratorDevice

    raw = str(value or "cuda").strip().lower()
    mapping = {
        "auto": AcceleratorDevice.AUTO,
        "cuda": AcceleratorDevice.CUDA,
        "gpu": AcceleratorDevice.CUDA,
        "cpu": AcceleratorDevice.CPU,
        "mps": AcceleratorDevice.MPS,
        "xpu": AcceleratorDevice.XPU,
    }
    return mapping.get(raw, AcceleratorDevice.CUDA)


def _select_accelerator_device(requested_device):
    from docling.datamodel.accelerator_options import AcceleratorDevice

    metadata: dict[str, Any] = {
        "torch_cuda_available": None,
        "torch_cuda_device_count": None,
        "torch_cuda_device_name": None,
        "fallback_reason": None,
    }
    if requested_device != AcceleratorDevice.CUDA:
        return requested_device, metadata

    try:
        import torch

        cuda_available = bool(torch.cuda.is_available())
        metadata["torch_cuda_available"] = cuda_available
        metadata["torch_cuda_device_count"] = int(torch.cuda.device_count())
        if cuda_available and torch.cuda.device_count() > 0:
            metadata["torch_cuda_device_name"] = torch.cuda.get_device_name(0)
            return AcceleratorDevice.CUDA, metadata
        metadata["fallback_reason"] = "torch.cuda.is_available() returned False"
    except Exception as exc:  # pragma: no cover - depends on host torch install
        metadata["fallback_reason"] = f"torch CUDA probe failed: {exc}"

    logger.warning(
        "Docling accelerator requested CUDA but CUDA is unavailable; falling back to CPU (%s).",
        metadata["fallback_reason"],
    )
    return AcceleratorDevice.CPU, metadata


def _collect_item_pages(item: Any) -> list[int]:
    """Return all ``page_no`` values carried by an item's ``prov`` list."""
    pages: list[int] = []
    if hasattr(item, "prov") and item.prov:
        for p in item.prov:
            page_no = getattr(p, "page_no", None)
            if isinstance(page_no, int):
                pages.append(page_no)
    return pages


def _build_structured_output(doc: Any, pdf_path: str) -> dict:
    """Build structured_document + document_outline dicts from a DoclingDocument."""
    sections: list[dict[str, Any]] = []
    current_section: dict[str, Any] | None = None
    section_counter = 0
    sections_page_inferred = 0

    for item, _level in doc.iterate_items():
        item_type = type(item).__name__

        if item_type in ("SectionHeaderItem", "SectionHeader"):
            if current_section is not None:
                sections.append(current_section)
            section_counter += 1
            heading_text = item.text if hasattr(item, "text") else str(item)
            level = getattr(item, "level", 1)
            current_section = {
                "section_id": f"s{section_counter}",
                "heading": heading_text.strip(),
                "level": level,
                "elements": [],
                "tables": [],
                "table_headers_preview": [],
                "text_blocks": [],
                "pages": set(),
            }
            for page_no in _collect_item_pages(item):
                current_section["pages"].add(page_no)
            continue

        if current_section is None:
            section_counter += 1
            current_section = {
                "section_id": f"s{section_counter}",
                "heading": "(Untitled Section)",
                "level": 0,
                "elements": [],
                "tables": [],
                "table_headers_preview": [],
                "text_blocks": [],
                "pages": set(),
            }

        for page_no in _collect_item_pages(item):
            current_section["pages"].add(page_no)

        if item_type in ("TableItem", "Table"):
            md_table = _table_to_markdown(item, doc=doc)
            current_section["tables"].append(md_table)
            header = _get_table_header_preview(item, doc=doc)
            if header:
                current_section["table_headers_preview"].append(header)
            current_section["elements"].append({"type": "table", "content": md_table})
        elif item_type in ("TextItem", "Text", "ListItem", "Paragraph"):
            text = item.text if hasattr(item, "text") else str(item)
            if text.strip():
                current_section["text_blocks"].append(text.strip())
                current_section["elements"].append({"type": "text", "content": text.strip()})
        elif item_type in ("FormulaItem", "Formula"):
            formula = item.text if hasattr(item, "text") else str(item)
            current_section["elements"].append({"type": "formula", "content": formula.strip()})

    if current_section is not None:
        sections.append(current_section)

    # Merge adjacent duplicate empty headings while preserving page grounding.
    def _heading_key(text: str) -> str:
        return " ".join(text.strip().lower().split())

    deduped: list[dict[str, Any]] = []
    sections_header_merged = 0
    for sec in sections:
        has_content = bool(sec["elements"])
        if (
            deduped
            and not has_content
            and _heading_key(sec["heading"]) == _heading_key(deduped[-1]["heading"])
        ):
            deduped[-1]["pages"].update(sec["pages"])
            sections_header_merged += 1
            continue
        deduped.append(sec)
    sections = deduped

    if sections_header_merged:
        logger.info(
            "Merged %d repeating page-header section(s) into their preceding "
            "same-heading section (P3-8).",
            sections_header_merged,
        )

    # Pass 2: inherit ``last_seen_page`` into any section that still has no page
    # provenance so evidence grounding later can anchor them to a real page.
    running_page = None
    for sec in sections:
        if sec["pages"]:
            running_page = max(sec["pages"])
            continue
        if running_page is not None:
            sec["pages"] = {running_page}
            sections_page_inferred += 1

    if sections_page_inferred:
        logger.info(
            "Inherited page number for %d section(s) lacking Docling prov page_no; "
            "they will still be anchored during evidence validation.",
            sections_page_inferred,
        )

    structured_sections: list[dict[str, Any]] = []
    for sec in sections:
        pages = sorted(sec["pages"]) if sec["pages"] else []
        page_range = [min(pages), max(pages)] if pages else []
        all_text = " ".join(sec["text_blocks"])

        structured_sections.append({
            "section_id": sec["section_id"],
            "heading": sec["heading"],
            "level": sec["level"],
            "page_range": page_range,
            "table_count": len(sec["tables"]),
            "table_headers_preview": sec["table_headers_preview"],
            "text_length_chars": len(all_text),
            "content_keywords": _extract_keywords(all_text),
            "elements": sec["elements"],
        })

    outline_sections: list[dict[str, Any]] = []
    for sec in structured_sections:
        outline_sections.append({
            "section_id": sec["section_id"],
            "heading": sec["heading"],
            "level": sec["level"],
            "page_range": sec["page_range"],
            "has_tables": sec["table_count"] > 0,
            "table_count": sec["table_count"],
            "text_length_chars": sec["text_length_chars"],
            "content_keywords": sec["content_keywords"],
            "table_headers_preview": sec["table_headers_preview"],
        })

    total_tables = sum(s["table_count"] for s in structured_sections)

    total_pages = getattr(doc, "num_pages", None)
    if callable(total_pages):
        total_pages = total_pages()
    if total_pages is not None:
        total_pages = int(total_pages)

    return {
        "structured_document": {
            "source_pdf": str(pdf_path),
            "total_sections": len(structured_sections),
            "total_tables": total_tables,
            "sections": structured_sections,
        },
        "document_outline": {
            "source_pdf": str(pdf_path),
            "total_pages": total_pages,
            "total_sections": len(outline_sections),
            "total_tables": total_tables,
            "sections": outline_sections,
        },
    }
