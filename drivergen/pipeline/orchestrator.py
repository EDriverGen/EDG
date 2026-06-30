"""Top-level RTOS-side orchestration for DriverGen runs and extraction tools."""

from __future__ import annotations

import dataclasses
import hashlib
import json
import logging
import re
import shutil
import time
from datetime import datetime
from pathlib import Path
from typing import Mapping

from ..core.catalog import DATA_ROOT, RUNS_ROOT
from ..context import (
    build_source_lookup,
    load_board_context,
)
from ..context.fixed import build_target_binding, resolve_run_fixed_context
from ..core.ir_canonicalize import (
    canonicalize_address_rule,
    canonicalize_conversion_formulae,
    canonicalize_gpio_byte_frame_sources,
    canonicalize_operation_flows,
    canonicalize_primary_interface_and_variant,
)
from ..core.ir_facts import (
    build_device_ir_fact_bank,
    format_candidate_fact_summary_for_ir_prompt,
    summarize_candidate_fact_bank,
    summarize_fact_bank,
)
from ..core.ir_flow_risk import assess_ir_flow_risk, repair_formula_flow_dependencies
from ..core.run_config import (
    PipelineRunConfig,
    run_config_from_task_package,
)
from ..core.response_schemas import (
    DEVICE_IR_EVIDENCE_REPAIR_SCHEMA,
    DEVICE_IR_FACT_CANDIDATES_SCHEMA,
    DEVICE_IR_FLOW_AUDIT_SCHEMA,
    DEVICE_IR_SCHEMA,
    IR_SCHEMA_VERSION,
)
from ..core.validators import validate_ir
from ..llm.prompts import (
    build_device_ir_evidence_repair_prompt,
    build_device_ir_fact_candidate_prompt,
    build_device_ir_flow_audit_prompt,
    build_device_ir_flow_risk_repair_prompt,
    build_device_ir_schema_repair_prompt,
    build_structured_ir_prompt,
)
from ..llm.prompts.ir_extraction import detect_violations as detect_ir_prompt_violations
from ..llm.providers import create_provider
from ..rtos import (
    ARTIFACT_VERSION,
    get_rtos_profile,
)
from ..rtos.artifact_serializer import (
    _rebuild_symbol_binding,
    build_artifact,
    save_artifact,
)
from ..rtos.bundle_cache import build_or_load_repo_index_bundle
from ..rtos.contract_builder import build_rtos_contract, save_rtos_contract
from ..rtos.evidence_cache import (
    evidence_cache_dir_for,
    load_evidence_artifact,
    save_evidence_artifact,
    task_cache_key,
)
from ..rtos.extraction_pipeline import (
    ExtractionResult,
    run_round0_extraction,
    save_extraction_ledger,
)
from ..rtos.llm_infra import make_budget_tracker
from ..rtos.registry import canonical_rtos_id as _canonical_rtos_id
from ..rtos.scope_llm import load_or_synthesize_scope_map
# Scope maps are synthesized per task; validation checks the map against
# resolved SourceRoots before handing it to the indexer.
from ..rtos.scope_map import validate_scope_map_against_roots
from ..rtos.slot_derivation import build_slot_plan
from ..rtos.source_roots import resolve_source_roots
from ..rtos.task_spec import build_task_spec
from ..codegen import (
    classify_device,
    derive_expected_transactions,
    route,
    run_repair_loop,
)

logger = logging.getLogger(__name__)


def _resolve_context_path(rel_path: str) -> Path:
    """Resolve repository-relative context paths to absolute paths."""
    from ..core.catalog import PROJECT_ROOT
    normalised = rel_path.replace("\\", "/")
    if normalised.startswith("DriverGen/"):
        normalised = normalised[len("DriverGen/"):]
    return PROJECT_ROOT / normalised


def _write_json(path: Path, payload: dict | list) -> None:
    """Write JSON with consistent formatting and auto-create parent folders."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def _write_text(path: Path, content: str) -> None:
    """Write UTF-8 text and create parent folders when needed."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def _read_optional_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8") if path.exists() else ""
    except OSError:
        return ""


def _stamp_ir_schema_version(
    device_ir: dict,
    *,
    target_bus_type: str | None = None,
    target_device_id: str | None = None,
) -> dict:
    """Ensure ``device_ir['ir_schema_version']`` matches the current schema and apply deterministic IR canonicalizers."""
    if isinstance(device_ir, dict):
        device_ir["ir_schema_version"] = IR_SCHEMA_VERSION
        canonicalize_address_rule(device_ir)
        canonicalize_operation_flows(device_ir)
        canonicalize_conversion_formulae(device_ir)
        canonicalize_gpio_byte_frame_sources(device_ir)
        canonicalize_primary_interface_and_variant(
            device_ir,
            target_bus_type=target_bus_type,
            target_device_id=target_device_id,
        )
    return device_ir


def _write_device_ir_fact_bank(
    device_ir: dict,
    run_dir: Path,
    stage_report: dict[str, object] | None = None,
    candidate_fact_bank: dict | None = None,
) -> dict:
    """Write the auditable fact-bank sidecar for the final Device IR."""
    if candidate_fact_bank is None:
        candidate_path = run_dir / "device_ir_candidate_facts.json"
        if candidate_path.exists():
            try:
                candidate_fact_bank = json.loads(candidate_path.read_text(encoding="utf-8"))
            except (OSError, ValueError):
                candidate_fact_bank = None
    fact_bank = build_device_ir_fact_bank(device_ir, candidate_fact_bank=candidate_fact_bank)
    _write_json(run_dir / "device_ir_facts.json", fact_bank)
    if stage_report is not None:
        stage_report.setdefault("stage_c", {})["fact_bank"] = summarize_fact_bank(fact_bank)
        if isinstance(candidate_fact_bank, dict):
            stage_report.setdefault("stage_c", {})["candidate_projection"] = (
                fact_bank.get("candidate_projection", {}).get("missing_candidate_counts", {})
            )
    return fact_bank


def _llm_debug_metadata(run_dir: Path, **extra: object) -> dict:
    """Build metadata for provider calls that should persist raw model outputs."""
    metadata = {"debug_dir": str(run_dir / "llm_debug")}
    metadata.update(extra)
    return metadata


def _timestamp() -> str:
    """Return a compact UTC timestamp for run directory names."""
    return datetime.utcnow().strftime("%y%m%dT%H%M%SZ")


def _sanitize_name(value: str) -> str:
    """Make user-provided names safe for run directory paths."""
    return re.sub(r"[^A-Za-z0-9._-]+", "_", value).strip("_") or "input"


def _board_run_token(board_context: dict) -> str:
    """Return a short board token for run directory names."""
    board_name = (
        board_context.get("board_alias")
        or board_context.get("board_short")
        or board_context.get("board")
        or "board"
    )
    return _sanitize_name(str(board_name))


def _run_config_run_name(config: PipelineRunConfig, board_context: dict, provider_name: str) -> str:
    """Build the default run directory name for a formal generation run_config."""
    return (
        f"{_sanitize_name(config.device_id)}_"
        f"{_sanitize_name(config.rtos_id)}_"
        f"{_board_run_token(board_context)}_"
        f"{_sanitize_name(provider_name)}_"
        f"{_timestamp()}"
    )


def _prepare_run_context(config: PipelineRunConfig) -> dict:
    """Load and validate all shared inputs before the agentless pipeline runs."""
    rtos_profile = get_rtos_profile(config.rtos_id)
    if not rtos_profile.implemented:
        raise ValueError(rtos_profile.unsupported_reason or f"RTOS '{config.rtos_id}' is not implemented.")

    fixed_context = resolve_run_fixed_context(config)
    if fixed_context:
        board_context = fixed_context["board_context"]
    else:
        board_context = load_board_context(config.board_context_path)
    board_rtos = _canonical_rtos_id(board_context.get("rtos"))
    if board_rtos != _canonical_rtos_id(config.rtos_id):
        raise ValueError(
            f"Board context RTOS mismatch: expected '{config.rtos_id}', found '{board_rtos}'."
        )

    target_binding = build_target_binding(config, board_context, fixed_context)

    # RTOS evidence extraction owns source discovery.
    context_sources: dict[str, Path] = {}
    source_lookup = build_source_lookup(config.device_id, [], context_sources)

    return {
        "run_config": config,
        "rtos_profile": rtos_profile,
        "board_context": board_context,
        "fixed_context": fixed_context,
        "target_binding": target_binding,
        "context_sources": context_sources,
        "source_lookup": source_lookup,
    }


