"""pipeline step - deterministic deep C parser."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path

from .c_header_parser import (
    FunctionDecl,
    MacroDef,
    StructDef,
    TypedefDef,
    EnumDef,
    ParsedFile,
    deep_parse_file,
    deep_parse_files,
    PARSER_VERSION,
)
from .file_selector import FileSelection
from .types import FileCard, RepoIndexBundle

logger = logging.getLogger(__name__)


# Re-export parser types for downstream modules.
# ``from drivergen.rtos.deep_parser import ParsedFile`` etc.
__all__ = [
    "FunctionDecl",
    "MacroDef",
    "StructDef",
    "TypedefDef",
    "EnumDef",
    "ParsedFile",
    "PARSER_VERSION",
    "ParsedFileBundle",
    "parse_selected_files",
]


# Parse result type


@dataclass
class ParsedFileBundle:
    """One parsed file plus the provenance cares about."""

    parsed: ParsedFile
    card: FileCard
    slot_ids: list[str] = field(default_factory=list)

    @property
    def file_key(self) -> str:
        """Stable string id used as the dict key in
        :func:`parse_selected_files` and onwards."""
        return f"{self.card.root_id}::{self.card.path}"

    @property
    def n_functions(self) -> int:
        return len(self.parsed.function_declarations)

    @property
    def n_macros(self) -> int:
        return len(self.parsed.macro_definitions)

    @property
    def n_typedefs(self) -> int:
        return len(self.parsed.typedef_definitions)

    @property
    def n_structs(self) -> int:
        return len(self.parsed.struct_definitions)

    @property
    def n_enums(self) -> int:
        return len(self.parsed.enum_definitions)


# Public entry


def parse_selected_files(
    *,
    bundle: RepoIndexBundle,
    selections: dict[str, FileSelection],
) -> dict[str, ParsedFileBundle]:
    """Deep-parse every distinct file across all per-slot selections."""
    # 1. Collect (root_id, rel_path) → [slot_ids] across all selections.
    file_to_slots: dict[tuple[str, str], list[str]] = {}
    file_to_card: dict[tuple[str, str], FileCard] = {}
    for slot_id, sel in selections.items():
        for m in sel.matches:
            key = (m.card.root_id, m.card.path)
            file_to_slots.setdefault(key, []).append(slot_id)
            file_to_card.setdefault(key, m.card)

    if not file_to_slots:
        logger.info("Deep parser: 0 files to parse (all selections empty)")
        return {}

    # 2. Resolve absolute paths via bundle.roots.
    root_id_to_path: dict[str, Path] = {r.root_id: r.path for r in bundle.roots}

    paths_to_parse: list[tuple[Path, str]] = []
    keys_in_order: list[tuple[str, str]] = []
    skipped_unknown_root = 0
    skipped_missing_file = 0
    for key in file_to_card.keys():
        root_id, rel = key
        root_path = root_id_to_path.get(root_id)
        if root_path is None:
            logger.warning(
                "Deep parser: unknown root_id %r for file %r; skipping",
                root_id,
                rel,
            )
            skipped_unknown_root += 1
            continue
        # FileCard.abs_path is already the resolved location at index
        # time; prefer it when available, fall back to root_path/rel.
        card = file_to_card[key]
        if card.abs_path:
            abs_path = Path(card.abs_path)
        else:
            abs_path = root_path / rel
        if not abs_path.exists():
            logger.warning(
                "Deep parser: file %r not found on disk at %s; skipping",
                rel,
                abs_path,
            )
            skipped_missing_file += 1
            continue
        paths_to_parse.append((abs_path, rel))
        keys_in_order.append(key)

    # 3. Hand off to the shared parser (tree-sitter when available, regex
    #    fallback otherwise — both produce ParsedFile).
    parsed_list = deep_parse_files(paths_to_parse)

    # 4. Pair parsed results back with their slot provenance.
    out: dict[str, ParsedFileBundle] = {}
    for key, parsed in zip(keys_in_order, parsed_list):
        root_id, rel = key
        bundle_obj = ParsedFileBundle(
            parsed=parsed,
            card=file_to_card[key],
            slot_ids=sorted(set(file_to_slots[key])),
        )
        out[bundle_obj.file_key] = bundle_obj

    n_funcs = sum(b.n_functions for b in out.values())
    n_macros = sum(b.n_macros for b in out.values())
    n_typedefs = sum(b.n_typedefs for b in out.values())
    logger.info(
        "Deep parser: parsed %d files (skipped %d unknown root, %d missing); "
        "extracted %d funcs / %d macros / %d typedefs",
        len(out),
        skipped_unknown_root,
        skipped_missing_file,
        n_funcs,
        n_macros,
        n_typedefs,
    )

    return out
