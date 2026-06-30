from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, Iterable, List


# ---------------------------------------------------------------------------
# Source category and authority constants for TRACE-RTOS typed inventory
# ---------------------------------------------------------------------------

class SourceCategory:
    """Well-known categories for RTOS context artifacts."""
    HEADER = "header"
    SOURCE = "source"
    DOC = "doc"
    BUILD = "build"
    EXEMPLAR = "exemplar"
    CONFIG = "config"


class ArtifactRole:
    """Semantic roles an artifact can play in the context."""
    API_DEFINITION = "api_definition"
    USAGE_DOC = "usage_doc"
    SAMPLE_COMMAND = "sample_command"
    BUS_HELPER = "bus_helper"
    BOARD_BINDING = "board_binding"
    FEATURE_GATE = "feature_gate"
    EXEMPLAR_DRIVER = "exemplar_driver"


# Default authority scores by source category (0.0–1.0).
DEFAULT_AUTHORITY_SCORES: dict[str, float] = {
    SourceCategory.HEADER: 1.0,
    SourceCategory.SOURCE: 0.8,
    SourceCategory.BUILD: 1.0,
    SourceCategory.CONFIG: 0.9,
    SourceCategory.DOC: 0.6,
    SourceCategory.EXEMPLAR: 0.5,
}

# Default token budget allocation by category.
DEFAULT_CATEGORY_BUDGET: dict[str, float] = {
    SourceCategory.HEADER: 0.35,
    SourceCategory.SOURCE: 0.25,
    SourceCategory.DOC: 0.15,
    SourceCategory.BUILD: 0.10,
    SourceCategory.EXEMPLAR: 0.15,
}


@dataclass
class ArtifactRecord:
    """Typed metadata for a single RTOS context artifact."""
    source_id: str
    path: str
    category: str  # SourceCategory.*
    role: str  # ArtifactRole.*
    source_layer: str = "unknown"
    authority_score: float = 0.5
    task_match_score: float = 0.0
    discovery_score: float = 0.0
    discovery_reasons: list[str] = field(default_factory=list)
    matched_needles: list[str] = field(default_factory=list)
    hit_count: int = 0
    line_hits: list[dict] = field(default_factory=list)
    snippet: str = ""

    def to_dict(self) -> dict:
        return {
            "source_id": self.source_id,
            "path": self.path,
            "source_layer": self.source_layer,
            "category": self.category,
            "role": self.role,
            "authority_score": self.authority_score,
            "task_match_score": self.task_match_score,
            "discovery_score": self.discovery_score,
            "discovery_reasons": self.discovery_reasons,
            "matched_needles": self.matched_needles,
            "hit_count": self.hit_count,
            "line_hits": self.line_hits[:10],
            "snippet": self.snippet,
        }


@dataclass
class SymbolRecord:
    """A single extracted symbol from an RTOS source file."""
    symbol_id: str
    name: str
    kind: str  # function, macro, struct, typedef, enum, config_flag
    declared_in: str = ""
    defined_in: str = ""
    signature_text: str = ""
    owner_confidence: float = 1.0
    evidence_spans: list[dict] = field(default_factory=list)

    def to_dict(self) -> dict:
        return {
            "symbol_id": self.symbol_id,
            "name": self.name,
            "kind": self.kind,
            "declared_in": self.declared_in,
            "defined_in": self.defined_in,
            "signature_text": self.signature_text,
            "owner_confidence": self.owner_confidence,
            "evidence_spans": self.evidence_spans,
        }


@dataclass
class PatternRecord:
    """An implementation pattern observed in exemplar or repo code."""
    pattern_id: str
    pattern_name: str  # e.g. register_pointer_then_read
    source_id: str = ""
    trigger_symbols: list[str] = field(default_factory=list)
    semantic_summary: str = ""
    negative_constraints: list[str] = field(default_factory=list)
    confidence: float = 0.5

    def to_dict(self) -> dict:
        return {
            "pattern_id": self.pattern_id,
            "pattern_name": self.pattern_name,
            "source_id": self.source_id,
            "trigger_symbols": self.trigger_symbols,
            "semantic_summary": self.semantic_summary,
            "negative_constraints": self.negative_constraints,
            "confidence": self.confidence,
        }


@dataclass
class AuthorityConflict:
    """Records a conflict between two sources about the same fact."""
    conflict_id: str
    fact_description: str
    preferred_source: str
    preferred_category: str
    losing_source: str
    losing_category: str
    conflict_type: str = "signature_mismatch"
    requires_human: bool = False
    resolution_note: str = ""

    def to_dict(self) -> dict:
        return {
            "conflict_id": self.conflict_id,
            "fact_description": self.fact_description,
            "preferred_source": self.preferred_source,
            "preferred_category": self.preferred_category,
            "losing_source": self.losing_source,
            "losing_category": self.losing_category,
            "conflict_type": self.conflict_type,
            "requires_human": self.requires_human,
            "resolution_note": self.resolution_note,
        }


DEVICE_IR_REQUIRED_FIELDS = [
    "device_id",
    "bus_type",
    "address_rule",
    "register_map",
    "read_channels",
    "raw_encoding",
    "registers_or_commands",
    "bitfields",
    "init_sequence",
    "read_sequence",
    "timing_constraints",
    "conversion_formulae",
    "error_conditions",
    "power_states",
    "evidence_spans",
    "requires_human",
]

KERNEL_PROFILE_REQUIRED_FIELDS = [
    "rtos",
    "board",
    "integration",
    "runtime_symbols",
    "delay_symbols",
    "error_symbols",
    "header_sources",
    "build_sources",
    "notes",
    "evidence_spans",
    "requires_human",
]

DRIVER_FRAMEWORK_PROFILE_REQUIRED_FIELDS = [
    "rtos",
    "board",
    "bus_type",
    "bus_api_symbols",
    "bus_helper_types",
    "transaction_patterns",
    "sample_symbols",
    "framework_sources",
    "exemplar_sources",
    "forbidden_assumptions",
    "notes",
    "evidence_spans",
    "requires_human",
]

BOARD_INTEGRATION_PROFILE_REQUIRED_FIELDS = [
    "rtos",
    "board",
    "integration",
    "connection_type",
    "mode",
    "bus_binding",
    "runtime_assumptions",
    "board_sources",
    "attachment_hints",
    "notes",
    "evidence_spans",
    "requires_human",
]

RTOS_CONTRACT_REQUIRED_FIELDS = [
    "contract_version",
    "task_package_id",
    "device_id",
    "rtos",
    "board",
    "bus_type",
    "connection",
    "runtime_contract",
    "bus_contract",
    "integration_contract",
    "device_contract",
    "allowed_symbols",
    "forbidden_assumptions",
    "notes",
    "evidence_spans",
    "requires_human",
]

@dataclass
class ValidationIssue:
    level: str
    check_id: str
    message: str


@dataclass
class ValidationResult:
    ok: bool
    issues: List[ValidationIssue]

    def to_dict(self) -> Dict[str, object]:
        return {
            "ok": self.ok,
            "issues": [
                {"level": issue.level, "check_id": issue.check_id, "message": issue.message}
                for issue in self.issues
            ],
        }


def append_missing_field_issues(
    target: dict,
    required_fields: Iterable[str],
    issues: List[ValidationIssue],
    prefix: str,
) -> None:
    for field_name in required_fields:
        if field_name not in target:
            issues.append(ValidationIssue("error", f"{prefix}.missing_field", f"Missing field: {field_name}"))
