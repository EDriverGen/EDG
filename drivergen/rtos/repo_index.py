"""pipeline step - multi-root repo index builder."""

from __future__ import annotations

import hashlib
import json
import logging
import os
import re
import subprocess
from pathlib import Path

from .config import load_thresholds
from .scope_map import (
    ScopeMapEntry,
    ScopeRule,
    assign_dir_role,
    path_in_scope,
)
from .types import (
    DirectoryCard,
    FileCard,
    IncludeEdge,
    RepoIndexBundle,
    SourceRoot,
    SymbolSketch,
)

logger = logging.getLogger(__name__)


# File extensions and kinds


_HEADER_SUFFIXES = {".h", ".hpp", ".hxx", ".hh"}
_SOURCE_SUFFIXES = {".c", ".cpp", ".cxx", ".cc"}
_CODE_SUFFIXES = _HEADER_SUFFIXES | _SOURCE_SUFFIXES | {".s", ".S"}
_DOC_SUFFIXES = {".md", ".txt", ".rst"}
_BUILD_NAMES = {
    "kconfig",
    "cmakelists.txt",
    "makefile",
    "sconscript",
    "sconstruct",
    "meson.build",
    "build.gn",
}
_BUILD_SUFFIXES = {".cmake", ".mk", ".gn", ".gni"}
_CONFIG_SUFFIXES = {".json", ".yaml", ".yml"}
_ALLOWED_SUFFIXES = (
    _CODE_SUFFIXES | _DOC_SUFFIXES | _BUILD_SUFFIXES | _CONFIG_SUFFIXES
)

# Directories ignored before other scanning filters.
_HARD_SKIP_DIRS = frozenset(
    {
        ".git",
        "__pycache__",
        "node_modules",
        ".svn",
        "build",
        "Build",
        "output",
        "out",
        ".cache",
        ".vscode",
        ".idea",
    }
)


_EXAMPLE_PATH_HINTS = (
    "/example",
    "/examples/",
    "/sample",
    "/samples/",
    "/demo",
    "/demos/",
)


def _classify_file_kind(rel_path: str) -> str:
    p = rel_path.lower()
    suffix = "." + p.rsplit(".", 1)[-1] if "." in p else ""
    name = p.rsplit("/", 1)[-1]

    if suffix in _HEADER_SUFFIXES:
        if any(hint in p for hint in _EXAMPLE_PATH_HINTS):
            return "example"
        return "header"
    if suffix in _SOURCE_SUFFIXES:
        if any(hint in p for hint in _EXAMPLE_PATH_HINTS):
            return "example"
        return "source"
    if name in _BUILD_NAMES or suffix in _BUILD_SUFFIXES:
        return "build"
    if suffix in _DOC_SUFFIXES:
        return "doc"
    if suffix in _CONFIG_SUFFIXES:
        return "config"
    return "config"


# Regex passes


_RE_FUNC_DECL = re.compile(
    r"^\s*"
    r"(?:(?:static|extern|inline|__attribute__\s*\([^)]*\))\s+)*"
    r"(?:(?:const|volatile|unsigned|signed|long|short|struct|enum|union)\s+)*"
    r"([A-Za-z_]\w*(?:\s*\*)*)\s+"
    r"([A-Za-z_]\w*)\s*\(",
    re.MULTILINE,
)
_RE_DEFINE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)", re.MULTILINE)
_RE_STRUCT = re.compile(r"\bstruct\s+([A-Za-z_]\w*)\s*\{", re.MULTILINE)
_RE_TYPEDEF = re.compile(r"\btypedef\s+.*?\b([A-Za-z_]\w*)\s*;", re.MULTILINE)
_RE_ENUM = re.compile(r"\benum\s+([A-Za-z_]\w*)\s*\{", re.MULTILINE)
_RE_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
_RE_DOXYGEN_BRIEF = re.compile(r"[@\\](?:brief|file)\s+(.+)", re.IGNORECASE)

_NOISE_SYMBOLS = frozenset(
    {
        "if",
        "for",
        "while",
        "do",
        "return",
        "sizeof",
        "NULL",
        "main",
        "int",
        "void",
        "char",
        "float",
        "double",
        "long",
        "short",
        "const",
        "static",
        "extern",
        "inline",
        "volatile",
    }
)


