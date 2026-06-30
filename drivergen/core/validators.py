import re
from typing import List, Optional

from .expression_safety import validate_expression_object
from .models import (
    DEVICE_IR_REQUIRED_FIELDS,
    KERNEL_PROFILE_REQUIRED_FIELDS,
    ValidationIssue,
    ValidationResult,
    append_missing_field_issues,
)

# Enumerations are duplicated here instead of imported from response_schemas so
# this validator stays dependency-light.
_REGISTER_MAP_ADDRESSING = frozenset(
    {"1-byte", "2-byte", "pointer", "command_opcode"}
)
_READ_CHANNEL_RAW_TYPES = frozenset(
    {"uint8", "int8", "uint16", "int16", "uint32", "int32", "float", "bytes"}
)
_RAW_ENCODING_BYTE_ORDER = frozenset(
    {"big_endian", "little_endian", "single_byte"}
)

# Allowed kinds for read_sequence[*].transaction / init_sequence[*].transaction.
_SEQUENCE_TRANSACTION_KINDS = frozenset({"write", "read", "write_then_read"})
# Tokens allowed in ``transaction.bytes`` besides raw integers 0..255. "DATA"
# is a runtime-filled placeholder.
_TRANSACTION_BYTES_LITERAL_RE = re.compile(r"^0x[0-9A-Fa-f]{1,2}$")

_ACCESS_MODEL_KINDS = frozenset({
    "register_pointer",
    "register_auto_increment",
    "command_then_direct_read",
    "memory",
    "stream",
    "packet",
    "gpio_timing",
    "unknown",
})
_OPERATION_FLOW_KINDS = frozenset({
    "init",
    "probe",
    "read",
    "calibration",
    "write",
    "power",
    "other",
})
_OPERATION_FLOW_STEP_OPS = frozenset({
    "write",
    "read",
    "write_then_read",
    "delay",
    "poll_until",
    "wait_until_ready",
    "select_page",
    "clear",
    "postprocess",
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
})
_BUS_STEP_OPS = frozenset({"write", "read", "write_then_read"})
_SIGNAL_STEP_OPS = frozenset({
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
})

_DEFAULT_ONLY_INIT_RE = re.compile(
    r"\b(defaults?|reset\s+state|no\s+bus\s+(?:init|initiali[sz]ation|write)|"
    r"no\s+explicit\s+(?:config|configuration|setup|write)|"
    r"config(?:uration)?\s+writes?\s+(?:is|are)\s+not\s+required)\b",
    re.IGNORECASE,
)


def _normalize_match_text(text: str) -> str:
    normalized = str(text or "").lower()
    replacements = {
        "\u2018": "'",
        "\u2019": "'",
        "\u201c": '"',
        "\u201d": '"',
        "\u2013": "-",
        "\u2014": "-",
        "\u2212": "-",
        "\u00a0": " ",
        "\ufeff": "",
    }
    for source, target in replacements.items():
        normalized = normalized.replace(source, target)
    normalized = re.sub(r"\s+", " ", normalized)
    return normalized.strip()


