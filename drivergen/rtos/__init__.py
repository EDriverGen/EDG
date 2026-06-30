"""DriverGen RTOS context pipeline ( MRSE)."""

ARTIFACT_VERSION = "1.0"

from .aliases import canonicalize_rtos_id
from .registry import RtosProfile, get_rtos_profile, list_registered_rtos
from .types import (
    SourceRoot,
    BusIntent,
    TaskSpec,
    SlotGoal,
    SlotPlan,
    SymbolSketch,
    IncludeEdge,
    FileCard,
    DirectoryCard,
    RepoIndexBundle,
    RankedSymbolCandidate,
    EvidenceSpan,
    SymbolBinding,
    SlotCoverageReport,
    RoundRecord,
    ExtractionLedger,
    EvidenceValidationReport,
    RtosEvidenceArtifact,
    SOURCE_KIND_MANIFEST_REPO,
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SOURCE_KIND_STUB,
    VERIFICATION_SOURCE_DECLARED,
    VERIFICATION_CONTRACT_DECLARED,
    VERIFICATION_STUB_ONLY,
)

__all__ = [
    "ARTIFACT_VERSION",
    # alias / registry
    "canonicalize_rtos_id",
    "RtosProfile",
    "get_rtos_profile",
    "list_registered_rtos",
    # dataclasses
    "SourceRoot",
    "BusIntent",
    "TaskSpec",
    "SlotGoal",
    "SlotPlan",
    "SymbolSketch",
    "IncludeEdge",
    "FileCard",
    "DirectoryCard",
    "RepoIndexBundle",
    "RankedSymbolCandidate",
    "EvidenceSpan",
    "SymbolBinding",
    "SlotCoverageReport",
    "RoundRecord",
    "ExtractionLedger",
    "EvidenceValidationReport",
    "RtosEvidenceArtifact",
    # constants
    "SOURCE_KIND_MANIFEST_REPO",
    "SOURCE_KIND_TASK_PACKAGE_HELPER",
    "SOURCE_KIND_STUB",
    "VERIFICATION_SOURCE_DECLARED",
    "VERIFICATION_CONTRACT_DECLARED",
    "VERIFICATION_STUB_ONLY",
]
