"""pipeline step - Evidence Validator with 8 quality gates."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from typing import Any

from .deep_parser import ParsedFileBundle
from .types import (
    SOURCE_KIND_MANIFEST_REPO,
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SOURCE_KIND_STUB,
    SlotPlan,
    SymbolBinding,
    TaskSpec,
)

logger = logging.getLogger(__name__)


# Output dataclasses


@dataclass
class GateResult:
    """One quality-gate outcome."""

    gate_id: str
    passed: bool
    severity: str
    message: str
    evidence: dict = field(default_factory=dict)


@dataclass
class EvidenceValidationReport:
    """Result of running every gate against an artifact + bindings."""

    passed_gates: list[str] = field(default_factory=list)
    failed_gates: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    gate_results: list[GateResult] = field(default_factory=list)

    @property
    def is_clean(self) -> bool:
        """True iff no gate failed (warnings allowed)."""
        return not self.failed_gates

    def to_dict(self) -> dict:
        return {
            "passed_gates": list(self.passed_gates),
            "failed_gates": list(self.failed_gates),
            "warnings": list(self.warnings),
            "gate_results": [
                {
                    "gate_id": r.gate_id,
                    "passed": r.passed,
                    "severity": r.severity,
                    "message": r.message,
                    "evidence": r.evidence,
                }
                for r in self.gate_results
            ],
        }


# Helpers


_KERNEL_ONLY_RTOS_IDS = frozenset({
    "freertos",
    "threadx",
    "openharmony-liteosm",
    "tobudos",
})

_FOREIGN_ROLE_HINTS = frozenset({"exemplar", "demo", "docs"})


def _path_mentions_family(path: str, task_mcu_family: str) -> bool:
    """True when the binding path literally contains the task's MCU family token (case-insensitive substring)."""
    if not task_mcu_family:
        return False
    return task_mcu_family.lower() in path.lower()


def _add_result(
    report: EvidenceValidationReport,
    gate_id: str,
    passed: bool,
    *,
    severity: str = "error",
    message: str = "",
    evidence: dict | None = None,
) -> None:
    """Append a :class:`GateResult` and update the report's index lists."""
    result = GateResult(
        gate_id=gate_id,
        passed=passed,
        severity=severity,
        message=message,
        evidence=evidence or {},
    )
    report.gate_results.append(result)
    if passed:
        report.passed_gates.append(gate_id)
    elif severity == "error":
        report.failed_gates.append(gate_id)
    elif severity == "warning":
        report.warnings.append(gate_id)


# Individual gates


def _gate_root_coverage(
    report: EvidenceValidationReport, task_spec: TaskSpec
) -> None:
    """Gate 1 — kernel-only RTOSs must have ≥ 2 source roots."""
    rtos_id = (task_spec.rtos_id or "").lower()
    n_roots = len(task_spec.source_roots or [])
    if rtos_id in _KERNEL_ONLY_RTOS_IDS:
        passed = n_roots >= 2
        _add_result(
            report,
            "root_coverage",
            passed,
            severity="error" if not passed else "info",
            message=(
                f"kernel-only rtos {rtos_id!r} has {n_roots} source roots "
                f"(need >=2 for kernel + vendor/board)"
            ),
            evidence={"rtos_id": rtos_id, "n_roots": n_roots},
        )
    else:
        # Integrated repositories commonly use a single source root.
        _add_result(
            report,
            "root_coverage",
            True,
            severity="info",
            message=f"rtos {rtos_id!r} is integrated; n_roots={n_roots} (no minimum)",
            evidence={"rtos_id": rtos_id, "n_roots": n_roots},
        )


def _gate_slot_coverage(
    report: EvidenceValidationReport,
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    artifact: dict | None = None,
) -> None:
    """Gate 2 — every ``required`` slot is bound."""
    artifact = artifact or {}
    slot_fulfillments = artifact.get("slot_fulfillments") or {}
    fulfilled = set(bindings)
    if isinstance(slot_fulfillments, dict):
        for slot_id, payload in slot_fulfillments.items():
            if isinstance(payload, dict) and payload.get("fulfillment_kind"):
                fulfilled.add(slot_id)
    missing = [s.slot_id for s in slot_plan.slots if s.required and s.slot_id not in fulfilled]
    passed = not missing
    _add_result(
        report,
        "slot_coverage",
        passed,
        severity="error" if not passed else "info",
        message=(
            f"{len(missing)} required slot(s) uncovered: {missing}"
            if missing else "all required slots covered"
        ),
        evidence={
            "missing_required": missing,
            "non_symbol_fulfillments": sorted(set(slot_fulfillments or {})),
        },
    )