def _compact_match_text(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", _normalize_match_text(text))


# Canonical bus-type families accepted by the generic
# (``expected_bus_type=None``) path of :func:`validate_ir`.
SUPPORTED_BUS_TYPES = frozenset({
    "i2c",
    "spi",
    "uart",
    "gpio",
    "gpio_timing",
    "display_parallel",
    "display_spi",
})


def _bus_type_family(value: object) -> str:
    normalized = _normalize_match_text(str(value or ""))
    if not normalized:
        return ""
    tokens = set(re.findall(r"[a-z0-9]+", normalized))
    if "i2c" in tokens or "smbus" in tokens:
        return "i2c"
    if "gpio" in tokens and ("timing" in tokens or "pulse" in tokens):
        return "gpio_timing"
    if "display" in tokens and "parallel" in tokens:
        return "display_parallel"
    if "display" in tokens and "spi" in tokens:
        return "display_spi"
    # Canonicalize the three non-i2c families the routing table cares about.
    # These checks come after i2c/gpio_timing so mixed tokens like
    # ``"i2c/spi"`` still map to ``"i2c"``.
    if "spi" in tokens:
        return "spi"
    if "uart" in tokens or "serial" in tokens:
        return "uart"
    if (
        "gpio" in tokens
        or "1wire" in tokens
        or "onewire" in tokens
        # Hyphenated aliases like "1-wire" / "one-wire" tokenize into
        # {"1","wire"} / {"one","wire"} after the [a-z0-9]+ split.
        or ("wire" in tokens and ("1" in tokens or "one" in tokens))
    ):
        return "gpio"
    return normalized


def _bus_types_match(expected: object, actual: object) -> bool:
    expected_family = _bus_type_family(expected)
    actual_family = _bus_type_family(actual)
    if not expected_family or not actual_family:
        return False
    return expected_family == actual_family


def _match_tokens(text: str) -> list[str]:
    return re.findall(r"0x[0-9a-f]+|[a-z0-9_]+", _normalize_match_text(text))


PINOUT_TOKENS = frozenset({
    "sda",
    "scl",
    "gnd",
    "vcc",
    "vdd",
    "vss",
    "os",
    "int",
    "irq",
    "alert",
    "reset",
    "rst",
    "xshut",
    "trig",
    "echo",
    "cs",
    "clk",
    "sck",
    "mosi",
    "miso",
    "sdo",
    "sdi",
    "sdio",
    "a0",
    "a1",
    "a2",
    "a3",
    "ad0",
    "ad1",
    "addr",
})

REGISTER_STOPWORDS = frozenset({
    "register",
    "registers",
    "read",
    "write",
    "value",
    "values",
    "address",
    "addresses",
    "pointer",
    "page",
    "pages",
    "and",
    "or",
    "the",
    "to",
    "of",
})


def _informative_tokens(text: str) -> list[str]:
    tokens = _match_tokens(text)
    informative: list[str] = []
    for token in tokens:
        if token.startswith("0x"):
            informative.append(token)
            continue
        if len(token) >= 4:
            informative.append(token)
            continue
        if any(ch.isdigit() for ch in token) and len(token) >= 3:
            informative.append(token)
    return informative


def _split_snippet_candidates(snippet: str) -> list[str]:
    parts = [snippet]
    parts.extend(re.split(r"\s*(?:\.{3}|…)\s*", snippet))
    parts.extend(re.split(r"\s*[.;]\s*", snippet))
    parts.extend(line.strip() for line in snippet.splitlines())
    return [part.strip() for part in parts if part.strip()]


def _resolve_page_record(source: object, page: int) -> dict:
    if isinstance(source, dict):
        if "pages" in source and isinstance(source.get("pages"), dict):
            page_record = source["pages"].get(page, {})
            if isinstance(page_record, dict):
                return {
                    "text": str(page_record.get("text", "") or ""),
                    "lines": [str(line) for line in page_record.get("lines", []) if str(line).strip()],
                    "blocks": [str(block) for block in page_record.get("blocks", []) if str(block).strip()],
                    "section_headings": [
                        str(heading) for heading in page_record.get("section_headings", []) if str(heading).strip()
                    ],
                    "section_summaries": [
                        str(summary) for summary in page_record.get("section_summaries", []) if str(summary).strip()
                    ],
                    "table_texts": [
                        str(table) for table in page_record.get("table_texts", []) if str(table).strip()
                    ],
                    "element_texts": [
                        str(text) for text in page_record.get("element_texts", []) if str(text).strip()
                    ],
                }
        page_text = source.get(page, "")
        if isinstance(page_text, str):
            lines = [line.strip() for line in page_text.splitlines() if line.strip()]
            blocks = [block.strip() for block in re.split(r"\n\s*\n", page_text) if block.strip()]
            return {
                "text": page_text,
                "lines": lines,
                "blocks": blocks,
                "section_headings": [],
                "section_summaries": [],
                "table_texts": [],
                "element_texts": [],
            }
    if isinstance(source, str):
        lines = [line.strip() for line in source.splitlines() if line.strip()]
        blocks = [block.strip() for block in re.split(r"\n\s*\n", source) if block.strip()]
        return {
            "text": source,
            "lines": lines,
            "blocks": blocks,
            "section_headings": [],
            "section_summaries": [],
            "table_texts": [],
            "element_texts": [],
        }
    return {
        "text": "",
        "lines": [],
        "blocks": [],
        "section_headings": [],
        "section_summaries": [],
        "table_texts": [],
        "element_texts": [],
    }


def _resolve_source_for_evidence(source_lookup: dict, source_id: str):
    source = source_lookup.get(source_id)
    if source is not None:
        return source

    paged_sources = [value for value in source_lookup.values() if isinstance(value, dict)]
    if len(paged_sources) == 1:
        return paged_sources[0]
    return None


def _candidate_sequences(page_record: dict) -> list[str]:
    sequences: list[str] = []
    seen: set[str] = set()

    def _append(text: str) -> None:
        normalized = _normalize_match_text(text)
        if not normalized or normalized in seen:
            return
        seen.add(normalized)
        sequences.append(text)

    _append(page_record.get("text", ""))
    for heading in page_record.get("section_headings", []):
        _append(heading)
    for summary in page_record.get("section_summaries", []):
        _append(summary)
    for block in page_record.get("blocks", []):
        _append(block)
    lines = [line for line in page_record.get("lines", []) if line]
    for line in lines:
        _append(line)
    for text in page_record.get("element_texts", []):
        _append(text)
    for table_text in page_record.get("table_texts", []):
        _append(table_text)
    for size in (2, 3, 4):
        for index in range(0, len(lines) - size + 1):
            _append(" ".join(lines[index : index + size]))
    return sequences


def _match_substring_like(haystack: str, candidate: str) -> bool:
    haystack_norm = _normalize_match_text(haystack)
    candidate_norm = _normalize_match_text(candidate)
    if not candidate_norm:
        return False
    if candidate_norm in haystack_norm:
        return True

    candidate_compact = _compact_match_text(candidate)
    haystack_compact = _compact_match_text(haystack)
    if candidate_compact and candidate_compact in haystack_compact:
        return True
    return False


def _candidate_matches(haystack: str, candidate: str) -> bool:
    if _match_substring_like(haystack, candidate):
        return True

    candidate_tokens = _informative_tokens(candidate)
    if len(candidate_tokens) < 3:
        return False
    haystack_tokens = set(_match_tokens(haystack))
    if not haystack_tokens:
        return False
    overlap = [token for token in candidate_tokens if token in haystack_tokens]
    coverage = len(overlap) / len(candidate_tokens)
    required_coverage = 0.85 if len(candidate_tokens) <= 4 else 0.75
    return coverage >= required_coverage


def _match_paragraph_evidence(page_record: dict, candidate: str, allow_short_candidates: bool) -> bool:
    if not allow_short_candidates and len(_normalize_match_text(candidate)) < 12:
        return False
    return any(_candidate_matches(sequence, candidate) for sequence in _candidate_sequences(page_record))


def _looks_like_list_snippet(candidate: str) -> bool:
    normalized = _normalize_match_text(candidate)
    return ("," in candidate or ";" in candidate or "\n" in candidate) and len(_match_tokens(normalized)) >= 4


def _looks_like_pinout_snippet(candidate: str) -> bool:
    normalized = _normalize_match_text(candidate)
    tokens = set(_match_tokens(normalized))
    pin_hits = len(tokens & PINOUT_TOKENS)
    return (
        "pin" in normalized
        or "pinout" in normalized
        or "pin configuration" in normalized
        or "pin description" in normalized
        or pin_hits >= 4
    )


def _looks_like_register_snippet(candidate: str) -> bool:
    normalized = _normalize_match_text(candidate)
    tokens = _match_tokens(normalized)
    hex_tokens = [token for token in tokens if token.startswith("0x")]
    return "register" in normalized and bool(hex_tokens)


def _list_items(candidate: str) -> list[str]:
    list_text = candidate.split(":", 1)[1] if ":" in candidate else candidate
    items = [
        item.strip()
        for item in re.split(r"\s*(?:,|;|\n|\|)\s*", list_text)
        if item.strip()
    ]
    return items


def _match_list_evidence(page_record: dict, candidate: str) -> bool:
    items = _list_items(candidate)
    if len(items) < 3:
        return False
    sequences = _candidate_sequences(page_record)
    page_token_set = set(_match_tokens(page_record.get("text", "")))
    matched = 0
    for item in items:
        item_norm = _normalize_match_text(item)
        if not item_norm:
            continue
        if any(_match_substring_like(sequence, item) for sequence in sequences):
            matched += 1
            continue
        item_tokens = _match_tokens(item)
        if item_tokens and all(token in page_token_set for token in item_tokens):
            matched += 1
    return (matched / len(items)) >= 0.8


def _match_pinout_evidence(page_record: dict, candidate: str) -> bool:
    page_text = page_record.get("text", "")
    page_tokens = set(_match_tokens(page_text))
    candidate_tokens = _match_tokens(candidate)
    pin_tokens = [token for token in candidate_tokens if token in PINOUT_TOKENS]
    if len(pin_tokens) < 4:
        return False
    anchor_ok = any(
        anchor in _normalize_match_text(page_text)
        for anchor in ("pin", "pinout", "configuration", "description")
    )
    if not anchor_ok:
        return False
    coverage = len([token for token in pin_tokens if token in page_tokens]) / len(pin_tokens)
    if coverage >= 0.75:
        return True
    return _match_list_evidence(page_record, candidate)


def _match_register_evidence(page_record: dict, candidate: str) -> bool:
    page_text = page_record.get("text", "")
    page_tokens = set(_match_tokens(page_text))
    candidate_tokens = _match_tokens(candidate)
    hex_tokens = [token for token in candidate_tokens if token.startswith("0x")]
    if not hex_tokens:
        return False
    name_tokens = [
        token
        for token in candidate_tokens
        if not token.startswith("0x") and token not in REGISTER_STOPWORDS and len(token) >= 2
    ]
    hex_coverage = len([token for token in hex_tokens if token in page_tokens]) / len(hex_tokens)
    name_coverage = 0.0
    if name_tokens:
        name_coverage = len([token for token in name_tokens if token in page_tokens]) / len(name_tokens)
    if hex_coverage >= 1.0 and (not name_tokens or name_coverage >= 0.5):
        return True
    if _match_list_evidence(page_record, candidate):
        return True
    return any(_candidate_matches(sequence, candidate) for sequence in _candidate_sequences(page_record))


def _snippet_exists(source_lookup: dict, source_id: str, page: int, snippet: str) -> bool:
    source = _resolve_source_for_evidence(source_lookup, source_id)
    if source is None:
        return False
    allow_short_candidates = "..." not in snippet and "…" not in snippet
    page_record = _resolve_page_record(source, page)
    for candidate in _split_snippet_candidates(snippet):
        if _match_paragraph_evidence(page_record, candidate, allow_short_candidates):
            return True
        if _looks_like_pinout_snippet(candidate) and _match_pinout_evidence(page_record, candidate):
            return True
        if _looks_like_register_snippet(candidate) and _match_register_evidence(page_record, candidate):
            return True
        if _looks_like_list_snippet(candidate) and _match_list_evidence(page_record, candidate):
            return True
    return False


def _source_total_pages(source_lookup: dict, source_id: str) -> Optional[int]:
    """Return the datasheet ``total_pages`` from ``build_source_lookup`` when available."""
    source = _resolve_source_for_evidence(source_lookup, source_id)
    if source is None or not isinstance(source, dict):
        return None
    total = source.get("total_pages")
    if isinstance(total, int) and not isinstance(total, bool) and total > 0:
        return total
    return None


def _validate_evidence_spans(target: dict, source_lookup: dict, issues: List[ValidationIssue], prefix: str) -> None:
    spans = target.get("evidence_spans", [])
    for index, span in enumerate(spans):
        source_id = span.get("source_id")
        page = span.get("page")
        snippet = span.get("snippet", "")
        total = _source_total_pages(source_lookup, source_id) if source_id else None
        if isinstance(page, int) and not isinstance(page, bool):
            if page < 1:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{prefix}.evidence_{index}.page",
                        f"Evidence page {page} is not a positive integer (source '{source_id}').",
                    )
                )
                continue
            if total is not None and page > total:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{prefix}.evidence_{index}.page",
                        f"Evidence page {page} is out of range (total_pages={total}) for source '{source_id}'.",
                    )
                )
                continue
        if not _snippet_exists(source_lookup, source_id, page, snippet):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{prefix}.evidence_{index}",
                    f"Evidence snippet not found: {source_id} page {page} -> {snippet}",
                )
            )