_BUS_KEYWORDS: dict[str, re.Pattern] = {
    "i2c": re.compile(r"\bi2c\b|\btwi\b", re.IGNORECASE),
    "spi": re.compile(r"\bspi\b", re.IGNORECASE),
    "uart": re.compile(r"\buart\b|\busart\b", re.IGNORECASE),
    "gpio": re.compile(r"\bgpio\b|\bpin\b", re.IGNORECASE),
    "adc": re.compile(r"\badc\b", re.IGNORECASE),
}

# Runtime keyword buckets used by the deterministic ranker.
_RUNTIME_KEYWORDS: dict[str, re.Pattern] = {
    "delay": re.compile(
        r"\bdelay\b|\bsleep\b",
        re.IGNORECASE,
    ),
    "thread": re.compile(
        r"\bthread\b|\btask\b|\bmutex\b|\bsemaphore\b",
        re.IGNORECASE,
    ),
    "tick": re.compile(r"\btick\b|\btime\b", re.IGNORECASE),
    "error": re.compile(
        r"\berror\b|\btimeout\b|\bbusy\b",
        re.IGNORECASE,
    ),
}

_BOARD_KEYWORDS: dict[str, re.Pattern] = {
    "board": re.compile(
        r"\bboard\b|\beval\b", re.IGNORECASE
    ),
    "mcu": re.compile(
        r"\bmcu\b|\bcortex\b|\bsoc\b|\bchip\b", re.IGNORECASE
    ),
    "bsp": re.compile(r"\bbsp\b|\bhal\b", re.IGNORECASE),
}


# File walking