def _gate_symbol_existence(
    report: EvidenceValidationReport,
    bindings: dict[str, SymbolBinding],
    parsed_bundles: dict[str, ParsedFileBundle] | None,
) -> None:
    """Gate 3 — manifest_repo bindings have a parsed declaration."""
    if parsed_bundles is None:
        _add_result(
            report,
            "symbol_existence",
            True,
            severity="info",
            message="parsed_bundles not supplied; gate skipped",
        )
        return

    # Build a fast lookup of parsed function/macro/typedef/struct/enum names.
    parsed_names: set[tuple[str, str]] = set()  # {(name, kind)}
    for b in parsed_bundles.values():
        for fn in b.parsed.function_declarations:
            parsed_names.add((fn.name, "function"))
        for m in b.parsed.macro_definitions:
            parsed_names.add((m.name, "macro"))
        for t in b.parsed.typedef_definitions:
            parsed_names.add((t.name, "typedef"))
        for s in b.parsed.struct_definitions:
            parsed_names.add((s.name, "struct"))
        for e in b.parsed.enum_definitions:
            parsed_names.add((e.name, "enum"))

    missing: list[dict] = []
    for slot_id, binding in bindings.items():
        if binding.source_kind != SOURCE_KIND_MANIFEST_REPO:
            continue
        if not binding.symbol or not binding.kind:
            missing.append({"slot": slot_id, "binding": binding.symbol, "reason": "no symbol/kind"})
            continue
        if (binding.symbol, binding.kind) not in parsed_names:
            missing.append({
                "slot": slot_id, "symbol": binding.symbol, "kind": binding.kind,
                "reason": "not in parsed_bundles",
            })

    passed = not missing
    _add_result(
        report,
        "symbol_existence",
        passed,
        severity="error" if not passed else "info",
        message=(
            f"{len(missing)} manifest_repo binding(s) missing parsed declaration"
            if missing else "all manifest_repo bindings traced to parsed source"
        ),
        evidence={"missing": missing[:8]},  # cap to keep the report short
    )


def _gate_root_role(
    report: EvidenceValidationReport,
    bindings: dict[str, SymbolBinding],
    parsed_bundles: dict[str, ParsedFileBundle] | None,
) -> None:
    """Gate 4 — bus / runtime bindings must not come from a demo / docs
    root.  Skipped for task_package_helper / stub bindings (they don't
    have a real source location).
    """
    if parsed_bundles is None:
        _add_result(
            report, "root_role", True, severity="info",
            message="parsed_bundles not supplied; gate skipped",
        )
        return

    file_role: dict[str, str | None] = {}
    for fkey, b in parsed_bundles.items():
        file_role[fkey] = b.card.dir_role_hint

    bad: list[dict] = []
    for slot_id, binding in bindings.items():
        if binding.source_kind != SOURCE_KIND_MANIFEST_REPO:
            continue
        # binding.declared_in is "<root_id>::<rel_path>"
        if not binding.declared_in:
            continue
        role = file_role.get(binding.declared_in)
        if role and role in _FOREIGN_ROLE_HINTS:
            bad.append({
                "slot": slot_id, "symbol": binding.symbol,
                "declared_in": binding.declared_in, "role_hint": role,
            })

    passed = not bad
    _add_result(
        report,
        "root_role",
        passed,
        severity="error" if not passed else "info",
        message=(
            f"{len(bad)} bindings come from demo / docs trees"
            if bad else "all manifest_repo bindings come from authoritative roles"
        ),
        evidence={"bad": bad[:8]},
    )