def _canonical_rtos_id(value: Optional[str]) -> str:
    """Delegate to :func:`drivergen.rtos.aliases.canonicalize_rtos_id`."""
    from ..rtos.aliases import canonicalize_rtos_id

    return canonicalize_rtos_id(value)


def _validate_register_map(device_ir: dict, issues: List[ValidationIssue]) -> None:
    """Validate the optional register map shape."""
    if "register_map" not in device_ir:
        return
    register_map = device_ir.get("register_map")
    if register_map is None:
        return
    if not isinstance(register_map, dict):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.register_map",
                f"register_map must be a mapping or null, got {type(register_map).__name__}.",
            )
        )
        return
    total = register_map.get("total_size_bytes")
    if not isinstance(total, int) or isinstance(total, bool) or total < 1:
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.register_map.total_size_bytes",
                f"total_size_bytes must be a positive integer, got {total!r}.",
            )
        )
    addressing = register_map.get("addressing")
    if addressing not in _REGISTER_MAP_ADDRESSING:
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.register_map.addressing",
                f"addressing must be one of {sorted(_REGISTER_MAP_ADDRESSING)}, got {addressing!r}.",
            )
        )
    if not isinstance(register_map.get("auto_increment"), bool):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.register_map.auto_increment",
                "auto_increment must be true or false.",
            )
        )


