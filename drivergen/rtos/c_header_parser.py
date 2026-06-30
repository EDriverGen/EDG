"""Deterministic deep C header / source parser (tree-sitter, regex fallback)."""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
import logging
import re
import threading
from dataclasses import dataclass, field, asdict
from pathlib import Path

logger = logging.getLogger(__name__)

PARSER_VERSION = "1.0"
_PARSER_LOCAL = threading.local()

try:
    import tree_sitter
    import tree_sitter_c

    _C_LANGUAGE = tree_sitter.Language(tree_sitter_c.language())
    _PARSER = tree_sitter.Parser(_C_LANGUAGE)
    _PARSER_LOCAL.parser = _PARSER
    _HAS_TREE_SITTER = True
except Exception:
    _HAS_TREE_SITTER = False
    logger.warning("tree-sitter-c not available, falling back to regex parsing")


# Data classes

@dataclass
class FunctionDecl:
    name: str
    return_type: str
    parameters: str  # raw parameter text
    signature: str   # full normalized signature
    is_static: bool = False
    is_inline: bool = False

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class MacroDef:
    name: str
    value: str = ""
    is_function_like: bool = False

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class StructDef:
    name: str
    fields: list[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class TypedefDef:
    name: str
    underlying: str = ""

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class EnumDef:
    name: str
    values: list[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class ParsedFile:
    """Complete structural parse of a single C source/header file."""
    path: str
    function_declarations: list[FunctionDecl] = field(default_factory=list)
    macro_definitions: list[MacroDef] = field(default_factory=list)
    struct_definitions: list[StructDef] = field(default_factory=list)
    typedef_definitions: list[TypedefDef] = field(default_factory=list)
    enum_definitions: list[EnumDef] = field(default_factory=list)
    include_graph: list[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        return {
            "path": self.path,
            "function_declarations": [f.to_dict() for f in self.function_declarations],
            "macro_definitions": [m.to_dict() for m in self.macro_definitions],
            "struct_definitions": [s.to_dict() for s in self.struct_definitions],
            "typedef_definitions": [t.to_dict() for t in self.typedef_definitions],
            "enum_definitions": [e.to_dict() for e in self.enum_definitions],
            "include_graph": self.include_graph,
        }

    @property
    def all_symbol_names(self) -> list[str]:
        """All exported symbol names for quick lookup."""
        names: list[str] = []
        names.extend(f.name for f in self.function_declarations)
        names.extend(m.name for m in self.macro_definitions)
        names.extend(s.name for s in self.struct_definitions)
        names.extend(t.name for t in self.typedef_definitions)
        names.extend(e.name for e in self.enum_definitions)
        return names


# Tree-sitter parsing

def _node_text(node, source: bytes) -> str:
    return source[node.start_byte:node.end_byte].decode("utf-8", errors="replace")


def _normalize_whitespace(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def _parse_with_tree_sitter(source: bytes, rel_path: str) -> ParsedFile:
    """Parse C source using tree-sitter AST."""
    tree = _get_parser().parse(source)
    root = tree.root_node
    result = ParsedFile(path=rel_path)

    for node in _walk_top_level(root):
        if node.type == "function_definition" or node.type == "declaration":
            _try_extract_function(node, source, result)
        elif node.type == "preproc_def":
            _try_extract_macro(node, source, result)
        elif node.type == "preproc_function_def":
            _try_extract_function_macro(node, source, result)
        elif node.type == "struct_specifier":
            _try_extract_struct(node, source, result)
        elif node.type == "enum_specifier":
            _try_extract_enum(node, source, result)
        elif node.type == "type_definition":
            _try_extract_typedef(node, source, result)
        elif node.type == "preproc_include":
            _try_extract_include(node, source, result)

    return result


def _get_parser():
    """Return a thread-local tree-sitter parser instance."""
    parser = getattr(_PARSER_LOCAL, "parser", None)
    if parser is None:
        parser = tree_sitter.Parser(_C_LANGUAGE)
        _PARSER_LOCAL.parser = parser
    return parser


# File-scope wrappers that may contain declarations.
_PREPROC_BLOCK_TYPES = frozenset(
    {
        "preproc_if",
        "preproc_ifdef",
        "preproc_ifndef",
        "preproc_else",
        "preproc_elif",
        "#if",
        "#ifdef",
        "#ifndef",
        "linkage_specification",  # ``extern "C" { ... }`` from C++ headers
        "ERROR",  # tree-sitter recovery node — often wraps real declarations
    }
)

# Nested scopes to skip during top-level extraction.
_SKIP_DESCENT_TYPES = frozenset(
    {
        "compound_statement",
        "parameter_list",
    }
)

# Declaration wrappers worth descending into.
_FILE_SCOPE_CONTAINER_TYPES = frozenset(
    {
        "declaration",
        "declaration_list",
        "type_definition",
    }
)


def _walk_top_level(root):
    """Yield top-level declarations, recursing only into preproc blocks."""
    stack = [root]
    while stack:
        node = stack.pop()
        # Yield this node so the caller can match its type.
        yield node
        # Recurse only through file-scope containers.
        if node.type in _SKIP_DESCENT_TYPES:
            continue
        if (
            node is root
            or node.type in _PREPROC_BLOCK_TYPES
            or node.type in _FILE_SCOPE_CONTAINER_TYPES
        ):
            # Push children in reverse so left-to-right traversal order
            # matches the old depth-first behaviour.
            for child in reversed(node.children):
                stack.append(child)
        # Per-extractor functions handle declaration internals.


def _walk(node):
    """Depth-first walker for callers that need full sub-tree iteration.

    For example, ``_try_extract_function`` walks pointer_declarator subtrees.
    """
    yield node
    for child in node.children:
        yield from _walk(child)


def _try_extract_function(node, source: bytes, result: ParsedFile):
    """Extract function declaration from a tree-sitter node."""
    declarator = None
    for child in node.children:
        if child.type in ("function_declarator", "pointer_declarator"):
            declarator = child
            break
        if child.type == "init_declarator":
            for sub in child.children:
                if sub.type == "function_declarator":
                    declarator = sub
                    break
            if declarator:
                break

    if not declarator:
        # Check for function_declarator nested in pointer_declarator
        for child in node.children:
            if child.type == "pointer_declarator":
                for sub in _walk(child):
                    if sub.type == "function_declarator":
                        declarator = sub
                        break
                if declarator:
                    break

    if not declarator:
        return

    # Find the function name
    func_decl = declarator
    if func_decl.type == "pointer_declarator":
        for sub in _walk(func_decl):
            if sub.type == "function_declarator":
                func_decl = sub
                break

    if func_decl.type != "function_declarator":
        return

    name_node = func_decl.child_by_field_name("declarator")
    if not name_node:
        for child in func_decl.children:
            if child.type == "identifier":
                name_node = child
                break
            if child.type == "parenthesized_declarator":
                for sub in _walk(child):
                    if sub.type == "identifier":
                        name_node = sub
                        break
                break

    if not name_node:
        return

    name = _node_text(name_node, source)
    if not name or len(name) < 2:
        return

    # Extract return type (everything before the declarator in the declaration)
    full_text = _node_text(node, source)
    decl_start = _node_text(declarator, source)
    idx = full_text.find(decl_start)
    ret_type = _normalize_whitespace(full_text[:idx]) if idx > 0 else ""

    # Extract parameters
    params_node = func_decl.child_by_field_name("parameters")
    params = _node_text(params_node, source) if params_node else "()"
    params_inner = params.strip("()")

    is_static = "static" in ret_type.split()
    is_inline = "inline" in ret_type.split()

    # Clean return type
    clean_ret = ret_type
    for kw in ("static", "inline", "extern"):
        clean_ret = re.sub(r"\b" + kw + r"\b", "", clean_ret).strip()
    if not clean_ret:
        clean_ret = ret_type

    signature = f"{_normalize_whitespace(ret_type)} {name}({_normalize_whitespace(params_inner)})"

    result.function_declarations.append(FunctionDecl(
        name=name,
        return_type=clean_ret,
        parameters=_normalize_whitespace(params_inner),
        signature=signature,
        is_static=is_static,
        is_inline=is_inline,
    ))


def _try_extract_macro(node, source: bytes, result: ParsedFile):
    name_node = node.child_by_field_name("name")
    value_node = node.child_by_field_name("value")
    if not name_node:
        return
    name = _node_text(name_node, source)
    value = _node_text(value_node, source).strip() if value_node else ""
    if name and len(name) >= 2:
        result.macro_definitions.append(MacroDef(name=name, value=value[:100]))


def _try_extract_function_macro(node, source: bytes, result: ParsedFile):
    name_node = node.child_by_field_name("name")
    if not name_node:
        return
    name = _node_text(name_node, source)
    if name and len(name) >= 2:
        result.macro_definitions.append(MacroDef(
            name=name, value="", is_function_like=True,
        ))


def _try_extract_struct(node, source: bytes, result: ParsedFile):
    name_node = node.child_by_field_name("name")
    if not name_node:
        return
    name = _node_text(name_node, source)
    body_node = node.child_by_field_name("body")
    fields: list[str] = []
    if body_node:
        for child in body_node.children:
            if child.type == "field_declaration":
                fields.append(_normalize_whitespace(_node_text(child, source).rstrip(";")))
    if name and len(name) >= 2:
        result.struct_definitions.append(StructDef(name=name, fields=fields[:20]))


def _try_extract_enum(node, source: bytes, result: ParsedFile):
    name_node = node.child_by_field_name("name")
    if not name_node:
        return
    name = _node_text(name_node, source)
    body_node = node.child_by_field_name("body")
    values: list[str] = []
    if body_node:
        for child in body_node.children:
            if child.type == "enumerator":
                val_name = child.child_by_field_name("name")
                if val_name:
                    values.append(_node_text(val_name, source))
    if name and len(name) >= 2:
        result.enum_definitions.append(EnumDef(name=name, values=values[:30]))


def _typedef_inner_identifier(decl_node, source: bytes) -> str | None:
    """Recursively walk a tree-sitter declarator subtree to find the innermost identifier that names the typedef."""
    if decl_node is None:
        return None
    if decl_node.type in ("identifier", "type_identifier"):
        return _node_text(decl_node, source)
    # Walk nested declarator shapes through their declarator field.
    inner = decl_node.child_by_field_name("declarator")
    if inner is not None:
        return _typedef_inner_identifier(inner, source)
    # Last-resort: scan children for the first identifier we find.
    # ``init_declarator`` / unusual shapes land here.
    for child in decl_node.children:
        if child.type == "identifier":
            return _node_text(child, source)
        rec = _typedef_inner_identifier(child, source)
        if rec is not None:
            return rec
    return None


def _try_extract_typedef(node, source: bytes, result: ParsedFile):
    """Extract typedef name(s) - supports function pointers and multi-declarator forms."""
    text = _node_text(node, source)

    # Collect every identifier-bearing declarator in a typedef.
    raw_decls = []
    for child in node.children:
        if child.type in (
            "identifier",
            "type_identifier",
            "pointer_declarator",
            "function_declarator",
            "array_declarator",
            "parenthesized_declarator",
            "init_declarator",
        ):
            raw_decls.append(child)
    # Fall back to the field-named declarator if the loop above came up
    # empty (single-decl typedef on some grammar builds).
    if not raw_decls:
        primary = node.child_by_field_name("declarator")
        if primary is not None:
            raw_decls.append(primary)

    names: list[str] = []
    for decl in raw_decls:
        name = _typedef_inner_identifier(decl, source)
        if name and len(name) >= 2 and name not in names:
            names.append(name)

    if not names:
        return

    # Strip the first declarator name to approximate the underlying type.
    primary_name = names[0]
    underlying = _normalize_whitespace(
        text.replace("typedef", "", 1).rsplit(primary_name, 1)[0]
    )

    for name in names:
        result.typedef_definitions.append(
            TypedefDef(name=name, underlying=underlying[:100])
        )


def _try_extract_include(node, source: bytes, result: ParsedFile):
    path_node = node.child_by_field_name("path")
    if path_node:
        inc = _node_text(path_node, source).strip('"<>')
        if inc:
            result.include_graph.append(inc)


# Regex fallback

_RE_FUNC_FULL = re.compile(
    r"^\s*"
    r"((?:(?:static|extern|inline|__attribute__\s*\([^)]*\))\s+)*"
    r"(?:(?:const|volatile|unsigned|signed|long|short|struct|enum|union)\s+)*"
    r"[A-Za-z_]\w*(?:\s*\*)*)\s+"
    r"([A-Za-z_]\w*)\s*\(([^)]*)\)",
    re.MULTILINE,
)
_RE_INCLUDE_SIMPLE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
_RE_DEFINE_SIMPLE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)(?:\s+(.+))?$", re.MULTILINE)
_RE_STRUCT_SIMPLE = re.compile(r"\bstruct\s+([A-Za-z_]\w*)\s*\{", re.MULTILINE)
_RE_TYPEDEF_SIMPLE = re.compile(r"\btypedef\s+.*?\b([A-Za-z_]\w*)\s*;", re.MULTILINE | re.DOTALL)
_RE_ENUM_SIMPLE = re.compile(r"\benum\s+([A-Za-z_]\w*)\s*\{", re.MULTILINE)

_C_CONTROL_KW = frozenset({
    "if", "else", "for", "while", "do", "switch", "case", "break",
    "continue", "return", "goto", "sizeof", "typeof", "default", "defined",
})


def _parse_with_regex(text: str, rel_path: str) -> ParsedFile:
    """Fallback regex-based parsing when tree-sitter is unavailable."""
    result = ParsedFile(path=rel_path)

    for m in _RE_FUNC_FULL.finditer(text):
        ret = m.group(1).strip()
        name = m.group(2)
        params = m.group(3).strip()
        if name in _C_CONTROL_KW or len(name) < 2:
            continue
        base_type = ret.split()[-1].rstrip("*").strip()
        if base_type in _C_CONTROL_KW:
            continue
        result.function_declarations.append(FunctionDecl(
            name=name,
            return_type=ret,
            parameters=params,
            signature=f"{ret} {name}({params})",
            is_static="static" in ret,
            is_inline="inline" in ret,
        ))

    for m in _RE_DEFINE_SIMPLE.finditer(text):
        name = m.group(1)
        value = (m.group(2) or "").strip()[:100]
        if name and len(name) >= 2:
            result.macro_definitions.append(MacroDef(name=name, value=value))

    for m in _RE_STRUCT_SIMPLE.finditer(text):
        name = m.group(1)
        if name and len(name) >= 2:
            result.struct_definitions.append(StructDef(name=name))

    for m in _RE_TYPEDEF_SIMPLE.finditer(text):
        name = m.group(1)
        if name and len(name) >= 2:
            result.typedef_definitions.append(TypedefDef(name=name))

    for m in _RE_ENUM_SIMPLE.finditer(text):
        name = m.group(1)
        if name and len(name) >= 2:
            result.enum_definitions.append(EnumDef(name=name))

    for m in _RE_INCLUDE_SIMPLE.finditer(text):
        result.include_graph.append(m.group(1))

    return result


# Public API

def deep_parse_file(file_path: Path, rel_path: str | None = None) -> ParsedFile:
    """Parse a single C source/header file and return structured results."""
    if rel_path is None:
        rel_path = file_path.name

    try:
        raw = file_path.read_bytes()
    except OSError as e:
        logger.warning("Cannot read %s: %s", file_path, e)
        return ParsedFile(path=rel_path)

    if _HAS_TREE_SITTER:
        try:
            return _parse_with_tree_sitter(raw, rel_path)
        except Exception as e:
            logger.warning("tree-sitter parse failed for %s: %s, falling back to regex", rel_path, e)

    text = raw.decode("utf-8", errors="replace")
    return _parse_with_regex(text, rel_path)


def _configured_parser_workers(default: int = 4) -> int:
    """Return ``thresholds.indexer.parser_workers`` with fallback."""
    try:
        from .config import load_thresholds  # local import to avoid cycles

        cfg = load_thresholds().get("indexer", {})
        raw = cfg.get("parser_workers", default)
        workers = int(raw)
    except Exception as e:
        logger.debug("Cannot load parser_workers from thresholds: %s", e)
        workers = default
    return max(1, workers)


def _deep_parse_file_tuple(item: tuple[Path, str]) -> ParsedFile:
    abs_path, rel_path = item
    try:
        return deep_parse_file(abs_path, rel_path)
    except Exception as e:
        logger.warning("deep parse worker failed for %s: %s", rel_path, e)
        return ParsedFile(path=rel_path)


def deep_parse_files(
    file_paths: list[tuple[Path, str]],
    max_workers: int | None = None,
) -> list[ParsedFile]:
    """Parse multiple files."""
    if not file_paths:
        return []

    workers = _configured_parser_workers() if max_workers is None else int(max_workers)
    workers = max(1, min(workers, len(file_paths)))
    if workers == 1:
        return [_deep_parse_file_tuple(item) for item in file_paths]

    with ThreadPoolExecutor(max_workers=workers, thread_name_prefix="rtos-c-parser") as executor:
        return list(executor.map(_deep_parse_file_tuple, file_paths))