def _gate_mcu_affinity(
    report: EvidenceValidationReport,
    task_spec: TaskSpec,
    bindings: dict[str, SymbolBinding],
) -> None:
    """Gate 5 - diagnostic check that at least one manifest binding's path actually references the task MCU family."""
    family = task_spec.mcu_family or ""
    if not family:
        _add_result(
            report, "mcu_affinity", True, severity="info",
            message="task_spec.mcu_family not set; gate skipped",
        )
        return

    paths_mentioning_family: list[str] = []
    paths_other: list[str] = []
    for slot_id, binding in bindings.items():
        if binding.source_kind != SOURCE_KIND_MANIFEST_REPO:
            continue
        if not binding.declared_in:
            continue
        if _path_mentions_family(binding.declared_in, family):
            paths_mentioning_family.append(binding.declared_in)
        else:
            paths_other.append(binding.declared_in)

    n_match = len(paths_mentioning_family)
    n_other = len(paths_other)
    if n_match > 0:
        _add_result(
            report,
            "mcu_affinity",
            True,
            severity="info",
            message=(
                f"{n_match} manifest binding(s) declared in a path mentioning "
                f"mcu_family={family} (other manifest bindings live in "
                f"family-agnostic source: {n_other})"
            ),
            evidence={
                "matching_paths_sample": paths_mentioning_family[:4],
                "other_paths_sample": paths_other[:4],
            },
        )
    else:
        _add_result(
            report,
            "mcu_affinity",
            False,
            severity="warning",
            message=(
                f"no manifest binding's declared_in mentions mcu_family={family} "
                f"({n_other} manifest binding(s) checked); symbol binder is "
                f"the source of truth for MCU correctness — review for confidence"
            ),
            evidence={"other_paths_sample": paths_other[:8]},
        )


def _gate_example_consistency(
    report: EvidenceValidationReport,
    artifact: dict,
    bindings: dict[str, SymbolBinding],
    parsed_bundles: dict[str, ParsedFileBundle] | None,
) -> None:
    """Gate 6 — exemplar files in artifact.files.exemplar should
    invoke at least one bound symbol; otherwise they're noise.
    Severity is **warning** (not error) — exemplars are advisory.
    """
    files = artifact.get("files", {}) if isinstance(artifact, dict) else {}
    exemplar_paths = [
        e.get("path") for e in (files.get("exemplar") or []) if isinstance(e, dict)
    ]
    exemplar_paths = [p for p in exemplar_paths if p]
    if not exemplar_paths:
        _add_result(
            report, "example_consistency", True, severity="info",
            message="no exemplar files referenced; gate skipped",
        )
        return

    bound_names = {b.symbol for b in bindings.values() if b.symbol}

    # Deep exemplar checks require parsed bundles.
    if parsed_bundles is None:
        _add_result(
            report, "example_consistency", True, severity="info",
            message=f"{len(exemplar_paths)} exemplar(s) listed; parsed_bundles missing for deep check",
            evidence={"exemplar_count": len(exemplar_paths)},
        )
        return

    # Map exemplar path → exported function names that overlap with bound symbols.
    relevant: list[dict] = []
    for fkey, b in parsed_bundles.items():
        if not any(b.card.path.endswith(p) or p.endswith(b.card.path) for p in exemplar_paths):
            continue
        # Approximate exemplar relevance by declared symbol overlap.
        local_names = {fn.name for fn in b.parsed.function_declarations}
        overlap = local_names & bound_names
        if overlap:
            relevant.append({"exemplar": b.card.path, "uses": sorted(overlap)[:5]})

    if relevant:
        _add_result(
            report, "example_consistency", True, severity="info",
            message=f"{len(relevant)} exemplar(s) overlap with bound symbols",
            evidence={"matches": relevant},
        )
    else:
        _add_result(
            report, "example_consistency", False, severity="warning",
            message=(
                f"no exemplar overlaps with bound symbols "
                f"({len(exemplar_paths)} exemplar(s) listed, none referenced bound API)"
            ),
            evidence={"exemplar_paths": exemplar_paths[:5]},
        )