def _validate_read_channels(device_ir: dict, issues: List[ValidationIssue]) -> None:
    """Validate semantic read channels."""
    if "read_channels" not in device_ir:
        return
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list) or not channels:
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.read_channels",
                "read_channels must be a non-empty list (at least one physical output).",
            )
        )
        return
    for idx, channel in enumerate(channels):
        if not isinstance(channel, dict):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}]",
                    "channel entry must be an object.",
                )
            )
            continue
        cid = channel.get("id")
        if not isinstance(cid, str) or not cid.strip():
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].id",
                    "channel id must be a non-empty string.",
                )
            )
        raw_type = channel.get("raw_type")
        if raw_type not in _READ_CHANNEL_RAW_TYPES:
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].raw_type",
                    f"raw_type must be one of {sorted(_READ_CHANNEL_RAW_TYPES)}, got {raw_type!r}.",
                )
            )
        unit = channel.get("physical_unit")
        if not isinstance(unit, str) or not unit.strip():
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].physical_unit",
                    "physical_unit must be a non-empty string.",
                )
            )
        flow_id = channel.get("flow_id")
        if flow_id is not None and (
            not isinstance(flow_id, str) or not flow_id.strip()
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].flow_id",
                    "flow_id must be a non-empty string or null.",
                )
            )
        source_bytes = channel.get("source_bytes")
        if source_bytes is not None:
            if not isinstance(source_bytes, list):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.read_channels[{idx}].source_bytes",
                        "source_bytes must be a list of strings or null.",
                    )
                )
            else:
                for pos, entry in enumerate(source_bytes):
                    if not isinstance(entry, str) or not entry.strip():
                        issues.append(
                            ValidationIssue(
                                "error",
                                f"device_ir.read_channels[{idx}].source_bytes[{pos}]",
                                "source byte entries must be non-empty strings.",
                            )
                        )
        source_signal = channel.get("source_signal")
        if source_signal is not None and (
            not isinstance(source_signal, str) or not source_signal.strip()
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].source_signal",
                    "source_signal must be a non-empty string or null.",
                )
            )
        formula_id = channel.get("formula_id")
        if formula_id is not None and (
            not isinstance(formula_id, str) or not formula_id.strip()
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.read_channels[{idx}].formula_id",
                    "formula_id must be a non-empty string or null.",
                )
            )


def _validate_raw_encoding(device_ir: dict, issues: List[ValidationIssue]) -> None:
    """Validate the bit-level layout used to assemble multi-byte integers."""
    if "raw_encoding" not in device_ir:
        return
    encoding = device_ir.get("raw_encoding")
    if encoding is None:
        return
    if not isinstance(encoding, dict):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.raw_encoding",
                f"raw_encoding must be an object or null, got {type(encoding).__name__}.",
            )
        )
        return
    byte_order = encoding.get("byte_order")
    if byte_order not in _RAW_ENCODING_BYTE_ORDER:
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.raw_encoding.byte_order",
                f"byte_order must be one of {sorted(_RAW_ENCODING_BYTE_ORDER)}, got {byte_order!r}.",
            )
        )
    bit_width = encoding.get("bit_width")
    if (
        not isinstance(bit_width, int)
        or isinstance(bit_width, bool)
        or not (1 <= bit_width <= 64)
    ):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.raw_encoding.bit_width",
                f"bit_width must be an integer in [1,64], got {bit_width!r}.",
            )
        )
    if not isinstance(encoding.get("signed"), bool):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.raw_encoding.signed",
                "signed must be true or false.",
            )
        )


def _validate_transaction_field(
    transaction: object,
    issues: List[ValidationIssue],
    base_path: str,
) -> None:
    """IR-E - shape check for a single ``transaction`` sub-object on a ``read_sequence`` / ``init_sequence`` step."""
    if transaction is None:
        return
    if not isinstance(transaction, dict):
        issues.append(
            ValidationIssue(
                "error",
                base_path,
                f"transaction must be an object or null, got {type(transaction).__name__}.",
            )
        )
        return

    kind = transaction.get("kind")
    if kind not in _SEQUENCE_TRANSACTION_KINDS:
        issues.append(
            ValidationIssue(
                "error",
                f"{base_path}.kind",
                (
                    f"kind must be one of {sorted(_SEQUENCE_TRANSACTION_KINDS)}, "
                    f"got {kind!r}."
                ),
            )
        )

    extra_keys = set(transaction.keys()) - {
        "kind",
        "bytes",
        "length",
        "pointer_target",
        "notes",
    }
    if extra_keys:
        issues.append(
            ValidationIssue(
                "error",
                base_path,
                (
                    "transaction object has unknown keys "
                    f"{sorted(extra_keys)}; allowed: kind|bytes|length|pointer_target|notes."
                ),
            )
        )

    if "bytes" in transaction and transaction["bytes"] is not None:
        byte_list = transaction["bytes"]
        if not isinstance(byte_list, list):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base_path}.bytes",
                    (
                        "bytes must be a list of int (0..255), hex-literal string "
                        "(\"0xNN\"), \"DATA\" placeholder, or null; got "
                        f"{type(byte_list).__name__}."
                    ),
                )
            )
        else:
            for pos, entry in enumerate(byte_list):
                if entry is None:
                    continue
                if isinstance(entry, bool):
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"{base_path}.bytes[{pos}]",
                            f"bytes entry cannot be a bool (got {entry!r}).",
                        )
                    )
                    continue
                if isinstance(entry, int):
                    if not 0 <= entry <= 255:
                        issues.append(
                            ValidationIssue(
                                "error",
                                f"{base_path}.bytes[{pos}]",
                                f"bytes integer must be in [0,255], got {entry}.",
                            )
                        )
                    continue
                if isinstance(entry, str):
                    if entry == "DATA":
                        continue
                    if _TRANSACTION_BYTES_LITERAL_RE.match(entry):
                        continue
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"{base_path}.bytes[{pos}]",
                            (
                                "bytes string entry must be \"DATA\" or a hex "
                                f"literal like \"0x1F\", got {entry!r}."
                            ),
                        )
                    )
                    continue
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{base_path}.bytes[{pos}]",
                        (
                            "bytes entry must be int (0..255), str, or null; "
                            f"got {type(entry).__name__}."
                        ),
                    )
                )

    length = transaction.get("length")
    if length is not None:
        if length == "DATA":
            pass
        elif isinstance(length, bool) or not isinstance(length, int):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base_path}.length",
                    f"length must be a positive integer or null, got {length!r}.",
                )
            )
        elif not 1 <= length <= 65535:
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base_path}.length",
                    f"length must be in [1,65535], got {length}.",
                )
            )

    # Cross-field check: a "read" step without length is meaningless.
    if kind in {"read", "write_then_read"} and length is None:
        issues.append(
            ValidationIssue(
                "error",
                f"{base_path}.length",
                f"transaction kind={kind!r} requires a non-null length.",
            )
        )

    # Cross-field check: "write" / "write_then_read" must list something to
    # write. Empty ``bytes: []`` is suspicious and treated as an error.
    if kind in {"write", "write_then_read"}:
        byte_list = transaction.get("bytes")
        if byte_list is None:
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base_path}.bytes",
                    f"transaction kind={kind!r} requires a non-null bytes list.",
                )
            )
        elif isinstance(byte_list, list) and not byte_list:
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base_path}.bytes",
                    f"transaction kind={kind!r} requires a non-empty bytes list.",
                )
            )