def _iter_repo_files(root_path: Path) -> list[Path]:
    """List all files under *root_path*, preferring ripgrep for speed."""
    if not root_path.exists():
        return []
    try:
        # Decode paths strictly first, then fall back with replacement.
        result = subprocess.run(
            ["rg", "--files"],
            check=True,
            capture_output=True,
            cwd=root_path,
            timeout=120,
        )
        try:
            stdout_text = result.stdout.decode("utf-8")
        except UnicodeDecodeError:
            stdout_text = result.stdout.decode("utf-8", errors="replace")
            logger.warning(
                "ripgrep stdout for %s contained non-utf-8 bytes; "
                "falling back to lossy decode (some path characters "
                "may be replaced with \\ufffd)",
                root_path,
            )
        out: list[Path] = []
        for line in stdout_text.splitlines():
            line = line.strip()
            if not line:
                continue
            # Keep hard skips even when a repository lacks ignore rules.
            if any(seg in _HARD_SKIP_DIRS for seg in line.split("/")):
                continue
            out.append(root_path / line)
        return out
    except (FileNotFoundError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        logger.info(
            "ripgrep unavailable for %s (%s); falling back to os.walk",
            root_path,
            exc,
        )

    paths: list[Path] = []
    for dirpath, dirnames, filenames in os.walk(root_path):
        dirnames[:] = [d for d in dirnames if d not in _HARD_SKIP_DIRS]
        for name in filenames:
            paths.append(Path(dirpath) / name)
    return paths


def _is_allowed(path: Path) -> bool:
    if any(part in _HARD_SKIP_DIRS for part in path.parts):
        return False
    if path.suffix.lower() in _ALLOWED_SUFFIXES:
        return True
    if path.name.lower() in _BUILD_NAMES:
        return True
    return False


# Scope filtering


def _matches_default_exclude(rel_path: str, scope_map: ScopeMapEntry | None) -> bool:
    if scope_map is None:
        return False
    if not scope_map.default_exclude:
        return False
    norm = rel_path.replace("\\", "/")
    import fnmatch  # local import — only used here

    return any(fnmatch.fnmatchcase(norm, p) for p in scope_map.default_exclude)


def _is_within_any_scope(
    rel_path: str, scopes_for_root: list[ScopeRule]
) -> bool:
    """Cheap pre-check: at least one scope must include this file."""
    if not scopes_for_root:
        return True
    return any(path_in_scope(rel_path, s) for s in scopes_for_root)


# Symbol and keyword extraction


def _extract_summary(text: str) -> str:
    """Up-to-200-char one-line summary, mirroring 's heuristic."""
    for m in _RE_DOXYGEN_BRIEF.finditer(text[:2000]):
        brief = m.group(1).strip()
        if brief and len(brief) > 5:
            return brief[:200]

    copyright_words = {
        "copyright",
        "license",
        "gpl",
        "mit license",
        "apache",
        "bsd",
        "all rights reserved",
    }
    blocks = re.findall(r"/\*(.+?)\*/", text[:3000], re.DOTALL)
    for block in blocks:
        lines = [l.strip().lstrip("* ").strip() for l in block.split("\n")]
        meaningful = [
            l
            for l in lines
            if l and not any(w in l.lower() for w in copyright_words)
        ]
        if meaningful:
            return " ".join(meaningful[:3])[:200]

    fallback_lines = []
    for line in text[:2000].split("\n"):
        s = line.strip()
        if not s or s.startswith("#include"):
            continue
        fallback_lines.append(s)
        if len(fallback_lines) >= 5:
            break
    return " ".join(fallback_lines)[:200] if fallback_lines else ""


def _extract_exported_symbols(
    text: str, suffix: str, *, cap: int
) -> tuple[list[str], list[SymbolSketch]]:
    """Return ``(name_list, sketch_list)``."""
    if suffix not in _CODE_SUFFIXES:
        return [], []

    sketches: dict[tuple[str, str], SymbolSketch] = {}

    def _add(name: str, kind: str, signature: str | None) -> None:
        if not name or len(name) < 3:
            return
        if name in _NOISE_SYMBOLS:
            return
        key = (name, kind)
        if key in sketches:
            existing = sketches[key]
            if existing.signature is None and signature:
                sketches[key] = SymbolSketch(
                    name=name,
                    kind=kind,
                    signature=signature,
                )
            return
        sketches[key] = SymbolSketch(name=name, kind=kind, signature=signature)

    for m in _RE_FUNC_DECL.finditer(text):
        ret_token = (m.group(1) or "").strip()
        name = m.group(2)
        # Reject declaration matches without a return token.
        if not ret_token:
            continue
        # Keep a rough signature for deterministic ranking.
        start = m.start()
        line_end = text.find("\n", m.end())
        if line_end == -1:
            line_end = min(len(text), m.end() + 200)
        rough_sig = text[start:line_end].strip().rstrip("{").rstrip().rstrip(";")
        # Cap signature length to avoid exploding the index size.
        if len(rough_sig) > 240:
            rough_sig = rough_sig[:240] + " /* ... */"
        _add(name, "function", rough_sig)

    for m in _RE_DEFINE.finditer(text):
        _add(m.group(1), "macro", None)
    for m in _RE_STRUCT.finditer(text):
        _add(m.group(1), "struct", None)
    for m in _RE_TYPEDEF.finditer(text):
        _add(m.group(1), "typedef", None)
    for m in _RE_ENUM.finditer(text):
        _add(m.group(1), "enum", None)

    items = list(sketches.values())[:cap]
    # Deduplicate flat names across symbol kinds.
    names = sorted({s.name for s in items})
    return names, items


def _count_keyword_hits(
    text: str, patterns: dict[str, re.Pattern]
) -> dict[str, int]:
    hits: dict[str, int] = {}
    for key, pat in patterns.items():
        n = len(pat.findall(text))
        if n > 0:
            hits[key] = n
    return hits


_DEFAULT_INCLUDE_DEPS_CAP = 100


def _extract_includes(text: str, cap: int = _DEFAULT_INCLUDE_DEPS_CAP) -> list[str]:
    """Return up to *cap* unique ``#include`` targets, sorted."""
    return sorted(
        {m.group(1) for m in _RE_INCLUDE.finditer(text)}
    )[:cap]


# FileCard build


def _build_file_card(
    root: SourceRoot,
    rel_path: str,
    abs_path: Path,
    scopes_for_root: list[ScopeRule],
    *,
    file_size_limit: int,
    exported_symbols_cap: int,
    summary_max_chars: int,
    include_deps_cap: int = _DEFAULT_INCLUDE_DEPS_CAP,
) -> FileCard | None:
    """Build a FileCard for *abs_path*; return ``None`` to skip."""
    try:
        size = abs_path.stat().st_size
    except OSError:
        return None

    file_kind = _classify_file_kind(rel_path)
    role_hint = assign_dir_role(scopes_for_root, rel_path)

    if size > file_size_limit:
        # Oversize binary blob or generated file — keep it visible in
        # the index but skip content extraction.
        return FileCard(
            root_id=root.root_id,
            path=rel_path,
            abs_path=str(abs_path),
            file_kind=file_kind,
            dir_role_hint=role_hint,
            short_summary="(file too large to index)",
        )

    try:
        text = abs_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None

    suffix = abs_path.suffix.lower()
    exported, sketches = _extract_exported_symbols(
        text, suffix, cap=exported_symbols_cap
    )

    # Backfill SymbolSketch metadata that depends on the host file.
    full_sketches: list[SymbolSketch] = []
    include_deps = _extract_includes(text, cap=include_deps_cap)
    summary = _extract_summary(text)[:summary_max_chars]
    path_tokens = [
        seg.lower()
        for seg in re.split(r"[/_\-.]", rel_path)
        if seg and seg.lower() not in _NOISE_SYMBOLS
    ]
    for s in sketches:
        full_sketches.append(
            SymbolSketch(
                name=s.name,
                kind=s.kind,
                signature=s.signature,
                file=rel_path,
                root_id=root.root_id,
                root_roles=frozenset(root.roles),
                path_tokens=path_tokens,
                file_kind=file_kind,
                dir_role_hint=role_hint,
                include_context=include_deps,
                # Fill nearby_text lazily for binder candidates.
                nearby_text=None,
            )
        )

    return FileCard(
        root_id=root.root_id,
        path=rel_path,
        abs_path=str(abs_path),
        file_kind=file_kind,
        dir_role_hint=role_hint,
        exported_symbols=exported,
        candidate_symbols=full_sketches,
        bus_hits=_count_keyword_hits(text, _BUS_KEYWORDS),
        runtime_hits=_count_keyword_hits(text, _RUNTIME_KEYWORDS),
        board_hits=_count_keyword_hits(text, _BOARD_KEYWORDS),
        mcu_affinity_score=0.0,
        ecosystem_score=1.0,
        include_deps=include_deps,
        short_summary=summary,
    )


# Directory aggregation


def _build_directory_cards(
    file_cards: list[FileCard], max_depth: int = 4
) -> list[DirectoryCard]:
    """Aggregate FileCard stats into per-directory cards."""
    grouped: dict[tuple[str, str], list[FileCard]] = {}
    for card in file_cards:
        parts = card.path.split("/")
        for depth in range(1, min(len(parts), max_depth + 1)):
            dir_path = "/".join(parts[:depth])
            grouped.setdefault((card.root_id, dir_path), []).append(card)

    summaries: list[DirectoryCard] = []
    for (root_id, dir_path), cards in sorted(grouped.items()):
        bus_hits: dict[str, int] = {}
        runtime_hits: dict[str, int] = {}
        board_hits: dict[str, int] = {}
        all_symbols: list[str] = []
        code_count = 0
        role_hint: str | None = None

        for card in cards:
            for k, v in card.bus_hits.items():
                bus_hits[k] = bus_hits.get(k, 0) + v
            for k, v in card.runtime_hits.items():
                runtime_hits[k] = runtime_hits.get(k, 0) + v
            for k, v in card.board_hits.items():
                board_hits[k] = board_hits.get(k, 0) + v
            all_symbols.extend(card.exported_symbols[:5])
            if card.file_kind in {"header", "source"}:
                code_count += 1
            # Use the first non-None hint we encounter — the file ordering
            # within a directory is alphabetical so this is stable.
            if role_hint is None and card.dir_role_hint:
                role_hint = card.dir_role_hint

        depth = dir_path.count("/") + 1
        children = sorted(
            {
                "/".join(c.path.split("/")[: depth + 1])
                for c in cards
                if c.path.count("/") >= depth
            }
        )

        summaries.append(
            DirectoryCard(
                root_id=root_id,
                dir_path=dir_path,
                file_count=len(cards),
                code_file_count=code_count,
                role_hint=role_hint,
                bus_hits={k: v for k, v in sorted(bus_hits.items()) if v > 0},
                runtime_hits={
                    k: v for k, v in sorted(runtime_hits.items()) if v > 0
                },
                board_hits={
                    k: v for k, v in sorted(board_hits.items()) if v > 0
                },
                top_symbols=sorted(set(all_symbols))[:20],
                children=children[:30],
            )
        )

    return summaries


# Include edges


def _build_include_edges(
    file_cards: list[FileCard],
) -> list[IncludeEdge]:
    edges: set[IncludeEdge] = set()
    for card in file_cards:
        for inc in card.include_deps:
            edges.add(
                IncludeEdge(
                    src_root_id=card.root_id,
                    src_path=card.path,
                    dst_path=inc,
                )
            )
    return sorted(edges, key=lambda e: (e.src_root_id, e.src_path, e.dst_path))


# Hashing helpers


def scope_map_hash_for(scope_map: ScopeMapEntry | None) -> str:
    """Stable digest of the ScopeMap raw JSON (or empty hash for None)."""
    if scope_map is None:
        return "no_scope_map"
    blob = json.dumps(scope_map.raw, sort_keys=True, ensure_ascii=False)
    return hashlib.sha1(blob.encode("utf-8")).hexdigest()[:16]


# Public entry point


def build_repo_index_bundle(
    *,
    source_roots: list[SourceRoot],
    scope_map: ScopeMapEntry | None = None,
    indexer_version: str | None = None,
) -> RepoIndexBundle:
    """Scan every SourceRoot, apply the ScopeMap, return a RepoIndexBundle."""
    cfg = load_thresholds()
    indexer_cfg = cfg.get("indexer", {})
    file_size_limit = int(indexer_cfg.get("max_file_size_bytes", 500_000))
    symbols_cap = int(indexer_cfg.get("exported_symbols_per_file_cap", 80))
    summary_chars = int(indexer_cfg.get("header_summary_max_chars", 200))
    include_cap = int(
        indexer_cfg.get("include_deps_per_file_cap", _DEFAULT_INCLUDE_DEPS_CAP)
    )

    if indexer_version is None:
        indexer_version = str(cfg.get("cache", {}).get("indexer_version", "1.0"))

    file_cards: list[FileCard] = []
    root_shas: dict[str, str] = {root.root_id: root.sha for root in source_roots}

    for root in source_roots:
        if not root.path.exists():
            logger.warning(
                "SourceRoot %s path %s does not exist; skipping",
                root.root_id,
                root.path,
            )
            continue

        scopes_for_root = (
            scope_map.scopes_for_root(root) if scope_map is not None else []
        )

        all_files = _iter_repo_files(root.path)
        allowed = [f for f in all_files if _is_allowed(f)]

        kept = 0
        scope_filtered = 0
        default_filtered = 0

        for abs_path in allowed:
            try:
                rel = abs_path.relative_to(root.path).as_posix()
            except ValueError:
                rel = abs_path.name

            if _matches_default_exclude(rel, scope_map):
                default_filtered += 1
                continue
            if not _is_within_any_scope(rel, scopes_for_root):
                scope_filtered += 1
                continue

            card = _build_file_card(
                root,
                rel,
                abs_path,
                scopes_for_root,
                file_size_limit=file_size_limit,
                exported_symbols_cap=symbols_cap,
                summary_max_chars=summary_chars,
                include_deps_cap=include_cap,
            )
            if card is not None:
                file_cards.append(card)
                kept += 1

        logger.info(
            "Indexed root %s: %d kept, %d default-excluded, %d scope-excluded "
            "(of %d allowed / %d total files)",
            root.root_id,
            kept,
            default_filtered,
            scope_filtered,
            len(allowed),
            len(all_files),
        )

    # Stable ordering keeps cache artifacts deterministic.
    file_cards.sort(key=lambda c: (c.root_id, c.path))

    dir_cards = _build_directory_cards(file_cards)
    include_edges = _build_include_edges(file_cards)

    # Aggregate symbols for fast bundle-level lookup.
    symbol_sketches: list[SymbolSketch] = []
    for c in file_cards:
        sketches = sorted(c.candidate_symbols, key=lambda s: (s.kind, s.name))
        symbol_sketches.extend(sketches)

    return RepoIndexBundle(
        roots=list(source_roots),
        file_cards=file_cards,
        dir_cards=dir_cards,
        symbol_sketches=symbol_sketches,
        include_edges=include_edges,
        root_shas=root_shas,
        scope_map_hash=scope_map_hash_for(scope_map),
        indexer_version=indexer_version,
    )


__all__ = [
    "build_repo_index_bundle",
    "scope_map_hash_for",
]