def _build_rtos_extraction(
    prepared: dict,
    *,
    device_ir: dict,
    provider,
    run_dir: Path | None = None,
    cache_root: Path | None = None,
    use_evidence_cache: bool = True,
    budget_mode: str = "default",
) -> tuple[dict, dict, ExtractionResult]:
    """Run the RTOS context pipeline end-to-end."""
    run_config = prepared["run_config"]
    fixed_context = prepared["fixed_context"] or {}

    platform_base = (fixed_context.get("platform_base_context") or {})
    # If platform_base_context is a path string (common in task packages),
    # load the actual JSON file to get rtos_bundle.
    if isinstance(platform_base, str) and platform_base.strip():
        _pb_path = _resolve_context_path(platform_base)
        if _pb_path.is_file():
            try:
                _pb_loaded = json.loads(_pb_path.read_text(encoding="utf-8"))
                if isinstance(_pb_loaded, Mapping):
                    platform_base = _pb_loaded
            except (OSError, ValueError):
                pass
    bundle_id = (
        platform_base.get("rtos_bundle")
        or platform_base.get("rtos")
        or run_config.rtos_id
    )
    if not bundle_id:
        raise ValueError(
            "RTOS extraction: cannot determine manifest bundle_id; "
            "task package must set platform_base_context.rtos_bundle."
    )

    rtos_id = (platform_base.get("rtos") or run_config.rtos_id or "").strip().lower()

    # 1. Resolve source roots.
    logger.info("RTOS extraction: resolving source roots for bundle_id=%s", bundle_id)
    source_roots = resolve_source_roots(bundle_id)
    logger.info("RTOS extraction: resolved %d source roots", len(source_roots))

    # 2. TaskSpec (needed early so scope-map synthesis can target the
    #    actual MCU + bus). Re-using task_spec for slot_plan below.
    connection_binding = fixed_context.get("connection_binding_context") or {}
    device_attachment = fixed_context.get("device_attachment_context") or {}

    task_spec = build_task_spec(
        platform_base_context=platform_base,
        connection_binding=connection_binding,
        device_attachment=device_attachment,
        device_ir=device_ir,
        source_roots=source_roots,
    )

    # Build or load a task-specific scope map.
    bus_kinds_for_scope: tuple[str, ...] = ()
    if task_spec.bus_intent and task_spec.bus_intent.canonical_bus:
        bk = task_spec.bus_intent.canonical_bus.strip().lower()
        if bk and bk != "unknown":
            bus_kinds_for_scope = (bk,)
    budget = make_budget_tracker(mode=budget_mode)
    logger.info(
        "RTOS extraction: loading/synthesizing scope map rtos_id=%s mcu_family=%s buses=%s provider=%s model=%s",
        rtos_id,
        task_spec.mcu_family,
        bus_kinds_for_scope,
        getattr(provider, "name", None),
        getattr(provider, "model", None),
    )
    scope_map = load_or_synthesize_scope_map(
        rtos_id=rtos_id,
        source_roots=source_roots,
        mcu_family=task_spec.mcu_family,
        bus_kinds=bus_kinds_for_scope,
        provider=provider,
        budget=budget,
    )
    logger.info(
        "RTOS extraction: scope map %s",
        "miss/none" if scope_map is None else f"ready ({getattr(scope_map, 'rtos_id', rtos_id)})",
    )
    if scope_map is None:
        logger.warning(
            "RTOS extraction: scope_llm produced no ScopeMap for rtos_id=%r "
            "(provider unavailable, cache miss, or scope synthesis disabled);"
            " indexer will keep every code file (no role hints).",
            rtos_id,
        )
    else:
        sm_report = validate_scope_map_against_roots(
            scope_map, source_roots,
            strict_unmatched_root_id_patterns=False,
        )
        for warn in sm_report.warnings:
            logger.warning("scope_map(%s): %s", scope_map.rtos_id, warn)
        for err in sm_report.errors:
            logger.error("scope_map(%s): %s", scope_map.rtos_id, err)

    # Repo index bundle.
    logger.info("RTOS extraction: building/loading repo index bundle")
    repo_index_bundle = build_or_load_repo_index_bundle(
        source_roots=source_roots,
        scope_map=scope_map,
    )
    logger.info("RTOS extraction: repo index bundle ready")

    # Slot plan.
    slot_plan = build_slot_plan(task_spec=task_spec, device_ir=device_ir)
    task_spec.slot_plan = slot_plan

    device_ir_hash = hashlib.sha1(
        json.dumps(device_ir, sort_keys=True, ensure_ascii=False).encode("utf-8")
    ).hexdigest()[:16]

    # Per-task evidence cache.
    cache_dir = None
    if use_evidence_cache:
        cache_dir = evidence_cache_dir_for(
            task_spec=task_spec,
            slot_plan=slot_plan,
            bundle=repo_index_bundle,
            device_ir_hash=device_ir_hash,
            cache_root=cache_root,
        )
        cached = load_evidence_artifact(
            cache_dir, expected_artifact_version=ARTIFACT_VERSION,
        )
        if cached is not None:
            logger.info("RTOS extraction: cache hit at %s", cache_dir)
            if run_dir is not None:
                save_artifact(artifact=cached, output_dir=run_dir)
            # Rebuild bindings so cached artifacts expose the same API surface as fresh extraction.
            cached_bindings = {
                slot_id: _rebuild_symbol_binding(payload)
                for slot_id, payload in (cached.get("symbols") or {}).items()
                if isinstance(payload, dict)
            }
            contract = build_rtos_contract(
                artifact=cached,
                task_spec=task_spec,
                slot_plan=slot_plan,
                bindings=cached_bindings,
                task_package_id=platform_base.get("task_package_id"),
            )
            if run_dir is not None:
                save_rtos_contract(contract=contract, output_dir=run_dir)
            return contract, cached, ExtractionResult(bindings=cached_bindings)
        logger.info("RTOS extraction: evidence cache miss at %s", cache_dir)

    # Initial extraction pass.
    metadata = {"debug_dir": str(run_dir) if run_dir else None}
    logger.info("RTOS extraction: running initial extraction")
    extraction_result = run_round0_extraction(
        bundle=repo_index_bundle,
        task_spec=task_spec,
        slot_plan=slot_plan,
        provider=provider,
        budget=budget,
        metadata=metadata,
    )
    logger.info("RTOS extraction: initial extraction finished")

    # Build artifact and contract.
    artifact = build_artifact(
        result=extraction_result,
        task_spec=task_spec,
        slot_plan=slot_plan,
        device_ir=device_ir,
        device_ir_hash=device_ir_hash,
        task_package_id=platform_base.get("task_package_id"),
        run_validation=True,
    )

    contract = build_rtos_contract(
        artifact=artifact,
        task_spec=task_spec,
        slot_plan=slot_plan,
        bindings=extraction_result.bindings,
        task_package_id=platform_base.get("task_package_id"),
        requires_human=[
            sid for sid, d in (extraction_result.gap_diagnoses or {}).items()
            if d.requires_human
        ],
    )

    if run_dir is not None:
        save_artifact(artifact=artifact, output_dir=run_dir)
        save_rtos_contract(contract=contract, output_dir=run_dir)
        save_extraction_ledger(extraction_result, run_dir)

    if use_evidence_cache and cache_dir is not None:
        try:
            save_evidence_artifact(artifact=artifact, cache_dir=cache_dir)
        except OSError as exc:
            logger.warning(
                "RTOS extraction: failed to write evidence cache to %s: %s",
                cache_dir, exc,
            )

    return contract, artifact, extraction_result


def _repair_device_ir_schema_if_needed(
    *,
    provider,
    device_id: str,
    device_ir: dict,
    source_lookup: dict,
    run_dir: Path,
) -> tuple[dict, object]:
    """Run one schema repair attempt for required-field issues in ``device_ir``."""
    def _issue_dicts(validation_result) -> list[dict]:
        return [
            {
                "level": issue.level,
                "check_id": issue.check_id,
                "message": issue.message,
            }
            for issue in validation_result.issues
        ]

    ir_validation = validate_ir(device_ir, source_lookup)
    if ir_validation.ok:
        return device_ir, ir_validation

    schema_issues = [
        {
            "level": issue.level,
            "check_id": issue.check_id,
            "message": issue.message,
        }
        for issue in ir_validation.issues
        if not issue.check_id.startswith("device_ir.evidence_")
    ]
    if not schema_issues:
        return device_ir, ir_validation

    structured_prompt_path = run_dir / "structured_prompt_preview.txt"
    structured_content = (
        structured_prompt_path.read_text(encoding="utf-8")
        if structured_prompt_path.exists()
        else ""
    )

    repair_system, repair_user = build_device_ir_schema_repair_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=device_ir,
        validation_issues=schema_issues,
    )
    try:
        repaired_payload = provider.generate_json(
            task_name="repair_device_ir_schema",
            schema=DEVICE_IR_SCHEMA,
            system_prompt=repair_system,
            user_prompt=repair_user,
            metadata=_llm_debug_metadata(
                run_dir,
                device_id=device_id,
                repair_target="device_ir_schema",
            ),
        )
    except Exception as exc:
        logger.warning(
            "Schema repair for %s failed (%s); keeping original IR.",
            device_id, exc,
        )
        _write_json(
            run_dir / "device_ir_schema_repair.json",
            {
                "applied": False,
                "error": str(exc),
                "issues_before": _issue_dicts(ir_validation),
                "repaired_payload": None,
            },
        )
        return device_ir, ir_validation

    # Preserve the original evidence spans defensively.
    candidate_ir = dict(repaired_payload)
    candidate_ir["evidence_spans"] = list(device_ir.get("evidence_spans", []))
    repaired_validation = validate_ir(candidate_ir, source_lookup)

    receipt = {
        "applied": False,
        "issues_before": _issue_dicts(ir_validation),
        "issues_after": _issue_dicts(repaired_validation),
        "schema_issues_targeted": schema_issues,
        "repaired_payload": candidate_ir,
    }
    if len(repaired_validation.issues) < len(ir_validation.issues):
        receipt["applied"] = True
        _write_json(run_dir / "device_ir_schema_repair.json", receipt)
        return candidate_ir, repaired_validation

    _write_json(run_dir / "device_ir_schema_repair.json", receipt)
    return device_ir, ir_validation