def _validate_sequence_transactions(
    device_ir: dict, issues: List[ValidationIssue]
) -> None:
    """IR-E - enforce shape on any ``transaction`` payload attached to ``init_sequence`` / ``read_sequence`` steps."""
    for seq_key in ("init_sequence", "read_sequence"):
        sequence = device_ir.get(seq_key)
        if not isinstance(sequence, list):
            continue
        for idx, step in enumerate(sequence):
            if not isinstance(step, dict):
                continue
            if "transaction" not in step:
                continue
            _validate_transaction_field(
                step["transaction"],
                issues,
                f"device_ir.{seq_key}[{idx}].transaction",
            )


def _validate_access_model(device_ir: dict, issues: List[ValidationIssue]) -> None:
    access_model = device_ir.get("access_model")
    if access_model is None:
        return
    if not isinstance(access_model, dict):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.access_model",
                f"access_model must be an object or null, got {type(access_model).__name__}.",
            )
        )
        return
    kind = access_model.get("kind")
    if kind not in _ACCESS_MODEL_KINDS:
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.access_model.kind",
                f"kind must be one of {sorted(_ACCESS_MODEL_KINDS)}, got {kind!r}.",
            )
        )
    for key in ("read_requires_pointer", "direct_read_after_write"):
        value = access_model.get(key)
        if value is not None and not isinstance(value, bool):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.access_model.{key}",
                    f"{key} must be true, false, or null.",
                )
            )
    address_bytes = access_model.get("address_bytes")
    if address_bytes is not None:
        if (
            isinstance(address_bytes, bool)
            or not isinstance(address_bytes, int)
            or not (0 <= address_bytes <= 4)
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    "device_ir.access_model.address_bytes",
                    "address_bytes must be an integer in [0,4] or null.",
                )
            )


def _channel_ids(device_ir: dict) -> set[str]:
    out: set[str] = set()
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return out
    for channel in channels:
        if isinstance(channel, dict):
            cid = channel.get("id")
            if isinstance(cid, str) and cid.strip():
                out.add(cid.strip())
    return out


def _transaction_kind(transaction: object) -> Optional[str]:
    if isinstance(transaction, dict):
        kind = transaction.get("kind")
        if isinstance(kind, str):
            return kind
    return None


def _coerce_byte_int(value: object) -> Optional[int]:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value if 0 <= value <= 255 else None
    if isinstance(value, str):
        text = value.strip()
        if not text or text == "DATA":
            return None
        try:
            if text.lower().startswith("0x"):
                parsed = int(text, 16)
            elif text.lower().startswith("0b"):
                parsed = int(text, 2)
            else:
                parsed = int(text, 0)
        except ValueError:
            return None
        return parsed if 0 <= parsed <= 255 else None
    return None


def _i2c_logical_addresses(device_ir: dict) -> set[int]:
    rule = device_ir.get("address_rule")
    if not isinstance(rule, dict):
        return set()
    values: list[object] = []
    raw_addresses = rule.get("addresses")
    if isinstance(raw_addresses, list):
        for item in raw_addresses:
            if isinstance(item, dict):
                values.extend((item.get("address"), item.get("value"), item.get("default")))
            else:
                values.append(item)
    values.extend((rule.get("address"), rule.get("value"), rule.get("default")))
    out: set[int] = set()
    for value in values:
        parsed = _coerce_byte_int(value)
        if parsed is not None and 0 <= parsed <= 0x7F:
            out.add(parsed)
    return out


def _i2c_wire_address_bytes(device_ir: dict) -> set[int]:
    out: set[int] = set()
    for address in _i2c_logical_addresses(device_ir):
        out.add((address << 1) & 0xFE)
        out.add(((address << 1) | 1) & 0xFF)
    return out


def _validate_i2c_payload_has_no_leading_address(
    device_ir: dict,
    transaction: object,
    issues: List[ValidationIssue],
    base_path: str,
) -> None:
    if _bus_type_family(device_ir.get("bus_type")) != "i2c":
        return
    if not isinstance(transaction, dict):
        return
    if transaction.get("kind") not in {"write", "write_then_read"}:
        return
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list) or len(bytes_value) <= 1:
        return
    first = _coerce_byte_int(bytes_value[0])
    if first is None:
        return
    if first not in _i2c_logical_addresses(device_ir) and first not in _i2c_wire_address_bytes(device_ir):
        return
    issues.append(
        ValidationIssue(
            "error",
            f"{base_path}.bytes[0]",
            (
                "I2C transaction.bytes appears to start with the slave address "
                f"{bytes_value[0]!r}. transaction.bytes must contain only "
                "register pointer, command, memory-address, or payload bytes; "
                "the 7-bit/8-bit address is handled by address_rule and the bus API."
            ),
        )
    )


