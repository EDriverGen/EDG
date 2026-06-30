"""pipeline step - task-level :class:`RtosEvidenceArtifact` cache."""

from __future__ import annotations

import hashlib
import json
import logging
import os
import tempfile
from pathlib import Path

from . import ARTIFACT_VERSION
from .types import RepoIndexBundle, SlotPlan, TaskSpec

logger = logging.getLogger(__name__)

_DEFAULT_CACHE_ROOT = Path("data/cache/rtos_evidence")

# Cache schema version for evidence artifacts.
_RANKER_BINDER_VERSION = "1.0"


def _slot_plan_signature(slot_plan: SlotPlan) -> str:
    """Stable digest of the slot ids + required flags + canonical buses + source_kinds_allowed + query_intents."""
    parts: list[str] = []
    for s in sorted(slot_plan.slots, key=lambda x: x.slot_id):
        parts.append("|".join([
            s.slot_id,
            "1" if s.required else "0",
            (s.canonical_bus or "").lower(),
            ",".join(sorted(s.expected_kinds or [])),
            ",".join(sorted(s.source_kinds_allowed or [])),
            (s.origin or "").lower(),
            ",".join(sorted(s.query_intents or [])),
        ]))
    blob = "\n".join(parts).encode("utf-8")
    return hashlib.sha1(blob).hexdigest()[:16]


def task_cache_key(
    *,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    bundle: RepoIndexBundle | None = None,
    device_ir_hash: str | None = None,
    artifact_version: str = ARTIFACT_VERSION,
) -> str:
    """Compose the on-disk cache key from task + slot-plan + bundle versions."""
    bi = task_spec.bus_intent
    parts = [
        artifact_version,
        _RANKER_BINDER_VERSION,
        (task_spec.rtos_id or "").lower(),
        (task_spec.board or "").lower(),
        (task_spec.mcu_family or "").lower(),
        (task_spec.integration or "").lower(),
        (task_spec.integration_style or "").lower(),
        (bi.canonical_bus or "").lower(),
        (bi.connection_type or "").lower(),
        (bi.mode or "").lower(),
        (bi.address_mode or "").lower(),
        (bi.bus_instance or "").lower(),
        (task_spec.device_id or "").lower(),
        (task_spec.device_transaction_shape or "").lower(),
        device_ir_hash or "",
        _slot_plan_signature(slot_plan),
        (bundle.scope_map_hash if bundle else "") or "",
        (bundle.indexer_version if bundle else "") or "",
    ]
    blob = "|".join(parts).encode("utf-8")
    return hashlib.sha1(blob).hexdigest()[:16]


def evidence_cache_dir_for(
    *,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    bundle: RepoIndexBundle | None = None,
    device_ir_hash: str | None = None,
    cache_root: Path | None = None,
    artifact_version: str = ARTIFACT_VERSION,
) -> Path:
    """Directory for this task's cached artifact.  Does not create it.

    Layout: ``<cache_root>/<artifact_version>/<task_cache_key>/``.
    """
    h = task_cache_key(
        task_spec=task_spec, slot_plan=slot_plan, bundle=bundle,
        device_ir_hash=device_ir_hash, artifact_version=artifact_version,
    )
    base = cache_root or _DEFAULT_CACHE_ROOT
    return base / artifact_version / h


def _atomic_write_text(path: Path, content: str) -> None:
    """Write *content* to *path* via tempfile + ``os.replace``."""
    fd, tmp_name = tempfile.mkstemp(
        prefix=path.name + ".",
        suffix=".tmp",
        dir=str(path.parent),
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(content)
        os.replace(tmp_name, path)
    except Exception:
        try:
            os.remove(tmp_name)
        except OSError:
            pass
        raise


def save_evidence_artifact(
    *,
    artifact: dict,
    cache_dir: Path,
    descriptor_extra: dict | None = None,
) -> Path:
    """Atomically write *artifact* to ``cache_dir/rtos_artifact.json``
    plus a small ``cache_descriptor.json`` recording task / slot_plan
    fingerprints so cache hits are inspectable from the filesystem.
    """
    cache_dir.mkdir(parents=True, exist_ok=True)
    art_path = cache_dir / "rtos_artifact.json"
    desc_path = cache_dir / "cache_descriptor.json"

    _atomic_write_text(art_path, json.dumps(artifact, ensure_ascii=False, indent=2))

    descriptor = {
        "artifact_version": artifact.get("version") or ARTIFACT_VERSION,
        "task_spec": artifact.get("task_spec") or {},
        "summary": artifact.get("summary") or {},
        "n_symbols": len(artifact.get("symbols") or {}),
        "n_slots": len(artifact.get("slots") or {}),
    }
    if descriptor_extra:
        descriptor["extra"] = descriptor_extra
    _atomic_write_text(desc_path, json.dumps(descriptor, ensure_ascii=False, indent=2))

    logger.info(
        "Wrote evidence cache to %s (artifact %d KB)",
        cache_dir, art_path.stat().st_size // 1024,
    )
    return art_path


def load_evidence_artifact(
    cache_dir: Path,
    *,
    expected_artifact_version: str | None = None,
) -> dict | None:
    """Try to load a cached artifact."""
    art_path = cache_dir / "rtos_artifact.json"
    if not art_path.exists():
        return None
    try:
        artifact = json.loads(art_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        logger.warning(
            "Cached evidence artifact at %s is corrupt: %s", art_path, exc
        )
        return None
    if expected_artifact_version and artifact.get("version") != expected_artifact_version:
        logger.info(
            "Cached evidence artifact at %s has version %r; expected %r",
            art_path, artifact.get("version"), expected_artifact_version,
        )
        return None
    return artifact


__all__ = [
    "task_cache_key",
    "evidence_cache_dir_for",
    "save_evidence_artifact",
    "load_evidence_artifact",
]