def _repair_device_ir_evidence_if_needed(
    *,
    provider,
    device_id: str,
    device_ir: dict,
    source_lookup: dict,
    run_dir: Path,
) -> tuple[dict, object]:
    """Repair only the evidence spans when validation fails on snippet grounding."""
    def _issue_dicts(validation_result) -> list[dict]:
        return [
            {
                "level": issue.level,
                "check_id": issue.check_id,
                "message": issue.message,
            }
            for issue in validation_result.issues
        ]

    ir_validation = validate_ir(device_ir, source_lookup)
    if ir_validation.ok:
        return device_ir, ir_validation

    evidence_issues = [
        {
            "level": issue.level,
            "check_id": issue.check_id,
            "message": issue.message,
        }
        for issue in ir_validation.issues
        if issue.check_id.startswith("device_ir.evidence_")
    ]
    if not evidence_issues:
        return device_ir, ir_validation

    structured_prompt_path = run_dir / "structured_prompt_preview.txt"
    if not structured_prompt_path.exists():
        return device_ir, ir_validation

    repair_system, repair_user = build_device_ir_evidence_repair_prompt(
        device_id=device_id,
        structured_content=structured_prompt_path.read_text(encoding="utf-8"),
        current_device_ir=device_ir,
        validation_issues=evidence_issues,
    )
    try:
        repaired_payload = provider.generate_json(
            task_name="repair_device_ir_evidence",
            schema=DEVICE_IR_EVIDENCE_REPAIR_SCHEMA,
            system_prompt=repair_system,
            user_prompt=repair_user,
            metadata=_llm_debug_metadata(
                run_dir,
                device_id=device_id,
                repair_target="device_ir_evidence",
            ),
        )
    except Exception:
        return device_ir, ir_validation

    candidate_ir = dict(device_ir)
    candidate_ir["evidence_spans"] = repaired_payload.get("evidence_spans", [])
    repaired_validation = validate_ir(candidate_ir, source_lookup)
    if len(repaired_validation.issues) < len(ir_validation.issues):
        _write_json(
            run_dir / "device_ir_evidence_repair.json",
            {
                "applied": True,
                "issues_before": _issue_dicts(ir_validation),
                "issues_after": _issue_dicts(repaired_validation),
                "repaired_evidence_spans": repaired_payload.get("evidence_spans", []),
            },
        )
        return candidate_ir, repaired_validation

    _write_json(
        run_dir / "device_ir_evidence_repair.json",
        {
            "applied": False,
            "issues_before": _issue_dicts(ir_validation),
            "issues_after": _issue_dicts(repaired_validation),
            "repaired_evidence_spans": repaired_payload.get("evidence_spans", []),
        },
    )
    return device_ir, ir_validation


FLOW_AUDIT_CACHE_VERSION = "2026-05-05.flow-audit-p2-receipt-cache"
FLOW_AUDIT_CACHE_FORMAT = "device_ir_flow_audit_cache_v2"
FLOW_RISK_REPAIR_CACHE_VERSION = "2026-05-05.flow-risk-repair-p0"
FLOW_RISK_REPAIR_CACHE_FORMAT = "device_ir_flow_risk_repair_cache_v1"
FLOW_AUDIT_ALLOWED_FIELDS: tuple[str, ...] = (
    "access_model",
    "operation_flows",
    "read_channels",
    "init_sequence",
    "read_sequence",
    "timing_constraints",
    "conversion_formulae",
    "requires_human",
    "evidence_spans",
)
FLOW_AUDIT_CONTEXT_FIELDS: tuple[str, ...] = (
    "read_channels",
    "registers_or_commands",
    "init_sequence",
    "read_sequence",
    "timing_constraints",
    "conversion_formulae",
    "raw_encoding",
    "evidence_spans",
)


