"""Codegen-facing API surface checks."""

from __future__ import annotations

import copy
import re
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

from .stub_compiler import _STUBS_DIR, available_stub_headers


_C_IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_C_DECL_LINE_RE = re.compile(
    r"^\s*(?!typedef\b)(?!#)[^;{}]*\b"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*;",
    re.MULTILINE,
)
_C_FUNCTION_DEF_RE = re.compile(
    r"^\s*(?!typedef\b)(?!#)[^;{}]*\b"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{",
    re.MULTILINE,
)
_C_MACRO_RE = re.compile(
    r"^\s*#\s*define\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\b(?P<body>[^\r\n]*)",
    re.MULTILINE,
)
_STRUCT_RE = re.compile(
    r"\bstruct\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\{(?P<body>.*?)\}\s*;",
    re.DOTALL,
)
_COMMENT_RE = re.compile(r"/\*.*?\*/|//[^\r\n]*", re.DOTALL)
_INCLUDE_DIRECTIVE_RE = re.compile(r'#[ \t]*include[ \t]*[<"]([^>"]+)[>"]')

_STANDARD_HEADERS = {
    "assert.h",
    "ctype.h",
    "errno.h",
    "float.h",
    "inttypes.h",
    "limits.h",
    "math.h",
    "stdbool.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "time.h",
}

_NON_DRIVER_PATH_TOKENS = {
    "app",
    "apps",
    "cli",
    "cmd",
    "commands",
    "demo",
    "demos",
    "doc",
    "docs",
    "example",
    "examples",
    "sample",
    "samples",
    "shell",
    "test",
    "tests",
    "testing",
    "tool",
    "tools",
}


def sanitize_rtos_contract_for_codegen(
    rtos_contract: Mapping[str, Any],
) -> Dict[str, Any]:
    """sanitize_rtos_contract_for_codegen helper."""
    contract: Dict[str, Any] = copy.deepcopy(dict(rtos_contract or {}))
    rtos_id = str(contract.get("rtos") or "").strip()
    stub_index = _build_stub_index(rtos_id)
    api_bindings = contract.get("api_bindings")
    if not isinstance(api_bindings, Mapping):
        contract["codegen_surface"] = _surface_payload(
            rtos_id=rtos_id,
            warnings=(),
            forbidden_symbols=(),
            forbidden_headers=(),
            link_unavailable_symbols=(),
            support_headers=(),
            struct_fields=stub_index.struct_fields,
            symbol_aliases={},
            removed_bindings=(),
        )
        return contract

    warnings: List[str] = []
    forbidden_symbols: List[str] = []
    forbidden_headers: List[str] = []
    link_unavailable_symbols: List[str] = []
    removed_bindings: List[Mapping[str, Any]] = []
    symbol_aliases: Dict[str, str] = {}
    sanitized: Dict[str, Any] = {}

    for slot_id, raw_binding in api_bindings.items():
        if not isinstance(raw_binding, Mapping):
            continue
        binding = copy.deepcopy(dict(raw_binding))
        symbol = str(binding.get("symbol") or "").strip()
        source_kind = str(binding.get("source_kind") or "").strip()
        source_blob = _binding_source_blob(binding)

        if (
            source_kind != "task_package_helper"
            and _looks_non_driver_source(source_blob)
        ):
            if symbol:
                forbidden_symbols.append(symbol)
            removed_bindings.append(
                {
                    "slot_id": slot_id,
                    "symbol": symbol,
                    "reason": "non-driver app/tool/example/test source path",
                    "source": source_blob,
                }
            )
            warnings.append(
                f"removed `{symbol}` from slot `{slot_id}`: source path "
                "looks like an app/tool/example/test surface, not a driver API"
            )
            continue

        if symbol and _looks_private_impl_symbol(symbol):
            public_alias = _public_alias_for_private_symbol(symbol, stub_index)
            if public_alias:
                old = symbol
                binding["symbol"] = public_alias
                binding["signature"] = _signature_for_symbol(public_alias, stub_index) or _replace_symbol_in_signature(
                    str(binding.get("signature") or ""), old, public_alias
                )
                symbol = public_alias
                symbol_aliases[old] = public_alias
                forbidden_symbols.append(old)
                warnings.append(
                    f"slot `{slot_id}` uses public alias `{public_alias}` "
                    f"instead of implementation symbol `{old}`"
                )

        headers, removed = _sanitize_required_headers(binding, stub_index)
        if removed:
            forbidden_headers.extend(removed)
            warnings.append(
                f"slot `{slot_id}` removed unavailable header(s): "
                + ", ".join(f"`{h}`" for h in removed)
            )

        if _binding_is_callable(binding):
            stub_signature = _signature_for_symbol(symbol, stub_index)
            if stub_signature and not str(binding.get("signature") or "").strip():
                binding["signature"] = stub_signature
            if not headers:
                inferred = _headers_for_symbol(symbol, stub_index)
                for header in inferred:
                    if header not in headers:
                        headers.append(header)
            if (
                symbol
                and rtos_id
                and symbol not in stub_index.symbols
                and not headers
                and binding.get("source_kind") != "task_package_helper"
            ):
                forbidden_symbols.append(symbol)
                removed_bindings.append(
                    {
                        "slot_id": slot_id,
                        "symbol": symbol,
                        "reason": "not declared by codegen stub headers",
                    }
                )
                warnings.append(
                    f"removed `{symbol}` from slot `{slot_id}`: the symbol "
                    "was not declared by codegen stub headers and has no "
                    "usable required header"
                )
                continue
            if symbol and symbol not in stub_index.symbols and not headers:
                warnings.append(
                    f"slot `{slot_id}` symbol `{symbol}` was not found in "
                    "stub headers and has no usable required header; codegen "
                    "may need a task-package helper or contract repair"
                )
            elif (
                symbol
                and symbol in stub_index.symbols
                and symbol not in stub_index.implemented_symbols
                and binding.get("source_kind") != "task_package_helper"
            ):
                forbidden_symbols.append(symbol)
                link_unavailable_symbols.append(symbol)
                removed_bindings.append(
                    {
                        "slot_id": slot_id,
                        "symbol": symbol,
                        "reason": "declared by stub headers but not implemented by link stubs",
                    }
                )
                warnings.append(
                    f"removed `{symbol}` from slot `{slot_id}`: the symbol "
                    "is declared in the stub headers but has no link-time "
                    "implementation in evaluation stubs"
                )
                continue

        binding["required_headers"] = headers
        sanitized[str(slot_id)] = binding

    integration = contract.get("integration_contract")
    support_headers: List[str] = []
    if isinstance(integration, Mapping):
        integration_copy = copy.deepcopy(dict(integration))
        support_headers, removed = _sanitize_header_values(
            integration_copy.get("include_headers") or [],
            stub_index,
        )
        if removed:
            forbidden_headers.extend(removed)
            warnings.append(
                "integration contract removed unavailable support header(s): "
                + ", ".join(f"`{h}`" for h in removed)
            )
        integration_copy["include_headers"] = support_headers
        contract["integration_contract"] = integration_copy

    contract["api_bindings"] = sanitized
    contract["codegen_surface"] = _surface_payload(
        rtos_id=rtos_id,
        warnings=_dedupe(warnings),
        forbidden_symbols=_dedupe(forbidden_symbols),
        forbidden_headers=_dedupe(forbidden_headers),
        link_unavailable_symbols=_dedupe(link_unavailable_symbols),
        support_headers=_dedupe(support_headers),
        struct_fields=stub_index.struct_fields,
        symbol_aliases=symbol_aliases,
        removed_bindings=tuple(removed_bindings),
    )
    if forbidden_symbols or forbidden_headers:
        existing = contract.get("forbidden_assumptions")
        forbidden: List[Any] = list(existing) if isinstance(existing, list) else []
        for sym in _dedupe(forbidden_symbols):
            forbidden.append(f"Do not call forbidden surface symbol `{sym}`.")
        for header in _dedupe(forbidden_headers):
            forbidden.append(f"Do not include unavailable header `{header}`.")
        contract["forbidden_assumptions"] = forbidden
    return contract


def struct_field_validation_errors(
    driver_header: str,
    driver_source: str,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    """Return errors for generated accesses to non-existent struct fields."""
    surface = sanitize_rtos_contract_for_codegen(rtos_contract).get("codegen_surface")
    fields = (surface or {}).get("struct_fields") if isinstance(surface, Mapping) else {}
    if not isinstance(fields, Mapping) or not fields:
        return ()
    code = f"{driver_header}\n{driver_source}"
    var_types = _struct_variable_types(code, fields.keys())
    if not var_types:
        return ()

    errors: List[str] = []
    stripped = _COMMENT_RE.sub(" ", code)
    for var, struct_name in sorted(var_types.items()):
        allowed = tuple(str(x) for x in fields.get(struct_name, ()) if x)
        if not allowed:
            continue
        pat = re.compile(
            rf"\b{re.escape(var)}\b(?:\s*\[[^\]]+\])?\s*(?P<op>\.|->)\s*"
            rf"(?P<field>[A-Za-z_][A-Za-z0-9_]*)"
        )
        for match in pat.finditer(stripped):
            field = match.group("field")
            if field in allowed:
                continue
            line_no = stripped.count("\n", 0, match.start()) + 1
            errors.append(
                f"driver_source/header line {line_no}: `struct {struct_name}` "
                f"has no field `{field}` in the codegen surface. "
                f"Allowed fields are: {', '.join(f'`{name}`' for name in allowed)}."
            )
    return tuple(errors)


def forbidden_surface_usage_errors(
    driver_header: str,
    driver_source: str,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    """Return errors when generated code uses sanitized-away symbols/headers."""
    sanitized = sanitize_rtos_contract_for_codegen(rtos_contract)
    surface = sanitized.get("codegen_surface")
    if not isinstance(surface, Mapping):
        return ()
    code = f"{driver_header}\n{driver_source}"
    stripped = _COMMENT_RE.sub(" ", code)
    errors: List[str] = []

    include_names = [
        match.group(1).strip().replace("\\", "/")
        for match in _INCLUDE_DIRECTIVE_RE.finditer(stripped)
    ]
    for header in surface.get("forbidden_headers") or []:
        forbidden = str(header or "").strip().strip("<>").strip('"').replace("\\", "/")
        base = _basename(forbidden)
        if not forbidden or not base:
            continue
        if any(_include_matches_forbidden(include, forbidden) for include in include_names):
            errors.append(
                f"driver_source/header includes unavailable surface header `{base}`. "
                "Use only headers rendered in SECTION C after codegen-surface "
                "sanitization."
            )

    for symbol in surface.get("forbidden_symbols") or []:
        if not _C_IDENTIFIER_RE.match(str(symbol or "")):
            continue
        if re.search(rf"\b{re.escape(str(symbol))}\s*\(", stripped):
            alias = (surface.get("symbol_aliases") or {}).get(symbol)
            suffix = f" Use public alias `{alias}` instead." if alias else ""
            errors.append(
                f"driver_source/header calls forbidden surface symbol "
                f"`{symbol}`.{suffix}"
            )
    return tuple(errors)


def _include_matches_forbidden(include_name: str, forbidden_header: str) -> bool:
    include = str(include_name or "").strip().replace("\\", "/")
    forbidden = str(forbidden_header or "").strip().replace("\\", "/")
    if not include or not forbidden:
        return False
    if "/" in forbidden:
        return include == forbidden
    return include == forbidden


def stub_headers_declaring_symbol(rtos_id: str, symbol: str) -> Tuple[str, ...]:
    """Return stub header basenames that declare ``symbol`` for ``rtos_id``."""
    text = str(symbol or "").strip()
    if not text or not _C_IDENTIFIER_RE.match(text):
        return ()
    pair = _build_stub_index(str(rtos_id or "")).symbols.get(text)
    return (pair[0],) if pair and pair[0] else ()


class _StubIndex:
    def __init__(
        self,
        *,
        rtos_id: str,
        headers: Mapping[str, str],
        symbols: Mapping[str, Tuple[str, str]],
        implemented_symbols: Iterable[str],
        struct_fields: Mapping[str, Tuple[str, ...]],
    ) -> None:
        self.rtos_id = rtos_id
        self.headers = dict(headers)
        self.symbols = dict(symbols)
        self.implemented_symbols = set(implemented_symbols)
        self.struct_fields = dict(struct_fields)

    @property
    def available_headers(self) -> set[str]:
        return set(self.headers)


def _build_stub_index(rtos_id: str) -> _StubIndex:
    stub_dir = _STUBS_DIR / str(rtos_id or "")
    headers: Dict[str, str] = {}
    symbols: Dict[str, Tuple[str, str]] = {}
    implemented_symbols: set[str] = set()
    struct_fields: Dict[str, Tuple[str, ...]] = {}
    if not stub_dir.is_dir():
        return _StubIndex(
            rtos_id=rtos_id,
            headers={},
            symbols={},
            implemented_symbols=(),
            struct_fields={},
        )
    for path in sorted(stub_dir.rglob("*.h")):
        if not path.is_file():
            continue
        text = _read_text_lossy(path)
        include_name = path.relative_to(stub_dir).as_posix()
        headers.setdefault(include_name, text)
        stripped_text = _COMMENT_RE.sub(" ", text)
        for match in _C_DECL_LINE_RE.finditer(stripped_text):
            name = match.group("name")
            signature = match.group(0).strip().rstrip(";")
            symbols.setdefault(name, (include_name, signature))
        for match in _C_MACRO_RE.finditer(stripped_text):
            name = match.group("name")
            body = match.group("body").strip()
            symbols.setdefault(name, (include_name, f"#define {name} {body}".strip()))
        for struct_name, fields in _parse_struct_fields(text).items():
            struct_fields.setdefault(struct_name, fields)
    for path in sorted(stub_dir.glob("stubs*.c")):
        if not path.is_file():
            continue
        text = _COMMENT_RE.sub(
            " ",
            _read_text_lossy(path),
        )
        for match in _C_FUNCTION_DEF_RE.finditer(text):
            implemented_symbols.add(match.group("name"))
    # ``available_stub_headers`` is the public API and may include forwarders
    # whose text we did not read due to platform/path oddities.
    for base in available_stub_headers(rtos_id):
        headers.setdefault(base, "")
    return _StubIndex(
        rtos_id=rtos_id,
        headers=headers,
        symbols=symbols,
        implemented_symbols=implemented_symbols,
        struct_fields=struct_fields,
    )


def _read_text_lossy(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "utf-16"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("utf-8", errors="ignore")


def _parse_struct_fields(text: str) -> Dict[str, Tuple[str, ...]]:
    out: Dict[str, Tuple[str, ...]] = {}
    stripped = _COMMENT_RE.sub(" ", text)
    for match in _STRUCT_RE.finditer(stripped):
        name = match.group("name")
        body = match.group("body")
        fields: List[str] = []
        for declaration in body.split(";"):
            declaration = declaration.strip()
            if not declaration or "(" in declaration or ")" in declaration:
                continue
            # Use the final identifier in each comma-separated field part.
            for part in declaration.split(","):
                part = part.strip()
                if not part or "{" in part or "}" in part:
                    continue
                part = re.sub(r"\[[^\]]*\]", " ", part)
                part = part.split(":", 1)[0]
                identifiers = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", part)
                if not identifiers:
                    continue
                field = identifiers[-1]
                if field and field not in fields:
                    fields.append(field)
        if fields:
            out[name] = tuple(fields)
    return out


def _surface_payload(
    *,
    rtos_id: str,
    warnings: Sequence[str],
    forbidden_symbols: Sequence[str],
    forbidden_headers: Sequence[str],
    link_unavailable_symbols: Sequence[str],
    support_headers: Sequence[str],
    struct_fields: Mapping[str, Sequence[str]],
    symbol_aliases: Mapping[str, str],
    removed_bindings: Sequence[Mapping[str, Any]],
) -> Dict[str, Any]:
    return {
        "version": 1,
        "rtos_id": rtos_id,
        "warnings": list(warnings),
        "forbidden_symbols": list(forbidden_symbols),
        "forbidden_headers": list(forbidden_headers),
        "link_unavailable_symbols": list(link_unavailable_symbols),
        "support_headers": list(support_headers),
        "struct_fields": {
            key: list(value)
            for key, value in sorted(struct_fields.items())
            if value
        },
        "symbol_aliases": dict(sorted(symbol_aliases.items())),
        "removed_bindings": list(removed_bindings),
    }


def _binding_source_blob(binding: Mapping[str, Any]) -> str:
    parts: List[str] = []
    for key in ("declared_in", "implemented_in", "source_path", "path", "file"):
        value = binding.get(key)
        if value:
            parts.append(str(value))
    for span in binding.get("evidence") or binding.get("evidence_spans") or []:
        if isinstance(span, Mapping):
            for key in ("path", "file", "source_path"):
                if span.get(key):
                    parts.append(str(span.get(key)))
        elif span:
            parts.append(str(span))
    return " ".join(parts)


def _looks_non_driver_source(source_blob: str) -> bool:
    if not source_blob:
        return False
    text = source_blob.replace("\\", "/").lower()
    segments = [segment for segment in re.split(r"[/:\s]+", text) if segment]
    for segment in segments:
        clean = segment.strip()
        if clean in _NON_DRIVER_PATH_TOKENS:
            return True
        if clean.endswith("-apps") or clean.endswith("_apps"):
            return True
    return False


def _looks_private_impl_symbol(symbol: str) -> bool:
    return symbol.startswith("_") and not symbol.startswith("__")


def _public_alias_for_private_symbol(symbol: str, stub_index: _StubIndex) -> str:
    if not _looks_private_impl_symbol(symbol):
        return ""
    alias = symbol.lstrip("_")
    if alias and alias in stub_index.symbols:
        return alias
    return ""


def _signature_for_symbol(symbol: str, stub_index: _StubIndex) -> str:
    pair = stub_index.symbols.get(symbol)
    return pair[1] if pair else ""


def _headers_for_symbol(symbol: str, stub_index: _StubIndex) -> List[str]:
    pair = stub_index.symbols.get(symbol)
    return [pair[0]] if pair and pair[0] else []


def _replace_symbol_in_signature(signature: str, old: str, new: str) -> str:
    if not signature:
        return signature
    return re.sub(rf"\b{re.escape(old)}\b", new, signature)


def _sanitize_required_headers(
    binding: Mapping[str, Any],
    stub_index: _StubIndex,
) -> Tuple[List[str], List[str]]:
    return _sanitize_header_values(binding.get("required_headers") or [], stub_index)


def _sanitize_header_values(
    raw_headers: Iterable[Any],
    stub_index: _StubIndex,
) -> Tuple[List[str], List[str]]:
    headers: List[str] = []
    removed: List[str] = []
    available = stub_index.available_headers
    for raw in raw_headers:
        text = str(raw or "").strip().replace("\\", "/")
        candidates = _include_name_candidates(text)
        if not candidates:
            continue
        chosen = ""
        for candidate in candidates:
            if candidate in _STANDARD_HEADERS or candidate in available:
                chosen = candidate
                break
        if chosen:
            if chosen not in headers:
                headers.append(chosen)
            continue
        removed.append(_basename(text) or candidates[0])
    return headers, _dedupe(removed)


def _binding_is_callable(binding: Mapping[str, Any]) -> bool:
    kind = str(binding.get("kind") or "function").strip().lower()
    return kind in {"", "function", "func"}


def _basename(value: Any) -> str:
    text = str(value or "").strip().strip("<>").strip('"')
    if not text:
        return ""
    return re.split(r"[/\\]", text)[-1]


def _include_name_candidates(value: Any) -> List[str]:
    text = str(value or "").strip().strip("<>").strip('"').replace("\\", "/")
    if not text:
        return []
    candidates: List[str] = []
    include_match = re.search(r"(?:^|/)include/(.+)$", text)
    if include_match:
        candidates.append(include_match.group(1).strip("/"))
    for prefix in ("cpukit/include/", "include/"):
        if text.startswith(prefix):
            candidates.append(text[len(prefix):])
    candidates.append(text)
    base = _basename(text)
    if base:
        candidates.append(base)
    return _dedupe(candidates)


def _dedupe(items: Iterable[str]) -> List[str]:
    out: List[str] = []
    seen: set[str] = set()
    for item in items:
        text = str(item or "").strip()
        if not text or text in seen:
            continue
        seen.add(text)
        out.append(text)
    return out


def _struct_variable_types(
    code: str,
    struct_names: Iterable[str],
) -> Dict[str, str]:
    names = [name for name in struct_names if _C_IDENTIFIER_RE.match(str(name or ""))]
    if not names:
        return {}
    stripped = _COMMENT_RE.sub(" ", code)
    out: Dict[str, str] = {}
    for struct_name in names:
        # Match scalar, array, and pointer struct variables.
        pat = re.compile(
            rf"\bstruct\s+{re.escape(struct_name)}\s+"
            rf"(?P<ptr>\*+\s*)?(?P<var>[A-Za-z_][A-Za-z0-9_]*)"
            rf"\s*(?:\[[^\]]*\])?\s*(?:=|;|,)"
        )
        for match in pat.finditer(stripped):
            out.setdefault(match.group("var"), struct_name)
    return out


def signature_config_pointer_args(signature: str) -> Tuple[Tuple[int, str, str, str], ...]:
    """Return config-like ``struct *`` parameters in a C function signature."""
    arg_text = _signature_argument_text(signature)
    if not arg_text:
        return ()
    args = _split_top_level_commas(arg_text)
    out: List[Tuple[int, str, str, str]] = []
    for index, raw_arg in enumerate(args, start=1):
        arg = " ".join(str(raw_arg or "").strip().split())
        if not arg or arg == "void" or "*" not in arg:
            continue
        struct_match = re.search(
            r"\bstruct\s+(?P<struct>[A-Za-z_][A-Za-z0-9_]*)\b[^,;()]*\*",
            arg,
        )
        if not struct_match:
            continue
        struct_name = struct_match.group("struct")
        param_name = _parameter_name_from_arg(arg)
        blob = f"{struct_name} {param_name}".lower()
        if "config" not in blob and "cfg" not in blob:
            continue
        out.append((index, struct_name, param_name, arg))
    return tuple(out)


def _signature_argument_text(signature: str) -> str:
    text = str(signature or "").strip().rstrip(";")
    if not text:
        return ""
    open_idx = text.find("(")
    if open_idx < 0:
        return ""
    depth = 0
    for i in range(open_idx, len(text)):
        ch = text[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return text[open_idx + 1:i].strip()
    return ""


def _split_top_level_commas(text: str) -> List[str]:
    items: List[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(str(text or "")):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            items.append(text[start:i].strip())
            start = i + 1
    tail = text[start:].strip()
    if tail:
        items.append(tail)
    return items


def _parameter_name_from_arg(arg: str) -> str:
    text = str(arg or "").strip()
    if not text or text == "...":
        return ""
    text = re.sub(r"\[[^\]]*\]\s*$", "", text).strip()
    text = text.rstrip("*").strip()
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", text)
    return match.group(1) if match else ""


__all__ = [
    "forbidden_surface_usage_errors",
    "sanitize_rtos_contract_for_codegen",
    "signature_config_pointer_args",
    "stub_headers_declaring_symbol",
    "struct_field_validation_errors",
]