def _gate_no_stub_authority(
    report: EvidenceValidationReport,
    bindings: dict[str, SymbolBinding],
) -> None:
    """Gate 7 — stub bindings must have allowed_for_codegen=False."""
    bad = [
        {"slot": s, "symbol": b.symbol, "allowed_for_codegen": b.allowed_for_codegen}
        for s, b in bindings.items()
        if b.source_kind == SOURCE_KIND_STUB and b.allowed_for_codegen
    ]
    passed = not bad
    _add_result(
        report,
        "no_stub_authority",
        passed,
        severity="error" if not passed else "info",
        message=(
            f"{len(bad)} stub binding(s) marked allowed_for_codegen=True"
            if bad else "no stub source_kind in current bindings (or all marked link-only)"
        ),
        evidence={"bad": bad[:8]},
    )


def _gate_cache_provenance(
    report: EvidenceValidationReport,
    artifact: dict,
) -> None:
    """Gate 8 — when artifact carries cache descriptors (extractor /
    contract metadata), the required version + compat fields must be
    populated.
    """
    if not isinstance(artifact, dict):
        _add_result(
            report, "cache_provenance", True, severity="info",
            message="artifact not a dict; gate skipped",
        )
        return

    version = artifact.get("version")
    task_spec_block = artifact.get("task_spec") or {}
    device_ir_hash = task_spec_block.get("device_ir_hash") if isinstance(task_spec_block, dict) else None
    summary = artifact.get("summary") or {}

    missing: list[str] = []
    if not version:
        missing.append("version")
    if not device_ir_hash:
        missing.append("task_spec.device_ir_hash")
    if not summary or "n_required_bound" not in summary:
        missing.append("summary.n_required_bound")

    passed = not missing
    _add_result(
        report,
        "cache_provenance",
        passed,
        severity="error" if not passed else "info",
        message=(
            f"missing cache-provenance field(s): {missing}"
            if missing else "all cache-provenance fields present"
        ),
        evidence={"missing": missing},
    )


# Public entry


def validate_evidence_artifact(
    *,
    artifact: dict | Any,
    bindings: dict[str, SymbolBinding],
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    parsed_bundles: dict[str, ParsedFileBundle] | None = None,
) -> EvidenceValidationReport:
    """Run every gate; return a consolidated :class:`EvidenceValidationReport`."""
    if not isinstance(artifact, dict):
        # Coerce typed artifacts and synthesize missing provenance.
        ts = getattr(artifact, "task_spec", None)
        synthetic_hash = "dataclass-no-ir-hash"
        if ts is not None:
            bi = getattr(ts, "bus_intent", None)
            seed_parts = [
                getattr(ts, "rtos_id", "") or "",
                getattr(ts, "device_id", "") or "",
                getattr(bi, "canonical_bus", "") if bi else "",
                getattr(bi, "connection_type", "") if bi else "",
            ]
            joined = "|".join(seed_parts)
            if joined.strip("|"):
                synthetic_hash = f"dataclass-{abs(hash(joined)) & 0xFFFFFFFF:08x}"
        artifact_dict = {
            "version": getattr(artifact, "version", None),
            "files": getattr(artifact, "files", {}) or {},
            "task_spec": {
                "device_ir_hash": synthetic_hash,
            },
            "summary": {"n_required_bound": sum(1 for s in slot_plan.slots if s.required and s.slot_id in bindings)},
        }
    else:
        artifact_dict = artifact

    report = EvidenceValidationReport()
    _gate_root_coverage(report, task_spec)
    _gate_slot_coverage(report, slot_plan, bindings, artifact_dict)
    _gate_symbol_existence(report, bindings, parsed_bundles)
    _gate_root_role(report, bindings, parsed_bundles)
    _gate_mcu_affinity(report, task_spec, bindings)
    _gate_example_consistency(report, artifact_dict, bindings, parsed_bundles)
    _gate_no_stub_authority(report, bindings)
    _gate_cache_provenance(report, artifact_dict)

    logger.info(
        "Evidence validator: %d passed, %d failed, %d warnings",
        len(report.passed_gates),
        len(report.failed_gates),
        len(report.warnings),
    )
    return report


__all__ = [
    "GateResult",
    "EvidenceValidationReport",
    "validate_evidence_artifact",
]