def _validate_operation_flows(device_ir: dict, issues: List[ValidationIssue]) -> None:
    flows = device_ir.get("operation_flows")
    if flows is None:
        return
    if not isinstance(flows, list):
        issues.append(
            ValidationIssue(
                "error",
                "device_ir.operation_flows",
                f"operation_flows must be a list when present, got {type(flows).__name__}.",
            )
        )
        return

    channel_ids = _channel_ids(device_ir)
    flow_ids: set[str] = set()
    covered_channels: set[str] = set()

    for flow_index, flow in enumerate(flows):
        base = f"device_ir.operation_flows[{flow_index}]"
        if not isinstance(flow, dict):
            issues.append(
                ValidationIssue(
                    "error",
                    base,
                    "flow entry must be an object.",
                )
            )
            continue

        flow_id = flow.get("flow_id")
        if not isinstance(flow_id, str) or not flow_id.strip():
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.flow_id",
                    "flow_id must be a non-empty string.",
                )
            )
        else:
            flow_id = flow_id.strip()
            if flow_id in flow_ids:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{base}.flow_id",
                        f"duplicate flow_id {flow_id!r}.",
                    )
                )
            flow_ids.add(flow_id)

        kind = flow.get("kind")
        if kind not in _OPERATION_FLOW_KINDS:
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.kind",
                    f"kind must be one of {sorted(_OPERATION_FLOW_KINDS)}, got {kind!r}.",
                )
            )

        channels = flow.get("channels")
        if channels is None:
            channels = []
        if not isinstance(channels, list):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.channels",
                    "channels must be a list of channel ids.",
                )
            )
            channels = []
        for pos, ch in enumerate(channels):
            if not isinstance(ch, str) or not ch.strip():
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{base}.channels[{pos}]",
                        "channel references must be non-empty strings.",
                    )
                )
                continue
            ch = ch.strip()
            if channel_ids and ch not in channel_ids:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{base}.channels[{pos}]",
                        f"flow references unknown read_channel {ch!r}.",
                    )
                )

        steps = flow.get("steps")
        if not isinstance(steps, list):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.steps",
                    "steps must be a list.",
                )
            )
            steps = []

        has_producer_step = False
        for step_index, step in enumerate(steps):
            step_base = f"{base}.steps[{step_index}]"
            if not isinstance(step, dict):
                issues.append(
                    ValidationIssue("error", step_base, "step must be an object.")
                )
                continue
            op = step.get("op")
            if op not in _OPERATION_FLOW_STEP_OPS:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{step_base}.op",
                        f"op must be one of {sorted(_OPERATION_FLOW_STEP_OPS)}, got {op!r}.",
                    )
                )
            if "transaction" in step:
                _validate_transaction_field(
                    step.get("transaction"), issues, f"{step_base}.transaction"
                )
                _validate_i2c_payload_has_no_leading_address(
                    device_ir,
                    step.get("transaction"),
                    issues,
                    f"{step_base}.transaction",
                )
            transaction = step.get("transaction")
            if op in _BUS_STEP_OPS:
                if transaction is None:
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"{step_base}.transaction",
                            f"operation flow bus step op={op!r} requires a non-null transaction.",
                        )
                    )
                else:
                    has_producer_step = True
                    tx_kind = _transaction_kind(transaction)
                    if tx_kind is not None and tx_kind != op:
                        issues.append(
                            ValidationIssue(
                                "error",
                                f"{step_base}.transaction.kind",
                                f"transaction kind {tx_kind!r} must match flow step op {op!r}.",
                            )
                        )
            elif op in {"poll_until", "clear", "select_page"}:
                if transaction is not None:
                    has_producer_step = True
                elif not step.get("register"):
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"{step_base}.register",
                            f"step op={op!r} requires either transaction or register.",
                        )
                    )
            elif op in _SIGNAL_STEP_OPS:
                if not _step_has_signal_reference(step):
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"{step_base}.signal",
                            f"signal step op={op!r} requires signal, source_signal, output_ref, or condition.",
                        )
                    )
                else:
                    has_producer_step = True

        outputs = flow.get("outputs")
        if not isinstance(outputs, list):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.outputs",
                    "outputs must be a list.",
                )
            )
            outputs = []
        for output_index, output in enumerate(outputs):
            out_base = f"{base}.outputs[{output_index}]"
            if not isinstance(output, dict):
                issues.append(
                    ValidationIssue("error", out_base, "output must be an object.")
                )
                continue
            ch = output.get("channel")
            if not isinstance(ch, str) or not ch.strip():
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{out_base}.channel",
                        "output channel must be a non-empty string.",
                    )
                )
                continue
            ch = ch.strip()
            covered_channels.add(ch)
            if channel_ids and ch not in channel_ids:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{out_base}.channel",
                        f"flow output references unknown read_channel {ch!r}.",
                    )
                )
            byte_source = output.get("byte_source")
            if byte_source is not None and (
                not isinstance(byte_source, str) or not byte_source.strip()
            ):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{out_base}.byte_source",
                        "byte_source must be a non-empty string or null.",
                    )
                )
            source_signal = output.get("source_signal")
            if source_signal is not None and (
                not isinstance(source_signal, str) or not source_signal.strip()
            ):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{out_base}.source_signal",
                        "source_signal must be a non-empty string or null.",
                    )
                )

        requires_human = flow.get("requires_human")
        if requires_human is not None and not isinstance(requires_human, bool):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.requires_human",
                    "requires_human must be true, false, or null.",
                )
            )
        if (
            kind in {"probe", "read", "calibration"}
            and not requires_human
            and not has_producer_step
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.steps",
                    f"flow kind={kind!r} must contain at least one bus/signal-producing step or set requires_human=true.",
                )
            )
        if (
            kind == "init"
            and not requires_human
            and not has_producer_step
            and not _flow_declares_default_only_init(flow)
        ):
            issues.append(
                ValidationIssue(
                    "error",
                    f"{base}.steps",
                    "init flow must contain at least one bus/signal-producing "
                    "step, explicitly state default/no-bus initialization, or "
                    "set requires_human=true.",
                )
            )
        if kind == "read" and not requires_human and channels:
            output_channels = {
                output.get("channel").strip()
                for output in outputs
                if isinstance(output, dict)
                and isinstance(output.get("channel"), str)
                and output.get("channel").strip()
            }
            missing_outputs = [
                ch.strip()
                for ch in channels
                if isinstance(ch, str)
                and ch.strip()
                and ch.strip() not in output_channels
            ]
            if missing_outputs:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"{base}.outputs",
                        "read flow declares channel(s) but does not output them: "
                        f"{missing_outputs}. Use kind='write'/'other' for a "
                        "command-only start flow, or add concrete outputs.",
                    )
                )

    if flows:
        for channel_index, channel in enumerate(device_ir.get("read_channels") or []):
            if not isinstance(channel, dict):
                continue
            cid = channel.get("id")
            if not isinstance(cid, str) or not cid.strip():
                continue
            cid = cid.strip()
            flow_id = channel.get("flow_id")
            if isinstance(flow_id, str) and flow_id.strip():
                if flow_id.strip() not in flow_ids:
                    issues.append(
                        ValidationIssue(
                            "error",
                            f"device_ir.read_channels[{channel_index}].flow_id",
                            f"flow_id {flow_id!r} does not resolve to operation_flows[*].flow_id.",
                        )
                    )
                covered_channels.add(cid)
            if cid not in covered_channels:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.read_channels[{channel_index}]",
                        f"read_channel {cid!r} is not covered by any operation flow output.",
                    )
                )


