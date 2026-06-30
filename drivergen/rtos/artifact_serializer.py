"""pipeline step - :class:`RtosEvidenceArtifact` serializer."""

from __future__ import annotations

import json
import logging
from dataclasses import asdict, is_dataclass
from pathlib import Path
from typing import Any

from . import ARTIFACT_VERSION
from .extraction_pipeline import ExtractionResult
from .slot_fulfillment import build_slot_fulfillments
from .transaction_translator import build_transaction_templates
from .types import (
    BusIntent,
    EvidenceSpan,
    RtosEvidenceArtifact,
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SlotPlan,
    SourceRoot,
    SymbolBinding,
    TaskSpec,
)
from .validator import validate_evidence_artifact

logger = logging.getLogger(__name__)


# Serialization helpers


def _to_jsonable(value: Any) -> Any:
    """Recursively coerce a dataclass tree to plain JSON types."""
    if is_dataclass(value):
        return _to_jsonable(asdict(value))
    if isinstance(value, dict):
        return {k: _to_jsonable(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [_to_jsonable(v) for v in value]
    if isinstance(value, (set, frozenset)):
        return sorted(_to_jsonable(v) for v in value)
    if isinstance(value, Path):
        return str(value)
    return value


def _bus_intent_block(bi: BusIntent | None) -> dict:
    if bi is None:
        return {}
    return {
        "canonical_bus": bi.canonical_bus,
        "connection_type": bi.connection_type,
        "mode": bi.mode,
        "backend": bi.backend,
        "address_mode": bi.address_mode,
        "bus_instance": bi.bus_instance,
    }


def _source_roots_block(roots: list[SourceRoot]) -> list[dict]:
    return [
        {
            "root_id": r.root_id,
            "path": str(r.path),
            "roles": sorted(r.roles),
            "priority": r.priority,
            "rtos_scope_id": r.rtos_scope_id,
            "sha": r.sha,
        }
        for r in roots
    ]


def _evidence_block(spans: list[EvidenceSpan]) -> list[dict]:
    return [
        {
            "root_id": s.root_id,
            "path": s.path,
            "start_line": s.start_line,
            "end_line": s.end_line,
            "kind": s.kind,
        }
        for s in spans
    ]


def _binding_block(b: SymbolBinding) -> dict:
    return {
        "slot_id": b.slot_id,
        "symbol": b.symbol,
        "kind": b.kind,
        "source_kind": b.source_kind,
        "verification": b.verification,
        "signature": b.signature,
        "signature_source": b.signature_source,
        "declared_in": b.declared_in,
        "implemented_in": b.implemented_in,
        "required_headers": list(b.required_headers),
        "required_types": list(b.required_types),
        "return_semantics": b.return_semantics,
        "semantic_role": b.semantic_role,
        "confidence": b.confidence,
        "allowed_for_codegen": b.allowed_for_codegen,
        "requires_runtime_provision": b.requires_runtime_provision,
        "evidence": _evidence_block(b.evidence),
        "notes": list(b.notes),
    }


def _slot_block(
    slot,
    bindings_by_slot: dict[str, list[str]],
    coverage_status: dict[str, str],
    derivation_summary: dict[str, list[str]],
    confidence_by_slot: dict[str, float],
    slot_fulfillments: dict[str, dict],
) -> dict:
    fulfillment = slot_fulfillments.get(slot.slot_id)
    fulfillment_kind = (
        "symbol_binding"
        if bindings_by_slot.get(slot.slot_id)
        else (fulfillment or {}).get("fulfillment_kind")
    )
    confidence = confidence_by_slot.get(
        slot.slot_id,
        float((fulfillment or {}).get("confidence") or 0.0),
    )
    return {
        "required": slot.required,
        "layer": slot.layer,
        "canonical_bus": slot.canonical_bus,
        "expected_kinds": list(slot.expected_kinds),
        "preferred_root_roles": list(slot.preferred_root_roles),
        "source_kinds_allowed": list(slot.source_kinds_allowed),
        "origin": "+".join(derivation_summary.get(slot.slot_id, []))
                  or slot.origin,
        "status": coverage_status.get(slot.slot_id, "unknown"),
        "bindings": list(bindings_by_slot.get(slot.slot_id, [])),
        "fulfillment_kind": fulfillment_kind,
        "fulfillment": fulfillment,
        "confidence": confidence,
    }


def _coverage_status_map(
    result: ExtractionResult,
    slot_plan: SlotPlan,
    slot_fulfillments: dict[str, dict] | None = None,
) -> dict[str, str]:
    """Return ``{slot_id: 'covered' | 'missing' | 'ambiguous'}``."""
    bindings = result.bindings
    cov = result.coverage
    fulfilled = set(bindings) | set(slot_fulfillments or {})
    out: dict[str, str] = {}
    for s in slot_plan.slots:
        if s.slot_id in fulfilled:
            out[s.slot_id] = "covered"
        elif cov and s.slot_id in (cov.ambiguous or []):
            out[s.slot_id] = "ambiguous"
        else:
            out[s.slot_id] = "missing"
    return out


def _files_block(result: ExtractionResult) -> dict[str, list[dict]]:
    """Build the back-compat ``files`` view."""
    files: dict[str, list[dict]] = {
        "api_definition": [],
        "implementation": [],
        "board_config": [],
        "exemplar": [],
        "other": [],
    }
    for fkey, b in result.parsed_bundles.items():
        if not b.slot_ids:
            continue
        path_l = b.card.path.lower()
        role_hint = b.card.dir_role_hint or ""
        if role_hint == "exemplar":
            cat = "exemplar"
        elif role_hint in ("board", "board_integration"):
            cat = "board_config"
        elif path_l.endswith((".c", ".cpp", ".cc", ".cxx")):
            cat = "implementation"
        elif path_l.endswith((".h", ".hpp", ".hh", ".hxx")):
            cat = "api_definition"
        else:
            # Keep non-C/API files out of the API bucket.
            cat = "other"
        files[cat].append(
            {
                "path": b.card.path,
                "root_id": b.card.root_id,
                "file_kind": b.card.file_kind,
                "role_hint": role_hint or None,
                "slot_ids": list(b.slot_ids),
                "n_functions": b.n_functions,
                "n_macros": b.n_macros,
                "n_typedefs": b.n_typedefs,
                "n_structs": b.n_structs,
                "n_enums": b.n_enums,
            }
        )
    return files


def _source_include_headers(result: ExtractionResult, task_spec: TaskSpec) -> list[str]:
    """Headers observed in selected RTOS source files."""

    headers: list[str] = []
    seen: set[str] = set()

    def add(raw: Any) -> None:
        text = str(raw or "").strip().replace("\\", "/").strip("<>").strip('"')
        if not text or text in seen:
            return
        seen.add(text)
        headers.append(text)

    for bundle in (result.parsed_bundles or {}).values():
        if not getattr(bundle, "slot_ids", None):
            continue
        card_path = str(getattr(getattr(bundle, "card", None), "path", "") or "").replace("\\", "/")
        if card_path.lower().endswith((".h", ".hpp", ".hh", ".hxx")):
            add(card_path)
        parsed = getattr(bundle, "parsed", None)
        for inc in getattr(parsed, "include_graph", None) or []:
            add(inc)

    return headers


def _integration_block(
    task_spec: TaskSpec,
    bindings: dict[str, SymbolBinding],
    result: ExtractionResult,
) -> dict:
    """Pull integration-relevant fields out of TaskSpec + bindings."""
    cb = task_spec.connection_binding or {}
    da = task_spec.device_attachment or {}
    integration = {
        "bus_instance": cb.get("bus_instance") or task_spec.bus_intent.bus_instance,
        "bus_symbol": cb.get("bus_symbol"),
        "backend": cb.get("backend") or task_spec.bus_intent.backend,
        "mode": cb.get("mode") or task_spec.bus_intent.mode,
        "address_mode": cb.get("address_mode") or task_spec.bus_intent.address_mode,
        "fixed_attachment": cb.get("fixed_attachment") or {},
        "device_attachment": da,
        # Headers a generated driver must include, derived from the
        # actual bound symbols.
        "include_headers": sorted({
            h
            for b in bindings.values()
            for h in (b.required_headers or [])
            if h
        } | set(_source_include_headers(result, task_spec))),
        # Optional structured usage recipes from the connection-binding context.
        "helper_usage_patterns": list(cb.get("helper_usage_patterns") or []),
        # Dedup helper symbols so downstream prompt builders stay compact.
        "runtime_provision_required_for": sorted({
            b.symbol
            for b in bindings.values()
            if b.requires_runtime_provision and b.symbol
        }),
    }
    return integration


# Public API


def build_artifact(
    *,
    result: ExtractionResult,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    device_ir: dict | None = None,
    device_ir_hash: str | None = None,
    task_package_id: str | None = None,
    run_validation: bool = True,
) -> dict:
    """Return a plain JSON-shaped dict for one extraction result."""
    # Per-slot bindings name list + top binding's confidence.
    bindings_by_slot: dict[str, list[str]] = {}
    confidence_by_slot: dict[str, float] = {}
    for slot_id, b in result.bindings.items():
        bindings_by_slot.setdefault(slot_id, []).append(b.symbol)
        confidence_by_slot[slot_id] = max(
            confidence_by_slot.get(slot_id, 0.0), b.confidence
        )

    transaction_templates = (
        build_transaction_templates(
            device_ir=device_ir,
            bindings=result.bindings,
            slot_plan=slot_plan,
            task_spec=task_spec,
        ) if device_ir is not None else []
    )
    slot_fulfillments = build_slot_fulfillments(
        task_spec=task_spec,
        slot_plan=slot_plan,
        bindings=result.bindings,
        ranks=result.ranks,
        transaction_templates=transaction_templates,
        device_ir=device_ir,
    )

    coverage_status = _coverage_status_map(result, slot_plan, slot_fulfillments)

    slots_block = {
        s.slot_id: _slot_block(
            s,
            bindings_by_slot,
            coverage_status,
            slot_plan.derivation_summary,
            confidence_by_slot,
            slot_fulfillments,
        )
        for s in slot_plan.slots
    }

    # Key by slot id; keep a reverse symbol index for lookups by name.
    symbols_block: dict[str, dict] = {}
    symbol_index: dict[str, list[str]] = {}
    for slot_id, b in result.bindings.items():
        if not b.symbol:
            continue
        symbols_block[slot_id] = _binding_block(b)
        symbol_index.setdefault(b.symbol, []).append(slot_id)

    context_bindings = {
        sid: payload
        for sid, payload in slot_fulfillments.items()
        if payload.get("fulfillment_kind") == "context_binding"
    }
    multi_symbol_evidence = {
        sid: payload
        for sid, payload in slot_fulfillments.items()
        if payload.get("fulfillment_kind") == "multi_symbol_evidence"
    }
    transaction_fulfillments = {
        sid: payload
        for sid, payload in slot_fulfillments.items()
        if payload.get("fulfillment_kind") == "transaction_template"
    }
    required_symbol_bound = sum(
        1 for s in slot_plan.slots if s.required and s.slot_id in result.bindings
    )
    required_fulfilled = sum(
        1
        for s in slot_plan.slots
        if s.required and coverage_status.get(s.slot_id) == "covered"
    )

    artifact = {
        "version": ARTIFACT_VERSION,
        "task_spec": {
            "rtos_id": task_spec.rtos_id,
            "board": task_spec.board,
            "mcu_family": task_spec.mcu_family,
            "integration": task_spec.integration,
            "integration_style": task_spec.integration_style,
            "bus_intent": _bus_intent_block(task_spec.bus_intent),
            "device_id": task_spec.device_id,
            "device_ir_hash": device_ir_hash,
            "device_transaction_shape": task_spec.device_transaction_shape,
            "task_package_id": task_package_id,
        },
        "source_roots": _source_roots_block(task_spec.source_roots),
        "slots": slots_block,
        "symbols": symbols_block,
        "_symbol_index": symbol_index,
        "slot_fulfillments": slot_fulfillments,
        "context_bindings": context_bindings,
        "multi_symbol_evidence": multi_symbol_evidence,
        "transaction_fulfillments": transaction_fulfillments,
        "types": {},  # filled by RtosContract
        "integration": _integration_block(task_spec, result.bindings, result),
        "files": _files_block(result),
        "transaction_templates": transaction_templates,
        "validation": {
            "passed_gates": [],
            "failed_gates": [],
            "warnings": [],
            "gate_results": [],
        },
        "ledger": _to_jsonable(result.ledger),
        "summary": {
            "n_slots": len(slot_plan.slots),
            "n_bound": len(result.bindings),
            "n_fulfilled": sum(1 for s in slot_plan.slots if coverage_status.get(s.slot_id) == "covered"),
            "n_non_symbol_fulfillments": len(slot_fulfillments),
            "n_required": sum(1 for s in slot_plan.slots if s.required),
            # Preserve the legacy covered-slot counter name.
            "n_required_bound": required_fulfilled,
            "n_required_fulfilled": required_fulfilled,
            "n_required_symbol_bound": required_symbol_bound,
            "provenance": _provenance_summary(result.bindings),
            "timings_ms": {k: round(v * 1000.0, 2) for k, v in (result.timings or {}).items()},
            "budget": result.budget.to_dict() if result.budget else None,
        },
    }

    # Embed validation unless the caller is building a partial artifact.
    if run_validation:
        report = validate_evidence_artifact(
            artifact=artifact,
            bindings=result.bindings,
            task_spec=task_spec,
            slot_plan=slot_plan,
            parsed_bundles=result.parsed_bundles,
        )
        artifact["validation"] = report.to_dict()

    return artifact


def _provenance_summary(bindings: dict[str, SymbolBinding]) -> dict[str, int]:
    out: dict[str, int] = {}
    for b in bindings.values():
        out[b.source_kind] = out.get(b.source_kind, 0) + 1
    # Also surface how many require runtime provision.
    out["_requires_runtime_provision"] = sum(
        1 for b in bindings.values() if b.requires_runtime_provision
    )
    return out


def save_artifact(
    *,
    artifact: dict,
    output_dir: Path,
    filename: str = "rtos_artifact.json",
) -> Path:
    """Write *artifact* to ``output_dir / filename`` and return the path."""
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / filename
    out_path.write_text(
        json.dumps(artifact, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    logger.info("Wrote RTOS artifact to %s (%d KB)", out_path, out_path.stat().st_size // 1024)
    return out_path


def load_artifact(path: Path) -> dict:
    """Read an artifact JSON.  Kept symmetric with ``save_artifact``."""
    return json.loads(path.read_text(encoding="utf-8"))


# Artifact builder


def build_artifact_dataclass(
    *,
    result: ExtractionResult,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    device_ir_hash: str | None = None,
    task_package_id: str | None = None,
) -> RtosEvidenceArtifact:
    """Same as :func:`build_artifact` but returns the typed dataclass."""
    art_dict = build_artifact(
        result=result,
        task_spec=task_spec,
        slot_plan=slot_plan,
        device_ir_hash=device_ir_hash,
        task_package_id=task_package_id,
    )
    return RtosEvidenceArtifact(
        version=art_dict["version"],
        task_spec=task_spec,
        source_roots=list(task_spec.source_roots),
        slots=art_dict["slots"],
        symbols={
            name: _rebuild_symbol_binding(payload)
            for name, payload in art_dict["symbols"].items()
        },
        slot_fulfillments=art_dict.get("slot_fulfillments", {}),
        context_bindings=art_dict.get("context_bindings", {}),
        multi_symbol_evidence=art_dict.get("multi_symbol_evidence", {}),
        types=art_dict["types"],
        integration=art_dict["integration"],
        files=art_dict["files"],
        transaction_templates=art_dict["transaction_templates"],
        validation=None,
        ledger=result.ledger,
    )


def _rebuild_symbol_binding(payload: dict) -> SymbolBinding:
    spans = [
        EvidenceSpan(**{k: v for k, v in s.items() if k in EvidenceSpan.__dataclass_fields__})
        for s in payload.get("evidence", [])
    ]
    field_set = SymbolBinding.__dataclass_fields__
    init_kwargs = {k: v for k, v in payload.items() if k in field_set}
    init_kwargs["evidence"] = spans
    return SymbolBinding(**init_kwargs)


__all__ = [
    "build_artifact",
    "save_artifact",
    "load_artifact",
    "build_artifact_dataclass",
]