def _flow_audit_cache_key(
    audit_system: str,
    audit_user: str,
    current_device_ir: dict,
) -> str:
    schema_bytes = json.dumps(
        DEVICE_IR_FLOW_AUDIT_SCHEMA,
        sort_keys=True,
        ensure_ascii=False,
    ).encode("utf-8")
    schema_digest = hashlib.sha256(schema_bytes).hexdigest()[:16]
    ir_bytes = json.dumps(
        current_device_ir,
        sort_keys=True,
        ensure_ascii=False,
        default=str,
    ).encode("utf-8")
    ir_digest = hashlib.sha256(ir_bytes).hexdigest()[:16]
    payload = (
        f"{FLOW_AUDIT_CACHE_VERSION}\n"
        f"schema:{schema_digest}\n"
        f"device_ir:{ir_digest}\n"
        f"---system---\n{audit_system}\n"
        f"---user---\n{audit_user}\n"
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def _has_flow_audit_context(device_ir: object) -> bool:
    if not isinstance(device_ir, dict):
        return False
    for field in FLOW_AUDIT_CONTEXT_FIELDS:
        if field not in device_ir:
            return False
    channels = device_ir.get("read_channels")
    return isinstance(channels, list) and bool(channels)


def _merge_flow_audit_candidate(original: dict, candidate: object) -> dict:
    """Copy only flow-audit fields from a candidate full IR."""
    merged = dict(original)
    if not isinstance(candidate, dict):
        return merged
    for field in FLOW_AUDIT_ALLOWED_FIELDS:
        if field in candidate:
            merged[field] = candidate[field]
    return merged


def _changed_flow_audit_fields(before: dict, after: dict) -> list[str]:
    return [
        field for field in FLOW_AUDIT_ALLOWED_FIELDS
        if before.get(field) != after.get(field)
    ]


def _decode_flow_audit_cache_payload(payload: object) -> tuple[dict | None, dict]:
    """Return ``(device_ir, receipt)`` from wrapped or bare-IR cache payloads."""
    if not isinstance(payload, dict):
        return None, {}
    if isinstance(payload.get("device_ir"), dict):
        receipt = payload.get("receipt")
        return payload["device_ir"], receipt if isinstance(receipt, dict) else {}
    return payload, {}


def _flow_audit_cache_payload(final_ir: dict, receipt: dict) -> dict:
    receipt_keys = (
        "applied",
        "changed_fields",
        "violations_before_count",
        "violations_after_count",
        "violations_before",
        "violations_after",
        "audit_findings",
        "rejected_reason",
    )
    return {
        "format": FLOW_AUDIT_CACHE_FORMAT,
        "device_ir": final_ir,
        "receipt": {
            key: receipt.get(key)
            for key in receipt_keys
            if key in receipt
        },
    }


def _flow_risk_error_count(report_or_validation) -> int:
    if hasattr(report_or_validation, "issues"):
        issues = report_or_validation.issues
        return sum(1 for issue in issues if getattr(issue, "level", None) == "error")
    if isinstance(report_or_validation, dict):
        issues = report_or_validation.get("issues")
        if isinstance(issues, list):
            return sum(
                1 for issue in issues
                if isinstance(issue, dict) and issue.get("level") == "error"
            )
    return 0


def _flow_risk_issue_dicts(flow_risk_validation) -> list[dict]:
    return [
        {
            "level": issue.level,
            "check_id": issue.check_id,
            "message": issue.message,
        }
        for issue in flow_risk_validation.issues
        if issue.level == "error"
    ]


def _flow_risk_repair_cache_key(
    repair_system: str,
    repair_user: str,
    current_device_ir: dict,
    flow_risk_issues: list[dict],
) -> str:
    schema_bytes = json.dumps(
        DEVICE_IR_FLOW_AUDIT_SCHEMA,
        sort_keys=True,
        ensure_ascii=False,
    ).encode("utf-8")
    schema_digest = hashlib.sha256(schema_bytes).hexdigest()[:16]
    ir_bytes = json.dumps(
        current_device_ir,
        sort_keys=True,
        ensure_ascii=False,
        default=str,
    ).encode("utf-8")
    ir_digest = hashlib.sha256(ir_bytes).hexdigest()[:16]
    issue_bytes = json.dumps(
        flow_risk_issues,
        sort_keys=True,
        ensure_ascii=False,
        default=str,
    ).encode("utf-8")
    issue_digest = hashlib.sha256(issue_bytes).hexdigest()[:16]
    payload = (
        f"{FLOW_RISK_REPAIR_CACHE_VERSION}\n"
        f"schema:{schema_digest}\n"
        f"device_ir:{ir_digest}\n"
        f"issues:{issue_digest}\n"
        f"---system---\n{repair_system}\n"
        f"---user---\n{repair_user}\n"
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def _decode_flow_risk_repair_cache_payload(payload: object) -> tuple[dict | None, dict]:
    if not isinstance(payload, dict):
        return None, {}
    if isinstance(payload.get("device_ir"), dict):
        receipt = payload.get("receipt")
        return payload["device_ir"], receipt if isinstance(receipt, dict) else {}
    return payload, {}


def _flow_risk_repair_cache_payload(final_ir: dict, receipt: dict) -> dict:
    receipt_keys = (
        "applied",
        "changed_fields",
        "audit_findings",
        "risk_before",
        "risk_after",
        "risk_errors_before",
        "risk_errors_after",
        "violations_before_count",
        "violations_after_count",
        "rejected_reason",
    )
    return {
        "format": FLOW_RISK_REPAIR_CACHE_FORMAT,
        "device_ir": final_ir,
        "receipt": {
            key: receipt.get(key)
            for key in receipt_keys
            if key in receipt
        },
    }


def _audit_device_ir_flows_if_needed(
    *,
    provider,
    device_id: str,
    device_ir: dict,
    structured_content: str,
    extraction_plan: str,
    extraction_notes: str,
    run_dir: Path,
    audit_cache_dir: Path,
    use_llm_cache: bool,
    stage_report: dict,
) -> dict:
    """Run a model audit for operation-flow completeness."""

    receipt_path = run_dir / "device_ir_flow_audit.json"

    def _record(receipt: dict) -> None:
        _write_json(receipt_path, receipt)
        stage_report.setdefault("stage_c", {})["flow_audit"] = {
            key: receipt.get(key)
            for key in (
                "applied",
                "cache_hit",
                "skipped_reason",
                "error",
                "changed_fields",
                "violations_before_count",
                "violations_after_count",
            )
            if key in receipt
        }

    if not _has_flow_audit_context(device_ir):
        _record({
            "applied": False,
            "cache_hit": False,
            "skipped_reason": "device_ir lacks required full-IR context fields",
        })
        return device_ir

    audit_system, audit_user = build_device_ir_flow_audit_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=device_ir,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        allowed_edits=list(FLOW_AUDIT_ALLOWED_FIELDS),
    )
    audit_cache_key = _flow_audit_cache_key(audit_system, audit_user, device_ir)
    audit_cache_path = audit_cache_dir / f"{audit_cache_key}.json"

    if use_llm_cache and audit_cache_path.exists():
        try:
            cached_payload = json.loads(audit_cache_path.read_text(encoding="utf-8"))
            cached_ir, cached_receipt = _decode_flow_audit_cache_payload(cached_payload)
            if isinstance(cached_ir, dict):
                _stamp_ir_schema_version(cached_ir)
                changed_fields = _changed_flow_audit_fields(device_ir, cached_ir)
                receipt = dict(cached_receipt)
                receipt.update({
                    "applied": bool(changed_fields),
                    "cache_hit": True,
                    "cache_key": audit_cache_key,
                    "changed_fields": changed_fields,
                })
                receipt.setdefault("audit_findings", [])
                _record(receipt)
                return cached_ir
        except (OSError, ValueError) as exc:
            logger.warning(
                "Flow audit cache at %s is unreadable (%s); requesting a fresh result.",
                audit_cache_path, exc,
            )

    try:
        raw_candidate = provider.generate_json(
            task_name="audit_device_ir_flows",
            schema=DEVICE_IR_FLOW_AUDIT_SCHEMA,
            system_prompt=audit_system,
            user_prompt=audit_user,
            metadata=_llm_debug_metadata(
                run_dir,
                device_id=device_id,
                repair_target="device_ir_flow_audit",
                flow_audit_cache_key=audit_cache_key,
            ),
        )
    except Exception as exc:
        logger.warning("Flow audit for %s failed (%s); keeping original IR.", device_id, exc)
        _record({
            "applied": False,
            "cache_hit": False,
            "cache_key": audit_cache_key,
            "error": str(exc),
        })
        return device_ir

    audit_findings: list = []
    if isinstance(raw_candidate, dict) and isinstance(raw_candidate.get("device_ir"), dict):
        audit_findings = (
            raw_candidate.get("audit_findings")
            if isinstance(raw_candidate.get("audit_findings"), list)
            else []
        )
        candidate_payload = raw_candidate["device_ir"]
    else:
        # Backward-compatible guard for providers/tests that still return a
        # bare Device IR. The schema asks for a wrapper, but this keeps the
        # stage fail-soft if a backend ignores it.
        candidate_payload = raw_candidate

    candidate_ir = _merge_flow_audit_candidate(device_ir, candidate_payload)
    _stamp_ir_schema_version(candidate_ir)
    before_violations = detect_ir_prompt_violations(device_ir)
    after_violations = detect_ir_prompt_violations(candidate_ir)
    changed_fields = _changed_flow_audit_fields(device_ir, candidate_ir)
    accepted = bool(changed_fields) and len(after_violations) <= len(before_violations)
    final_ir = candidate_ir if accepted else device_ir

    receipt = {
        "applied": accepted,
        "cache_hit": False,
        "cache_key": audit_cache_key,
        "changed_fields": changed_fields,
        "violations_before_count": len(before_violations),
        "violations_after_count": len(after_violations),
        "violations_before": before_violations,
        "violations_after": after_violations,
        "audit_findings": audit_findings,
        "raw_candidate": raw_candidate,
        "merged_candidate": candidate_ir,
    }
    if changed_fields and not accepted:
        receipt["rejected_reason"] = "flow audit candidate increased prompt-rule violations"
    _record(receipt)

    if use_llm_cache:
        try:
            audit_cache_dir.mkdir(parents=True, exist_ok=True)
            _write_json(audit_cache_path, _flow_audit_cache_payload(final_ir, receipt))
        except OSError as exc:
            logger.warning("Flow audit cache write failed at %s: %s", audit_cache_path, exc)

    return final_ir


def _apply_formula_dependency_repair_if_needed(
    *,
    device_ir: dict,
    run_dir: Path,
    stage_report: dict,
) -> dict:
    changes = repair_formula_flow_dependencies(device_ir)
    if not changes:
        return device_ir
    _stamp_ir_schema_version(device_ir)
    receipt = {
        "applied": True,
        "changes": changes,
    }
    _write_json(run_dir / "device_ir_formula_dependency_repair.json", receipt)
    stage_report.setdefault("stage_c", {})["formula_dependency_repair"] = {
        "applied": True,
        "change_count": len(changes),
    }
    return device_ir


def _load_flow_audit_receipt(run_dir: Path) -> dict | None:
    path = run_dir / "device_ir_flow_audit.json"
    if not path.exists():
        return None
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        logger.warning("Could not read flow audit receipt at %s: %s", path, exc)
        return {
            "audit_findings": [],
            "read_error": str(exc),
        }
    return payload if isinstance(payload, dict) else None


def _repair_device_ir_flow_risk_if_needed(
    *,
    provider,
    device_id: str,
    device_ir: dict,
    structured_content: str,
    extraction_plan: str,
    extraction_notes: str,
    flow_audit_receipt: dict | None,
    flow_risk_validation,
    run_dir: Path,
    repair_cache_dir: Path,
    use_llm_cache: bool,
    stage_report: dict,
) -> tuple[dict, dict | None, object]:
    """Run one source-grounded repair when the deterministic risk gate fails."""

    if flow_risk_validation.ok:
        return device_ir, flow_audit_receipt, flow_risk_validation

    risk_issues = _flow_risk_issue_dicts(flow_risk_validation)
    if not risk_issues:
        return device_ir, flow_audit_receipt, flow_risk_validation

    repair_system, repair_user = build_device_ir_flow_risk_repair_prompt(
        device_id=device_id,
        structured_content=structured_content,
        current_device_ir=device_ir,
        flow_audit_receipt=flow_audit_receipt,
        flow_risk_issues=risk_issues,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        allowed_edits=list(FLOW_AUDIT_ALLOWED_FIELDS),
    )
    repair_cache_key = _flow_risk_repair_cache_key(
        repair_system,
        repair_user,
        device_ir,
        risk_issues,
    )
    repair_cache_path = repair_cache_dir / f"{repair_cache_key}.json"
    risk_before = flow_risk_validation.to_dict()
    errors_before = _flow_risk_error_count(flow_risk_validation)
    before_violations = detect_ir_prompt_violations(device_ir)

    raw_candidate = None
    cache_hit = False
    cached_receipt: dict = {}
    if use_llm_cache and repair_cache_path.exists():
        try:
            cached_payload = json.loads(repair_cache_path.read_text(encoding="utf-8"))
            cached_ir, cached_receipt = _decode_flow_risk_repair_cache_payload(cached_payload)
            if isinstance(cached_ir, dict):
                raw_candidate = {
                    "audit_findings": cached_receipt.get("audit_findings", []),
                    "device_ir": cached_ir,
                }
                cache_hit = True
        except (OSError, ValueError) as exc:
            logger.warning(
                "Flow-risk repair cache at %s is unreadable (%s); requesting a fresh result.",
                repair_cache_path, exc,
            )

    if raw_candidate is None:
        try:
            raw_candidate = provider.generate_json(
                task_name="repair_device_ir_flow_risk",
                schema=DEVICE_IR_FLOW_AUDIT_SCHEMA,
                system_prompt=repair_system,
                user_prompt=repair_user,
                metadata=_llm_debug_metadata(
                    run_dir,
                    device_id=device_id,
                    repair_target="device_ir_flow_risk",
                    flow_risk_repair_cache_key=repair_cache_key,
                ),
            )
        except Exception as exc:
            logger.warning("Flow-risk repair for %s failed (%s); keeping audited IR.", device_id, exc)
            receipt = {
                "applied": False,
                "cache_hit": False,
                "cache_key": repair_cache_key,
                "error": str(exc),
                "risk_before": risk_before,
            }
            _write_json(run_dir / "device_ir_flow_risk_repair.json", receipt)
            stage_report.setdefault("stage_c", {})["flow_risk_repair"] = {
                "applied": False,
                "cache_hit": False,
                "error": str(exc),
            }
            return device_ir, flow_audit_receipt, flow_risk_validation

    audit_findings: list = []
    if isinstance(raw_candidate, dict) and isinstance(raw_candidate.get("device_ir"), dict):
        audit_findings = (
            raw_candidate.get("audit_findings")
            if isinstance(raw_candidate.get("audit_findings"), list)
            else []
        )
        candidate_payload = raw_candidate["device_ir"]
    else:
        candidate_payload = raw_candidate

    candidate_ir = _merge_flow_audit_candidate(device_ir, candidate_payload)
    _stamp_ir_schema_version(candidate_ir)
    changed_fields = _changed_flow_audit_fields(device_ir, candidate_ir)
    candidate_audit_receipt = {
        "source": "flow_risk_repair",
        "cache_hit": cache_hit,
        "cache_key": repair_cache_key,
        "audit_findings": audit_findings,
        "previous_audit_cache_key": (
            flow_audit_receipt.get("cache_key")
            if isinstance(flow_audit_receipt, dict)
            else None
        ),
    }
    candidate_risk_validation = assess_ir_flow_risk(
        candidate_ir,
        candidate_audit_receipt,
        source_context=structured_content,
    )
    risk_after = candidate_risk_validation.to_dict()
    errors_after = _flow_risk_error_count(candidate_risk_validation)
    after_violations = detect_ir_prompt_violations(candidate_ir)
    accepted = (
        bool(audit_findings)
        and errors_after < errors_before
        and len(after_violations) <= len(before_violations)
    )
    final_ir = candidate_ir if accepted else device_ir
    final_receipt = candidate_audit_receipt if accepted else flow_audit_receipt
    final_risk = candidate_risk_validation if accepted else flow_risk_validation

    receipt = dict(cached_receipt) if cache_hit else {}
    receipt.update({
        "applied": accepted,
        "cache_hit": cache_hit,
        "cache_key": repair_cache_key,
        "changed_fields": changed_fields,
        "risk_issues_targeted": risk_issues,
        "risk_before": risk_before,
        "risk_after": risk_after,
        "risk_errors_before": errors_before,
        "risk_errors_after": errors_after,
        "violations_before_count": len(before_violations),
        "violations_after_count": len(after_violations),
        "audit_findings": audit_findings,
        "raw_candidate": raw_candidate,
        "merged_candidate": candidate_ir,
    })
    if not accepted:
        if not audit_findings:
            receipt["rejected_reason"] = "candidate did not return audit_findings"
        elif errors_after >= errors_before:
            receipt["rejected_reason"] = "candidate did not reduce flow-risk errors"
        else:
            receipt["rejected_reason"] = "candidate increased prompt-rule violations"
    _write_json(run_dir / "device_ir_flow_risk_repair.json", receipt)

    if accepted:
        final_receipt = dict(candidate_audit_receipt)
        final_receipt.update({
            "applied": True,
            "updated_by": "flow_risk_repair",
            "changed_fields": changed_fields,
        })
        _write_json(run_dir / "device_ir_flow_audit.json", final_receipt)

    if use_llm_cache and not cache_hit:
        try:
            repair_cache_dir.mkdir(parents=True, exist_ok=True)
            _write_json(
                repair_cache_path,
                _flow_risk_repair_cache_payload(final_ir, receipt),
            )
        except OSError as exc:
            logger.warning("Flow-risk repair cache write failed at %s: %s", repair_cache_path, exc)

    stage_report.setdefault("stage_c", {})["flow_risk_repair"] = {
        "applied": accepted,
        "cache_hit": cache_hit,
        "risk_errors_before": errors_before,
        "risk_errors_after": errors_after,
        "changed_fields": changed_fields,
    }
    return final_ir, final_receipt, final_risk


DATASHEET_PIPELINE_VERSION = "2026-04-20.1"


def _load_cache_meta(cache_dir: Path) -> dict:
    """Return the cached pipeline metadata, or an empty dict if missing/invalid."""
    meta_path = cache_dir / "cache_meta.json"
    if not meta_path.exists():
        return {}
    try:
        return json.loads(meta_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def _compute_pdf_sha256(pdf_path: Path) -> str:
    """Compute a streaming SHA-256 over the PDF bytes."""
    hasher = hashlib.sha256()
    try:
        with open(pdf_path, "rb") as fh:
            for chunk in iter(lambda: fh.read(1024 * 1024), b""):
                hasher.update(chunk)
    except OSError as exc:
        logger.warning("Cannot fingerprint %s: %s", pdf_path, exc)
        return ""
    return hasher.hexdigest()[:16]


def _cache_is_compatible(
    cache_dir: Path,
    *,
    bus_type: str,
    pdf_sha256: str,
    docling_config_fingerprint: str,
    relevance_prompt_template_version: str,
) -> tuple[bool, str | None]:
    """Check whether an existing cache matches every pinned component."""
    meta = _load_cache_meta(cache_dir)
    if not meta:
        return False, "no cache_meta.json"

    expected = {
        "pipeline_version": DATASHEET_PIPELINE_VERSION,
        "bus_type": bus_type,
        "pdf_sha256": pdf_sha256,
        "docling_config_fingerprint": docling_config_fingerprint,
        "relevance_prompt_template_version": relevance_prompt_template_version,
    }
    for key, expected_value in expected.items():
        cached_value = meta.get(key)
        # Missing fingerprint (empty string) counts as mismatch on both
        # Missing fingerprints are treated as incompatible on both sides.
        if not expected_value or cached_value != expected_value:
            return False, f"{key} mismatch (cache={cached_value!r}, current={expected_value!r})"
    return True, None


def _write_cache_meta(
    cache_dir: Path,
    *,
    bus_type: str,
    pdf_sha256: str,
    docling_config_fingerprint: str,
    relevance_prompt_template_version: str,
) -> None:
    """Stamp the cache directory with every component that gates compatibility."""
    meta = {
        "pipeline_version": DATASHEET_PIPELINE_VERSION,
        "bus_type": bus_type,
        "pdf_sha256": pdf_sha256,
        "docling_config_fingerprint": docling_config_fingerprint,
        "relevance_prompt_template_version": relevance_prompt_template_version,
        "stamped_at": datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
    }
    cache_dir.mkdir(parents=True, exist_ok=True)
    (cache_dir / "cache_meta.json").write_text(
        json.dumps(meta, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )


# Device IR result cache version. Bump it when schema changes invalidate cached payloads.
STAGE_C_IR_CACHE_VERSION = "2026-05-05.fact-c0"
STAGE_C0_FACT_CACHE_VERSION = "2026-05-05.fact-c0"


def _stage_c_ir_cache_key(ir_system: str, ir_user: str) -> str:
    """Derive a 16-hex-char cache key for Device IR extraction."""
    schema_bytes = json.dumps(DEVICE_IR_SCHEMA, sort_keys=True, ensure_ascii=False).encode("utf-8")
    schema_digest = hashlib.sha256(schema_bytes).hexdigest()[:16]
    payload = (
        f"{STAGE_C_IR_CACHE_VERSION}\n"
        f"schema:{schema_digest}\n"
        f"---system---\n{ir_system}\n"
        f"---user---\n{ir_user}\n"
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def _stage_c0_fact_cache_key(system_prompt: str, user_prompt: str) -> str:
    schema_bytes = json.dumps(DEVICE_IR_FACT_CANDIDATES_SCHEMA, sort_keys=True, ensure_ascii=False).encode("utf-8")
    schema_digest = hashlib.sha256(schema_bytes).hexdigest()[:16]
    payload = (
        f"{STAGE_C0_FACT_CACHE_VERSION}\n"
        f"schema:{schema_digest}\n"
        f"---system---\n{system_prompt}\n"
        f"---user---\n{user_prompt}\n"
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def _extract_device_ir_candidate_facts(
    *,
    provider,
    device_id: str,
    structured_content: str,
    extraction_plan: str,
    extraction_notes: str,
    run_dir: Path,
    fact_cache_dir: Path,
    use_llm_cache: bool,
    stage_report: dict[str, object],
) -> dict | None:
    """Run broad candidate-fact extraction."""

    t_c0 = time.monotonic()
    stage_report.setdefault("cache_hits", {})["stage_c0_fact_candidates"] = False
    candidate_system, candidate_user = build_device_ir_fact_candidate_prompt(
        device_id=device_id,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
    )
    candidate_cache_key = _stage_c0_fact_cache_key(candidate_system, candidate_user)
    candidate_cache_path = fact_cache_dir / f"{candidate_cache_key}.json"
    candidate_fact_bank: dict | None = None
    cache_hit = False

    if use_llm_cache and candidate_cache_path.exists():
        try:
            loaded = json.loads(candidate_cache_path.read_text(encoding="utf-8"))
            if isinstance(loaded, dict):
                candidate_fact_bank = loaded
                cache_hit = True
                stage_report["cache_hits"]["stage_c0_fact_candidates"] = True
                logger.info(
                    "Candidate facts: using cached result for %s (key=%s)",
                    device_id,
                    candidate_cache_key,
                )
        except (OSError, ValueError) as exc:
            logger.warning(
                "Candidate fact cache at %s is unreadable (%s); requesting a fresh result.",
                candidate_cache_path,
                exc,
            )
    elif not use_llm_cache:
        stage_report.setdefault("stage_c0", {})["cache_bypass"] = "use_llm_cache_false"

    if candidate_fact_bank is None:
        try:
            raw_candidate = provider.generate_json(
                task_name="extract_device_ir_fact_candidates",
                schema=DEVICE_IR_FACT_CANDIDATES_SCHEMA,
                system_prompt=candidate_system,
                user_prompt=candidate_user,
                metadata=_llm_debug_metadata(
                    run_dir,
                    device_id=device_id,
                    extraction_backend="docling_structured",
                    pipeline_step="fact_candidates",
                ),
            )
            if isinstance(raw_candidate, dict):
                candidate_fact_bank = raw_candidate
            else:
                raise TypeError(f"candidate fact response must be object, got {type(raw_candidate).__name__}")
            if use_llm_cache:
                try:
                    fact_cache_dir.mkdir(parents=True, exist_ok=True)
                    _write_json(candidate_cache_path, candidate_fact_bank)
                except OSError as exc:
                    logger.warning("Candidate fact cache write failed at %s: %s", candidate_cache_path, exc)
        except Exception as exc:
            logger.warning("Candidate fact extraction failed for %s: %s", device_id, exc)
            stage_report["stage_c0"] = {
                "ms": int((time.monotonic() - t_c0) * 1000),
                "candidate_cache_key": candidate_cache_key,
                "cache_hit": False,
                "error": str(exc),
                "applied_to_stage_c": False,
            }
            return None

    _write_json(run_dir / "device_ir_candidate_facts.json", candidate_fact_bank)
    candidate_summary = summarize_candidate_fact_bank(candidate_fact_bank)
    stage_report["stage_c0"] = {
        "ms": int((time.monotonic() - t_c0) * 1000),
        "candidate_cache_key": candidate_cache_key,
        "cache_hit": cache_hit,
        "summary": candidate_summary,
        "applied_to_stage_c": True,
    }
    return candidate_fact_bank


def extract_device_ir_structured(
    device_id: str,
    pdf_path: Path,
    provider,
    run_dir: Path,
    server_config=None,
    bus_type: str = "i2c",
    *,
    use_llm_cache: bool = True,
) -> dict:
    """Extract device IR using the Docling-based pipeline."""
    from ..datasheet.docling_backend import DoclingConfig, format_sections_for_prompt, parse_pdf
    from ..datasheet.relevance import (
        RELEVANCE_PROMPT_TEMPLATE_VERSION,
        apply_relevance_filter,
        assess_relevance,
        grouped_to_section_ids_by_relevance,
        select_extraction_section_ids,
    )
    from ..core.catalog import DATA_ROOT

    if server_config is None:
        server_config = DoclingConfig()

    bus_type_norm = (bus_type or "i2c").strip().lower() or "i2c"

    # Cache entries are scoped by every input that affects parsing and relevance.
    pdf_sha256 = _compute_pdf_sha256(pdf_path)
    docling_fingerprint = server_config.fingerprint()

    # Structured run report.
    stage_report: dict[str, object] = {
        "device_id": device_id,
        "bus_type": bus_type_norm,
        "pipeline_version": DATASHEET_PIPELINE_VERSION,
        "pdf_sha256": pdf_sha256,
        "docling_config_fingerprint": docling_fingerprint,
        "relevance_prompt_template_version": RELEVANCE_PROMPT_TEMPLATE_VERSION,
        "cache_hits": {"stage_a": False, "stage_b": False},
        "fallback": None,
        "stage_a": {},
        "stage_b": {},
        "stage_c": {},
    }
    t_total = time.monotonic()

    def _write_stage_report(final: bool = True) -> None:
        if final:
            stage_report["total_ms"] = int((time.monotonic() - t_total) * 1000)
        _write_json(run_dir / "stage_report.json", stage_report)

    # --- Cache directory (per device + bus) ---
    cache_dir = DATA_ROOT / "cache" / "docling" / device_id.lower() / bus_type_norm
    cache_compatible, cache_mismatch = _cache_is_compatible(
        cache_dir,
        bus_type=bus_type_norm,
        pdf_sha256=pdf_sha256,
        docling_config_fingerprint=docling_fingerprint,
        relevance_prompt_template_version=RELEVANCE_PROMPT_TEMPLATE_VERSION,
    )
    if not cache_compatible and cache_mismatch and cache_mismatch != "no cache_meta.json":
        logger.info(
            "Docling cache at %s is stale (%s); refreshing parsing and relevance outputs.",
            cache_dir, cache_mismatch,
        )
    stage_report["cache_mismatch_reason"] = cache_mismatch
    stage_report["cache_hits"]["stage_c"] = False
    ir_cache_dir = cache_dir / "ir"

    # --- Docling parsing (cached) ---
    cached_structured = cache_dir / "structured_document.json"
    cached_outline = cache_dir / "document_outline.json"
    cached_parse_metadata = cache_dir / "docling_parse_metadata.json"
    t_a = time.monotonic()

    if cache_compatible and cached_structured.exists() and cached_outline.exists():
        logger.info("Docling parsing: using cached output for %s (bus=%s)", device_id, bus_type_norm)
        structured_doc = json.loads(cached_structured.read_text(encoding="utf-8"))
        outline = json.loads(cached_outline.read_text(encoding="utf-8"))
        _write_json(run_dir / "structured_document.json", structured_doc)
        _write_json(run_dir / "document_outline.json", outline)
        if cached_parse_metadata.exists():
            try:
                parse_metadata = json.loads(cached_parse_metadata.read_text(encoding="utf-8"))
                _write_json(run_dir / "docling_parse_metadata.json", parse_metadata)
                stage_report["stage_a"]["parse_metadata"] = parse_metadata
            except (OSError, ValueError):
                stage_report["stage_a"]["parse_metadata_unreadable"] = True
        stage_report["cache_hits"]["stage_a"] = True
    else:
        try:
            docling_output_dir = run_dir / "docling_output"
            result = parse_pdf(pdf_path, server_config, docling_output_dir)
            structured_doc = result["structured_document"]
            outline = result["document_outline"]
            parse_metadata = result.get("parse_metadata") or {}
            _write_json(run_dir / "document_outline.json", outline)
            _write_json(run_dir / "structured_document.json", structured_doc)
            _write_json(run_dir / "docling_parse_metadata.json", parse_metadata)
            cache_dir.mkdir(parents=True, exist_ok=True)
            _write_json(cached_structured, structured_doc)
            _write_json(cached_outline, outline)
            _write_json(cached_parse_metadata, parse_metadata)
            stage_report["stage_a"]["parse_metadata"] = parse_metadata
            _write_cache_meta(
                cache_dir,
                bus_type=bus_type_norm,
                pdf_sha256=pdf_sha256,
                docling_config_fingerprint=docling_fingerprint,
                relevance_prompt_template_version=RELEVANCE_PROMPT_TEMPLATE_VERSION,
            )
            logger.info("Docling parsing: cached output for %s (bus=%s)", device_id, bus_type_norm)
        except Exception as exc:
            logger.warning("Docling parsing failed: %s.", exc)
            stage_report["stage_a"]["ms"] = int((time.monotonic() - t_a) * 1000)
            stage_report["stage_a"]["error"] = str(exc)
            stage_report["failed"] = "stage_a_docling_error"
            _write_stage_report()
            raise RuntimeError(f"Docling failed to parse {pdf_path}: {exc}") from exc

    stage_report["stage_a"]["ms"] = int((time.monotonic() - t_a) * 1000)
    stage_report["stage_a"]["total_sections"] = structured_doc.get("total_sections", 0)
    stage_report["stage_a"]["total_tables"] = structured_doc.get("total_tables", 0)
    stage_report["stage_a"]["total_pages"] = outline.get("total_pages")

    if structured_doc.get("total_sections", 0) == 0:
        msg = f"Docling produced 0 sections for {device_id}; the datasheet could not be segmented."
        logger.warning(msg)
        stage_report["failed"] = "stage_a_zero_sections"
        _write_stage_report()
        raise RuntimeError(msg)

    # --- Relevance assessment (cached; keyed on bus_type) ---
    cached_relevance = cache_dir / "relevance_map.json"
    t_b = time.monotonic()

    if use_llm_cache and cache_compatible and cached_relevance.exists():
        logger.info("Relevance assessment: using cached map for %s (bus=%s)", device_id, bus_type_norm)
        relevance_result = json.loads(cached_relevance.read_text(encoding="utf-8"))
        _write_json(run_dir / "relevance_map.json", relevance_result)
        stage_report["cache_hits"]["stage_b"] = True
        if relevance_result.get("stage_b_mode") == "fallback_rules":
            logger.warning(
                "Relevance cache for %s (bus=%s) was produced by the rule-based "
                "fallback (%s).",
                device_id,
                bus_type_norm,
                relevance_result.get("fallback_reason", "reason unknown"),
            )
        grouped = apply_relevance_filter(structured_doc, relevance_result)
        extraction_section_ids, low_retain_info = select_extraction_section_ids(grouped)
        extraction_plan = relevance_result.get("extraction_plan", "")
        extraction_notes = relevance_result.get("extraction_notes", "")
    else:
        if not use_llm_cache:
            stage_report["stage_b"]["cache_bypass"] = "use_llm_cache_false"
            logger.info(
                "Relevance cache bypassed for %s (bus=%s) by use_llm_cache=False",
                device_id, bus_type_norm,
            )
        low_retain_info = None
        try:
            relevance_result = assess_relevance(outline, device_id, provider, bus_type=bus_type_norm)
            _write_json(run_dir / "relevance_map.json", relevance_result)
            if use_llm_cache:
                cache_dir.mkdir(parents=True, exist_ok=True)
                _write_json(cached_relevance, relevance_result)
                _write_cache_meta(
                    cache_dir,
                    bus_type=bus_type_norm,
                    pdf_sha256=pdf_sha256,
                    docling_config_fingerprint=docling_fingerprint,
                    relevance_prompt_template_version=RELEVANCE_PROMPT_TEMPLATE_VERSION,
                )
                logger.info("Relevance assessment: cached map for %s (bus=%s)", device_id, bus_type_norm)
            else:
                logger.info(
                    "Relevance assessment: skipped persistent cache write for %s (bus=%s) "
                    "because use_llm_cache=False",
                    device_id, bus_type_norm,
                )
            grouped = apply_relevance_filter(structured_doc, relevance_result)
            extraction_section_ids, low_retain_info = select_extraction_section_ids(grouped)
            extraction_plan = relevance_result.get("extraction_plan", "")
            extraction_notes = relevance_result.get("extraction_notes", "")
        except Exception as exc:
            logger.warning("Relevance assessment failed: %s. Using all sections.", exc)
            stage_report["stage_b"]["error"] = str(exc)
            extraction_section_ids = None
            extraction_plan = ""
            extraction_notes = ""
            grouped = None
            relevance_result = None

    stage_report["stage_b"]["ms"] = int((time.monotonic() - t_b) * 1000)
    if relevance_result is not None:
        stage_report["stage_b"]["stage_b_mode"] = relevance_result.get("stage_b_mode")
        if relevance_result.get("stage_b_mode") == "fallback_rules":
            stage_report["stage_b"]["fallback_reason"] = relevance_result.get("fallback_reason")
    stage_report["stage_b"]["sections_kept_high"] = len(grouped.get("high", [])) if grouped else 0
    stage_report["stage_b"]["sections_kept_medium"] = len(grouped.get("medium", [])) if grouped else 0
    stage_report["stage_b"]["sections_kept_low_retained"] = len(grouped.get("low_retained", [])) if grouped else 0
    stage_report["stage_b"]["sections_filtered"] = len(grouped.get("filtered", [])) if grouped else 0
    if low_retain_info is not None:
        stage_report["stage_b"]["low_retained_policy"] = low_retain_info

    # --- Schema-guided structured extraction ---
    section_ids_by_relevance = grouped_to_section_ids_by_relevance(grouped) if grouped else None
    t_c = time.monotonic()
    structured_content, stage_c_stats = format_sections_for_prompt(
        structured_doc,
        section_ids=extraction_section_ids,
        section_ids_by_relevance=section_ids_by_relevance,
        max_chars=server_config.stage_c_max_chars,
        drop_sections_without_page=True,
        return_stats=True,
    )
    _write_text(run_dir / "structured_prompt_preview.txt", structured_content)

    candidate_fact_bank = _extract_device_ir_candidate_facts(
        provider=provider,
        device_id=device_id,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        run_dir=run_dir,
        fact_cache_dir=ir_cache_dir / "fact_candidates",
        use_llm_cache=use_llm_cache,
        stage_report=stage_report,
    )
    candidate_fact_summary = format_candidate_fact_summary_for_ir_prompt(candidate_fact_bank)
    if candidate_fact_summary:
        _write_text(run_dir / "device_ir_candidate_fact_summary_for_prompt.txt", candidate_fact_summary)

    ir_system, ir_user = build_structured_ir_prompt(
        device_id=device_id,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        candidate_fact_summary=candidate_fact_summary,
        target_bus_type=bus_type,
    )

    # Device IR cache is keyed on the exact prompt input and schema fingerprint.
    ir_cache_key = _stage_c_ir_cache_key(ir_system, ir_user)
    ir_cache_path = ir_cache_dir / f"{ir_cache_key}.json"
    device_ir = None
    if use_llm_cache and ir_cache_path.exists():
        try:
            device_ir = json.loads(ir_cache_path.read_text(encoding="utf-8"))
            stage_report["cache_hits"]["stage_c"] = True
            logger.info("Device IR extraction: using cached result for %s (key=%s)", device_id, ir_cache_key)
        except (OSError, ValueError) as exc:
            logger.warning("Device IR cache at %s is unreadable (%s); requesting a fresh result.", ir_cache_path, exc)
            device_ir = None
    elif not use_llm_cache:
        stage_report["stage_c"]["cache_bypass"] = "use_llm_cache_false"
        logger.info(
            "Device IR extraction: cache bypassed for %s (key=%s) by use_llm_cache=False",
            device_id, ir_cache_key,
        )

    if device_ir is None:
        device_ir = provider.generate_json(
            task_name="extract_device_ir",
            schema=DEVICE_IR_SCHEMA,
            system_prompt=ir_system,
            user_prompt=ir_user,
            metadata=_llm_debug_metadata(
                run_dir,
                device_id=device_id,
                extraction_backend="docling_structured",
            ),
        )
        if use_llm_cache:
            try:
                ir_cache_dir.mkdir(parents=True, exist_ok=True)
                _write_json(ir_cache_path, device_ir)
                logger.info("Device IR extraction: cached result for %s (key=%s)", device_id, ir_cache_key)
            except OSError as exc:
                logger.warning("Device IR cache write failed at %s: %s", ir_cache_path, exc)
        else:
            logger.info(
                "Device IR extraction: skipped persistent cache write for %s (key=%s) "
                "because use_llm_cache=False",
                device_id, ir_cache_key,
            )

    stage_report["stage_c"]["ms"] = int((time.monotonic() - t_c) * 1000)
    stage_report["stage_c"]["prompt_stats"] = stage_c_stats
    stage_report["stage_c"]["extraction_backend"] = "docling_structured"
    stage_report["stage_c"]["ir_cache_key"] = ir_cache_key

    # Stamp the current IR schema version before returning.
    _stamp_ir_schema_version(
        device_ir,
        target_bus_type=bus_type,
        target_device_id=device_id,
    )
    device_ir = _audit_device_ir_flows_if_needed(
        provider=provider,
        device_id=device_id,
        device_ir=device_ir,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        run_dir=run_dir,
        audit_cache_dir=ir_cache_dir / "flow_audit",
        use_llm_cache=use_llm_cache,
        stage_report=stage_report,
    )
    _stamp_ir_schema_version(
        device_ir,
        target_bus_type=bus_type,
        target_device_id=device_id,
    )
    device_ir = _apply_formula_dependency_repair_if_needed(
        device_ir=device_ir,
        run_dir=run_dir,
        stage_report=stage_report,
    )
    flow_audit_receipt = _load_flow_audit_receipt(run_dir)
    flow_risk_validation = assess_ir_flow_risk(
        device_ir,
        flow_audit_receipt,
        source_context=structured_content,
    )
    device_ir, _flow_audit_receipt, _flow_risk_validation = _repair_device_ir_flow_risk_if_needed(
        provider=provider,
        device_id=device_id,
        device_ir=device_ir,
        structured_content=structured_content,
        extraction_plan=extraction_plan,
        extraction_notes=extraction_notes,
        flow_audit_receipt=flow_audit_receipt,
        flow_risk_validation=flow_risk_validation,
        run_dir=run_dir,
        repair_cache_dir=ir_cache_dir / "flow_risk_repair",
        use_llm_cache=use_llm_cache,
        stage_report=stage_report,
    )
    _stamp_ir_schema_version(
        device_ir,
        target_bus_type=bus_type,
        target_device_id=device_id,
    )
    _write_device_ir_fact_bank(device_ir, run_dir, stage_report, candidate_fact_bank=candidate_fact_bank)

    _write_stage_report()
    return device_ir


def _build_task_package(board_context: dict, fixed_context: dict | None) -> dict:
    """Flatten board + fixed context into the shape expected by classifier/router."""
    tp: dict = {}
    bus = board_context.get("bus_type")
    if bus:
        tp["bus_type"] = bus
    if fixed_context:
        connection = fixed_context.get("connection_binding_context") or {}
        device_attachment = fixed_context.get("device_attachment_context") or {}
        task_package = fixed_context.get("task_package") or {}
        fixed_task_context = task_package.get("fixed_task_context") or {}
        fixed_device = fixed_task_context.get("device") or {}
        if connection:
            tp["connection_binding"] = connection
        if connection.get("bus_type"):
            tp["bus_type"] = connection["bus_type"]
        elif connection.get("connection_type"):
            tp["bus_type"] = connection["connection_type"]
        protocol_hints: dict[str, object] = {}
        for source in (
            connection,
            device_attachment.get("protocol_hints") if isinstance(device_attachment, dict) else {},
            fixed_device.get("protocol_hints") if isinstance(fixed_device, dict) else {},
        ):
            if isinstance(source, dict):
                protocol_hints.update(source)
        if protocol_hints:
            tp["protocol_hints"] = dict(protocol_hints)
        if protocol_hints.get("spi_proto"):
            tp["spi_proto"] = protocol_hints["spi_proto"]
        if protocol_hints.get("gpio_protocol_hint"):
            tp["gpio_protocol_hint"] = protocol_hints["gpio_protocol_hint"]
        fixed_attachment = connection.get("fixed_attachment")
        if isinstance(fixed_attachment, dict):
            tp["fixed_attachment"] = dict(fixed_attachment)
            for key in (
                "trig_pin",
                "trig_line",
                "trig_path",
                "echo_pin",
                "echo_line",
                "echo_path",
            ):
                if key in fixed_attachment:
                    tp[key] = fixed_attachment[key]
    return tp


def _save_codegen_outputs(
    run_dir: Path,
    result,
) -> dict:
    """Persist the best attempt's driver/adapter outputs to ``run_dir``."""
    files_written: dict[str, int] = {}
    if not result.success and result.final_bundle is None:
        return files_written

    dev = (result.device_id or "device").lower()
    if result.final_bundle is not None:
        if result.final_bundle.driver_header.strip():
            p = run_dir / f"{dev}.h"
            p.write_text(result.final_bundle.driver_header, encoding="utf-8")
            files_written[p.name] = len(result.final_bundle.driver_header)
        if result.final_bundle.driver_source.strip():
            p = run_dir / f"{dev}.c"
            p.write_text(result.final_bundle.driver_source, encoding="utf-8")
            files_written[p.name] = len(result.final_bundle.driver_source)
    if result.final_adapter is not None and result.final_adapter.source_c.strip():
        p = run_dir / f"{dev}_eval_adapter.c"
        p.write_text(result.final_adapter.source_c, encoding="utf-8")
        files_written[p.name] = len(result.final_adapter.source_c)

    return files_written


def _summarise_probe_outcomes(result) -> dict:
    """Collapse the best attempt's probe outcomes into a report-friendly summary."""
    best = None
    if result.attempts:
        for a in result.attempts:
            if a.attempt == result.final_attempt:
                best = a
                break
        if best is None:
            best = result.attempts[-1]

    if best is None or not best.probe_outcomes:
        return {"total": 0, "pass": 0, "fail": 0, "error": 0}

    total = len(best.probe_outcomes)
    passed = sum(
        1 for o in best.probe_outcomes
        if getattr(o, "boot_detected", False)
        and getattr(o, "test_done", False)
        and getattr(o, "result_pass", False)
    )
    errors = sum(1 for o in best.probe_outcomes if getattr(o, "error", "") or "")
    failed = total - passed - errors
    if failed < 0:
        failed = 0
    return {"total": total, "pass": passed, "fail": failed, "error": errors}


def _run_codegen_stage(
    *,
    provider,
    device_ir: dict,
    rtos_contract: dict,
    artifact: dict | None,
    task_package: dict,
    run_dir: Path,
    max_repairs: int,
    skip_compile: bool,
    run_renode: bool,
) -> dict:
    """Drive pipeline step / S3 / S3.1 / S8 end-to-end and persist outputs."""
    logger.info("=== Code generation: classify, route, and repair ===")

    classify_result = classify_device(device_ir, task_package=task_package)
    _write_json(run_dir / "classify_result.json", dataclasses.asdict(classify_result))

    routing = route(device_ir, task_package, classify_result)
    _write_json(run_dir / "routing_result.json", dataclasses.asdict(routing))

    expected_txs = derive_expected_transactions(
        device_ir,
        classify_result,
        task_package=task_package,
    )
    _write_json(
        run_dir / "expected_transactions.json",
        [tx.to_dict() for tx in expected_txs],
    )

    result = run_repair_loop(
        provider,
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        expected_transactions=expected_txs,
        artifact=artifact,
        task_package=task_package,
        max_attempts=max(1, max_repairs + 1),
        run_dir=run_dir,
        # Runtime probing remains gated behind the explicit Renode flag.
        skip_runtime=skip_compile,
        skip_syntax=skip_compile,
        skip_probe=not run_renode,
    )

    files_written = _save_codegen_outputs(run_dir, result)
    probe_summary = _summarise_probe_outcomes(result)

    return {
        "schema": "driver_codegen",
        "success": result.success,
        "total_time_s": round(result.total_time_s, 3),
        "final_attempt": result.final_attempt,
        "eval_class": result.eval_class,
        "bus_kind": result.bus_kind,
        "runtime_path": routing.runtime_path,
        "slave_kind": routing.slave_kind,
        "spi_sub_mode": routing.spi_sub_mode or "",
        "layer_failed": result.layer_failed,
        "final_elf_path": str(result.final_elf_path) if result.final_elf_path else None,
        "files_written": files_written,
        "probe": probe_summary,
        "invariants_count": len(result.invariants),
        "attempts": [a.to_dict() for a in result.attempts],
    }


def run_task_package(
    task_package: str | Path,
    *,
    provider: str,
    model: str,
    output_root: Path | None = None,
    skip_codegen: bool = True,
    max_repairs: int = 2,
    artifact_path: Path | None = None,
    skip_compile: bool = False,
    run_renode: bool = True,
    evaluation_hook=None,
    disable_llm_cache: bool = False,
    reuse_ir_path: Path | str | None = None,
) -> dict:
    """Run the pipeline from a fixed task package id/path."""
    return run_pipeline(
        run_config_from_task_package(task_package, provider=provider, model=model),
        output_root=output_root,
        skip_codegen=skip_codegen,
        max_repairs=max_repairs,
        artifact_path=artifact_path,
        skip_compile=skip_compile,
        run_renode=run_renode,
        evaluation_hook=evaluation_hook,
        disable_llm_cache=disable_llm_cache,
        reuse_ir_path=reuse_ir_path,
    )


def run_pipeline(
    config: PipelineRunConfig,
    output_root: Path | None = None,
    *,
    skip_codegen: bool = True,
    max_repairs: int = 2,
    artifact_path: Path | None = None,
    skip_compile: bool = False,
    run_renode: bool = True,
    evaluation_hook=None,
    disable_llm_cache: bool = False,
    reuse_ir_path: Path | str | None = None,
) -> dict:
    """Run the agentless pipeline for one resolved task-package config."""
    provider = create_provider(config.provider_name, config.provider_model)
    prepared = _prepare_run_context(config)
    run_config = config

    rtos_profile = prepared["rtos_profile"]
    board_context = prepared["board_context"]
    source_lookup = prepared["source_lookup"]

    run_dir = output_root or (RUNS_ROOT / _run_config_run_name(config, board_context, provider.name))
    run_dir.mkdir(parents=True, exist_ok=True)

    # Persist task metadata first so debugging earlier-stage
    # failures still has the input pinned on disk.
    _write_json(run_dir / "run_config.json", config.to_dict())
    _write_json(run_dir / "rtos_manifest_entry.json", rtos_profile.manifest_entry)
    _write_json(run_dir / "board_context.json", board_context)
    _write_json(run_dir / "target_binding.json", prepared["target_binding"])
    if prepared["fixed_context"] is not None:
        _write_json(run_dir / "fixed_task_context.json", prepared["fixed_context"])

    if reuse_ir_path is not None:
        # Reuse cached device IR only; RTOS extraction still runs fresh.
        logger.info("=== Reusing cached device IR from %s ===", reuse_ir_path)
        reuse_ir = Path(reuse_ir_path)
        if not reuse_ir.exists():
            raise FileNotFoundError(f"Reuse IR not found: {reuse_ir_path}")
        device_ir = json.loads(reuse_ir.read_text(encoding="utf-8"))
        _stamp_ir_schema_version(
            device_ir,
            target_bus_type=board_context.get("bus_type"),
            target_device_id=run_config.device_id,
        )
        _write_json(run_dir / "device_ir.json", device_ir)
        _write_device_ir_fact_bank(device_ir, run_dir)

        ir_validation = validate_ir(device_ir, source_lookup)
        # Evidence repair only — schema issues are already fixed in cached IR.
        if not ir_validation.ok:
            device_ir, ir_validation = _repair_device_ir_evidence_if_needed(
                provider=provider,
                device_id=run_config.device_id,
                device_ir=device_ir,
                source_lookup=source_lookup,
                run_dir=run_dir,
            )
        _stamp_ir_schema_version(
            device_ir,
            target_bus_type=board_context.get("bus_type"),
            target_device_id=run_config.device_id,
        )
        _write_json(run_dir / "device_ir.json", device_ir)

        # RTOS contract construction.
        logger.info("=== RTOS context extraction ===")
        rtos_contract, rtos_artifact, extraction_result = _build_rtos_extraction(
            prepared,
            device_ir=device_ir,
            provider=provider,
            run_dir=run_dir,
            use_evidence_cache=not disable_llm_cache,
        )
        if artifact_path is None:
            artifact_path = run_dir / "rtos_artifact.json"
    else:
        # Device IR extraction must run before RTOS extraction so transaction templates
        # and timing slots can be derived from read_sequence.
        device_ir = extract_device_ir_structured(
            device_id=run_config.device_id,
            pdf_path=run_config.pdf_path,
            provider=provider,
            run_dir=run_dir,
            bus_type=board_context.get("bus_type", "i2c"),
            use_llm_cache=not disable_llm_cache,
        )
        structured_doc_path = run_dir / "structured_document.json"
        if structured_doc_path.exists():
            structured_document = json.loads(structured_doc_path.read_text(encoding="utf-8"))
            outline_path = run_dir / "document_outline.json"
            outline_total_pages = None
            if outline_path.exists():
                try:
                    outline_payload = json.loads(outline_path.read_text(encoding="utf-8"))
                    candidate = outline_payload.get("total_pages")
                    if isinstance(candidate, int) and not isinstance(candidate, bool) and candidate > 0:
                        outline_total_pages = candidate
                except (OSError, ValueError):
                    outline_total_pages = None
            source_lookup = build_source_lookup(
                run_config.device_id,
                [],
                prepared["context_sources"],
                structured_document=structured_document,
                total_pages=outline_total_pages,
            )
        device_ir, ir_validation = _repair_device_ir_evidence_if_needed(
            provider=provider,
            device_id=run_config.device_id,
            device_ir=device_ir,
            source_lookup=source_lookup,
            run_dir=run_dir,
        )
        # If evidence repair still leaves schema issues, run one targeted retry.
        if not ir_validation.ok:
            device_ir, ir_validation = _repair_device_ir_schema_if_needed(
                provider=provider,
                device_id=run_config.device_id,
                device_ir=device_ir,
                source_lookup=source_lookup,
                run_dir=run_dir,
            )
        # Stamping right before persistence guarantees every on-disk
        # ``device_ir.json`` carries the active schema version.
        _stamp_ir_schema_version(
            device_ir,
            target_bus_type=board_context.get("bus_type"),
            target_device_id=run_config.device_id,
        )
        _write_json(run_dir / "device_ir.json", device_ir)
        _write_device_ir_fact_bank(device_ir, run_dir)

        # RTOS contract construction.
        logger.info("=== RTOS context extraction ===")
        rtos_contract, rtos_artifact, extraction_result = _build_rtos_extraction(
            prepared,
            device_ir=device_ir,
            provider=provider,
            run_dir=run_dir,
            use_evidence_cache=not disable_llm_cache,
        )
        if artifact_path is None:
            artifact_path = run_dir / "rtos_artifact.json"

    # Surface artifact validation alongside IR validation in the final report.
    evidence_validation = rtos_artifact.get("validation") or {}
    ir_report = ir_validation.to_dict()
    ir_checks_passed = ir_validation.ok
    flow_audit_receipt = _load_flow_audit_receipt(run_dir)
    flow_risk_validation = assess_ir_flow_risk(
        device_ir,
        flow_audit_receipt,
        source_context=_read_optional_text(run_dir / "structured_prompt_preview.txt"),
    )
    flow_risk_report = flow_risk_validation.to_dict()
    _write_json(run_dir / "device_ir_flow_risk.json", flow_risk_report)
    if reuse_ir_path is not None:
        ir_report["informational_only"] = True
        ir_report["source"] = "cached_device_ir"
        ir_checks_passed = True
        if flow_audit_receipt is None:
            flow_risk_report["informational_only"] = True
            flow_risk_report["source"] = "cached_device_ir"
            flow_risk_report["ok"] = True
            flow_risk_validation.ok = True
    report = {
        "run_id": run_config.run_id,
        "device_id": run_config.device_id,
        "rtos_id": run_config.rtos_id,
        "provider": provider.name,
        "run_dir": str(run_dir),
        "pipeline": "DriverGen",
        "context_mode": "rtos-extraction",
        "checks": {
            "rtos_evidence_validation": evidence_validation,
            "ir": ir_report,
            "ir_flow_risk": flow_risk_report,
        },
        "all_checks_passed": (
            ir_checks_passed
            and flow_risk_validation.ok
            and not evidence_validation.get("failed_gates")
        ),
    }
    _write_json(run_dir / "validation_report.json", report)

    # Optional code generation with repair loop.
    if not skip_codegen:
        artifact = rtos_artifact

        task_package = _build_task_package(board_context, prepared["fixed_context"])
        _write_json(run_dir / "task_package.json", task_package)
        if not flow_risk_validation.ok:
            codegen_payload = {
                "schema": "driver_codegen",
                "success": False,
                "skipped_reason": "ir_flow_risk_failed",
                "flow_risk": flow_risk_report,
            }
        else:
            codegen_payload = _run_codegen_stage(
                provider=provider,
                device_ir=device_ir,
                rtos_contract=rtos_contract,
                artifact=artifact,
                task_package=task_package,
                run_dir=run_dir,
                max_repairs=max_repairs,
                skip_compile=skip_compile,
                run_renode=run_renode,
            )
        report["codegen"] = codegen_payload
        codegen_success = codegen_payload["success"]

        report["all_checks_passed"] = report["all_checks_passed"] and codegen_success

        if evaluation_hook is not None and codegen_success:
            try:
                eval_summary = evaluation_hook(run_dir, run_config.device_id, run_config.rtos_id, report)
                if eval_summary is not None:
                    report["evaluation"] = eval_summary
                    overall = eval_summary.get("overall_claim") if isinstance(eval_summary, dict) else None
                    report["all_checks_passed"] = (
                        report["all_checks_passed"]
                        and overall in ("semantic-valid", "robust-valid")
                    )
            except Exception as exc:  # pragma: no cover - evaluation failures are non-fatal
                report["evaluation"] = {"ok": False, "error": str(exc)}

        _write_json(run_dir / "validation_report.json", report)

    return report