def _step_has_signal_reference(step: dict) -> bool:
    for key in ("signal", "source_signal", "output_ref", "condition"):
        value = step.get(key)
        if isinstance(value, str) and value.strip():
            return True
    return False


def _flow_declares_default_only_init(flow: dict) -> bool:
    text_parts: list[str] = []
    for key in ("flow_id", "notes"):
        text_parts.append(str(flow.get(key) or ""))
    for step in flow.get("steps") or []:
        if isinstance(step, dict):
            text_parts.append(str(step.get("notes") or ""))
            text_parts.append(str(step.get("role") or ""))
            text_parts.append(str(step.get("condition") or ""))
    return bool(_DEFAULT_ONLY_INIT_RE.search(" ".join(text_parts)))


def _validate_conversion_formulae_expressions(
    device_ir: dict, issues: List[ValidationIssue]
) -> None:
    """Validate machine-usable conversion expressions."""
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, list):
        return
    channel_formula_units = _read_channel_formula_units(device_ir)
    for idx, row in enumerate(formulae):
        if not isinstance(row, dict):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}]",
                    "row must be an object.",
                )
            )
            continue
        if "integer_approximation_expression" not in row:
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression",
                    "field is required (use null when no closed-form exists).",
                )
            )
            continue
        expr_obj = row.get("integer_approximation_expression")
        formula_name = row.get("name")
        formula_key = str(formula_name).strip() if isinstance(formula_name, str) else ""
        if expr_obj is None:
            if formula_key in channel_formula_units and not _formula_declares_complex_non_executable(row):
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.conversion_formulae[{idx}].integer_approximation_expression",
                        (
                            f"formula {formula_key!r} is referenced by read_channels "
                            "but has null integer_approximation_expression; codegen "
                            "cannot derive executable expected values."
                        ),
                    )
                )
            continue
        if not isinstance(expr_obj, dict):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression",
                    f"must be an object or null, got {type(expr_obj).__name__}.",
                )
            )
            continue
        for shape_issue in validate_expression_object(expr_obj):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression",
                    shape_issue,
                )
            )
        expr_str = str(expr_obj.get("expression") or "").strip()
        input_names = _expression_input_names(expr_obj)
        if input_names and _is_placeholder_zero_expression(expr_str):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression.expression",
                    (
                        "expression is a placeholder zero despite declaring "
                        f"inputs {sorted(input_names)}; use the datasheet formula "
                        "or set the expression to null with requires_human."
                    ),
                )
            )
        elif input_names and not _expression_mentions_any_input(expr_str, input_names):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression.expression",
                    (
                        "expression does not reference any declared input; "
                        f"declared inputs are {sorted(input_names)}."
                    ),
                )
            )
        if _looks_like_unscaled_milli_unit_expression(row, expr_obj, expr_str):
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.conversion_formulae[{idx}].integer_approximation_expression.expression",
                    (
                        "expression output is labelled as a milli-unit but appears "
                        "to return an unscaled raw/count value. Include the "
                        "datasheet scale factor in the integer expression, or use "
                        "null with requires_human for a complex compensation algorithm."
                    ),
                )
            )
        expected_units = channel_formula_units.get(formula_key, set())
        output = expr_obj.get("output")
        output_unit = ""
        if isinstance(output, dict):
            raw_unit = output.get("unit")
            if isinstance(raw_unit, str):
                output_unit = raw_unit
        if expected_units and output_unit:
            norm_output = _normalise_unit(output_unit)
            norm_expected = {_normalise_unit(unit) for unit in expected_units if unit}
            if norm_output and norm_expected and norm_output not in norm_expected:
                issues.append(
                    ValidationIssue(
                        "error",
                        f"device_ir.conversion_formulae[{idx}].integer_approximation_expression.output.unit",
                        (
                            f"formula {formula_key!r} outputs unit {output_unit!r}, "
                            f"but read_channels using it declare physical_unit "
                            f"{sorted(expected_units)!r}."
                        ),
                    )
                )


def _read_channel_formula_units(device_ir: dict) -> dict[str, set[str]]:
    out: dict[str, set[str]] = {}
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return out
    for channel in channels:
        if not isinstance(channel, dict):
            continue
        formula_id = channel.get("formula_id")
        unit = channel.get("physical_unit")
        if isinstance(formula_id, str) and formula_id.strip():
            out.setdefault(formula_id.strip(), set())
            if isinstance(unit, str) and unit.strip():
                out[formula_id.strip()].add(unit.strip())
    return out


def _expression_input_names(expr_obj: dict) -> set[str]:
    names: set[str] = set()
    inputs = expr_obj.get("inputs")
    if not isinstance(inputs, list):
        return names
    for item in inputs:
        if isinstance(item, dict) and isinstance(item.get("name"), str):
            name = item["name"].strip()
            if name:
                names.add(name)
    return names


