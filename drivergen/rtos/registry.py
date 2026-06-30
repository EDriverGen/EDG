"""RTOS registry and profile helpers for DriverGen."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from ..core.catalog import DATA_ROOT, PROJECT_ROOT
from .aliases import canonicalize_rtos_id as _canonicalize_rtos_id

RTOS_MANIFEST_PATH = DATA_ROOT / "rtos" / "manifest.json"
REFERENCES_ROOT = DATA_ROOT / "references"


@dataclass(frozen=True)
class RtosProfile:
    """Resolved RTOS metadata for the agentless pipeline."""

    rtos_id: str
    display_name: str
    repo_root: Path
    manifest_entry: dict
    implemented: bool
    unsupported_reason: str | None
    reference_root: Path | None


def _load_manifest() -> dict[str, dict]:
    payload = json.loads(RTOS_MANIFEST_PATH.read_text(encoding="utf-8"))
    repositories = payload.get("repositories", [])
    return {entry["id"]: entry for entry in repositories}


def _resolve_repo_path(path_text: str) -> Path:
    path = Path(path_text)
    if path.is_absolute():
        return path

    candidates: list[Path] = []
    if path.parts and path.parts[0].lower() == PROJECT_ROOT.name.lower():
        trimmed = Path(*path.parts[1:])
        candidates.append((PROJECT_ROOT / trimmed).resolve())
        candidates.append((PROJECT_ROOT.parent / trimmed).resolve())
    candidates.append((PROJECT_ROOT / path).resolve())
    candidates.append((PROJECT_ROOT.parent / path).resolve())

    seen: set[Path] = set()
    for candidate in candidates:
        if candidate in seen:
            continue
        seen.add(candidate)
        if candidate.exists():
            return candidate
    return candidates[0]


def canonical_rtos_id(value: str | None) -> str:
    """Map user-facing RTOS ids (e.g."""
    return _canonicalize_rtos_id(value)


# Built-in registry entries.
_RTOS_SPECS: dict[str, dict] = {
    "rtthread": {
        "manifest_id": "rt-thread",
        "display_name": "RT-Thread",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "freertos": {
        "manifest_id": "freertos",
        "display_name": "FreeRTOS bundle",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "threadx": {
        "manifest_id": "threadx",
        "display_name": "ThreadX bundle",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "tobudos": {
        "manifest_id": "tobudos-project",
        "display_name": "TobudOS + ChipAdaptation",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "openharmony-liteosm": {
        "manifest_id": "openharmony-liteosm-project",
        "display_name": "OpenHarmony LiteOS-M + HDF Core",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "xiuos": {
        "manifest_id": "xiuos",
        "display_name": "XiUOS",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "zephyr": {
        "manifest_id": "zephyr",
        "display_name": "Zephyr",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "nuttx": {
        "manifest_id": "nuttx",
        "display_name": "Apache NuttX",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "chibios": {
        "manifest_id": "chibios",
        "display_name": "ChibiOS/RT",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "riot": {
        "manifest_id": "riot",
        "display_name": "RIOT OS",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "cmsis-rtx": {
        "manifest_id": "cmsis-rtx-stm32f103-project",
        "display_name": "CMSIS-RTX / RTX5 bundle",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "apache-mynewt": {
        "manifest_id": "apache-mynewt",
        "display_name": "Apache Mynewt",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
    "rtems": {
        "manifest_id": "rtems",
        "display_name": "RTEMS",
        "implemented": True,
        "unsupported_reason": None,
        "reference_root": REFERENCES_ROOT,
    },
}


def _profile_spec_for(rtos_id: str) -> dict:
    """Return the built-in DriverGen profile specification for one RTOS."""
    try:
        return _RTOS_SPECS[rtos_id]
    except KeyError as exc:
        raise KeyError(f"Unsupported RTOS '{rtos_id}'.") from exc


def get_rtos_profile(rtos_id: str) -> RtosProfile:
    """Resolve a user-facing RTOS id to a validated profile object."""
    canonical = canonical_rtos_id(rtos_id)
    spec = _profile_spec_for(canonical)
    manifest_entries = _load_manifest()
    try:
        manifest_entry = manifest_entries[spec["manifest_id"]]
    except KeyError as exc:
        supported = ", ".join(sorted(manifest_entries))
        raise KeyError(f"Unknown RTOS '{canonical}'. Known manifest ids: {supported}") from exc
    return RtosProfile(
        rtos_id=canonical,
        display_name=spec["display_name"],
        repo_root=_resolve_repo_path(manifest_entry["path"]),
        manifest_entry=manifest_entry,
        implemented=spec["implemented"],
        unsupported_reason=spec["unsupported_reason"],
        reference_root=spec["reference_root"],
    )


def list_registered_rtos() -> list[dict]:
    """List RTOS entries that are known to the current DriverGen install."""
    items = []
    for rtos_id in sorted(_RTOS_SPECS):
        profile = get_rtos_profile(rtos_id)
        components = profile.manifest_entry.get("components") or []
        first_component = components[0] if components else {}
        items.append(
            {
                "id": profile.rtos_id,
                "display_name": profile.display_name,
                "implemented": profile.implemented,
                "origin_url": profile.manifest_entry.get("origin_url") or first_component.get("origin_url"),
            }
        )
    return items