def _is_placeholder_zero_expression(expr_str: str) -> bool:
    return bool(re.fullmatch(r"[+-]?(?:0+(?:\.0*)?|\.0+)", expr_str.strip()))


def _expression_mentions_any_input(expr_str: str, input_names: set[str]) -> bool:
    for name in input_names:
        if re.search(rf"\b{re.escape(name)}\b", expr_str):
            return True
    return False


def _looks_like_unscaled_milli_unit_expression(
    row: dict,
    expr_obj: dict,
    expr_str: str,
) -> bool:
    output = expr_obj.get("output")
    if not isinstance(output, dict):
        return False
    unit = output.get("unit")
    if not isinstance(unit, str):
        return False
    norm_unit = _normalise_unit(unit)
    if norm_unit not in {"mdegc", "millidegc", "millilx", "milliv", "millipa"}:
        return False
    if not expr_str:
        return False
    if "*" in expr_str or "/" in expr_str:
        return False
    text = _normalize_match_text(
        " ".join(
            str(row.get(key) or "")
            for key in ("name", "formula", "notes", "description")
        )
    )
    if any(token in text for token in ("0.0625", "0.125", "0.25", "0.5", "lsb", "count")):
        return True
    return False


def _formula_declares_complex_non_executable(row: dict) -> bool:
    status = str(row.get("executable_expression_status") or "").strip().lower()
    if status in {
        "complex_compensation_algorithm",
        "non_executable_complex_algorithm",
        "requires_driver_algorithm",
    }:
        return True
    text = _normalize_match_text(
        " ".join(
            str(row.get(key) or "")
            for key in ("name", "formula", "notes", "description")
        )
    )
    return (
        "complex compensation" in text
        or "vendor compensation" in text
        or "safe expression evaluator" in text
    )


def _normalise_unit(unit: str) -> str:
    text = unit.strip().lower()
    text = text.replace("degrees", "deg")
    text = text.replace("degree", "deg")
    text = text.replace("celsius", "c")
    text = re.sub(r"[^a-z0-9]+", "", text)
    text = text.replace("millidegc", "mdegc")
    text = text.replace("millidegreec", "mdegc")
    text = text.replace("micro", "u")
    return text


def validate_ir(
    device_ir: dict,
    source_lookup: dict,
    *,
    expected_bus_type: Optional[str] = None,
) -> ValidationResult:
    """Validate a Device IR document."""
    issues: List[ValidationIssue] = []
    append_missing_field_issues(device_ir, DEVICE_IR_REQUIRED_FIELDS, issues, "device_ir")
    actual_bus_type = device_ir.get("bus_type", "")
    if expected_bus_type is not None:
        if not _bus_types_match(expected_bus_type, actual_bus_type):
            issues.append(
                ValidationIssue(
                    "error",
                    "device_ir.bus_type",
                    (
                        f"Device IR declares bus_type={actual_bus_type!r}, "
                        f"but pipeline expected {expected_bus_type!r}."
                    ),
                )
            )
    else:
        actual_family = _bus_type_family(actual_bus_type)
        if actual_family not in SUPPORTED_BUS_TYPES:
            issues.append(
                ValidationIssue(
                    "error",
                    "device_ir.bus_type",
                    (
                        f"Device IR bus_type={actual_bus_type!r} "
                        f"(canonicalized to {actual_family!r}) is not in "
                        f"SUPPORTED_BUS_TYPES={sorted(SUPPORTED_BUS_TYPES)}."
                    ),
                )
            )
    _validate_register_map(device_ir, issues)
    _validate_read_channels(device_ir, issues)
    _validate_raw_encoding(device_ir, issues)
    _validate_access_model(device_ir, issues)
    _validate_conversion_formulae_expressions(device_ir, issues)
    _validate_sequence_transactions(device_ir, issues)
    _validate_operation_flows(device_ir, issues)
    _validate_no_uncertain_registers(device_ir, issues)
    _validate_evidence_spans(device_ir, source_lookup, issues, "device_ir")
    return ValidationResult(ok=not issues, issues=issues)


def _validate_no_uncertain_registers(
    device_ir: dict, issues: List[ValidationIssue]
) -> None:
    """Reject register rows with explicitly uncertain access values."""
    regs = device_ir.get("registers_or_commands")
    if not isinstance(regs, list):
        return
    bad_markers = {"n/a", "na", "not applicable", "not_applicable",
                   "unknown", "tbd", "?", ""}
    for i, row in enumerate(regs):
        if not isinstance(row, dict):
            continue
        # Common aliases for the access spec.
        rw_raw = (
            row.get("rw")
            or row.get("access")
            or row.get("operation")
            or row.get("op")
            or ""
        )
        rw = str(rw_raw).strip().lower() if rw_raw is not None else ""
        if rw in bad_markers:
            name = str(row.get("name") or row.get("register_name") or f"index_{i}")
            issues.append(
                ValidationIssue(
                    "error",
                    f"device_ir.registers_or_commands[{i}].uncertain_access",
                    (
                        f"Register row {name!r} has uncertain access "
                        f"(rw/access={rw_raw!r}). Drop the row from "
                        f"device_ir or supply a concrete R/W classification "
                        f"grounded in the datasheet."
                    ),
                )
            )


def validate_kernel_profile(kernel_profile: dict, source_lookup: dict, expected_rtos: str, board_context: dict) -> ValidationResult:
    issues: List[ValidationIssue] = []
    append_missing_field_issues(kernel_profile, KERNEL_PROFILE_REQUIRED_FIELDS, issues, "kernel_profile")
    if _canonical_rtos_id(kernel_profile.get("rtos")) != _canonical_rtos_id(expected_rtos):
        issues.append(
            ValidationIssue("error", "kernel_profile.rtos", f"Kernel profile must target '{expected_rtos}'.")
        )
    if kernel_profile.get("board") != board_context.get("board"):
        issues.append(
            ValidationIssue("error", "kernel_profile.board", f"Kernel profile must target board '{board_context.get('board')}'.")
        )
    _validate_evidence_spans(kernel_profile, source_lookup, issues, "kernel_profile")
    return ValidationResult(ok=not issues, issues=issues)


