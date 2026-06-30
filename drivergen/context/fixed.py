from __future__ import annotations

import json
from pathlib import Path

from ..core.catalog import DATA_ROOT, PROJECT_ROOT


FIXED_CONTEXT_ROOT = DATA_ROOT / "fixed_context"
TASK_PACKAGES_ROOT = FIXED_CONTEXT_ROOT / "task_packages"
PLATFORM_BASE_CONTEXT_ROOT = FIXED_CONTEXT_ROOT / "platform_base_context"
CONNECTION_BINDING_CONTEXT_ROOT = FIXED_CONTEXT_ROOT / "connection_binding_context"
DEVICE_ATTACHMENT_CONTEXT_ROOT = FIXED_CONTEXT_ROOT / "device_attachment_context"


def _load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _resolve_path(raw_path: str | Path) -> Path:
    candidate = Path(raw_path)
    if candidate.is_absolute():
        return candidate

    candidates: list[Path] = []
    if candidate.parts and candidate.parts[0].lower() in {
        PROJECT_ROOT.name.lower(),
        "drivergen",
    }:
        trimmed = Path(*candidate.parts[1:])
        candidates.append((PROJECT_ROOT / trimmed).resolve())
        candidates.append((PROJECT_ROOT.parent / trimmed).resolve())

    candidates.append((PROJECT_ROOT / candidate).resolve())
    candidates.append((FIXED_CONTEXT_ROOT / candidate).resolve())

    seen: set[Path] = set()
    for resolved in candidates:
        if resolved in seen:
            continue
        seen.add(resolved)
        if resolved.exists():
            return resolved

    return candidates[0]


def _find_index_entry(index_path: Path, bucket_name: str, item_key: str, item_value: str) -> dict | None:
    payload = _load_json(index_path)
    for entry in payload.get(bucket_name, []):
        if entry.get(item_key) == item_value:
            return entry
    return None


def resolve_task_package_path(task_package_ref: str | Path) -> Path:
    candidate = Path(task_package_ref)
    if candidate.suffix.lower() == ".json":
        resolved = _resolve_path(candidate)
        if not resolved.exists():
            raise FileNotFoundError(f"Task package file not found: {resolved}")
        return resolved

    entry = _find_index_entry(
        TASK_PACKAGES_ROOT / "index.json",
        bucket_name="packages",
        item_key="package_id",
        item_value=str(task_package_ref),
    )
    if not entry:
        raise FileNotFoundError(f"Unknown task package id: {task_package_ref}")
    return _resolve_path(entry["path"])


def load_task_package(task_package_ref: str | Path) -> dict:
    return _load_json(resolve_task_package_path(task_package_ref))


def load_platform_base_context(path_or_ref: str | Path) -> dict:
    candidate = Path(path_or_ref)
    if candidate.suffix.lower() == ".json":
        return _load_json(_resolve_path(candidate))

    entry = _find_index_entry(
        PLATFORM_BASE_CONTEXT_ROOT / "index.json",
        bucket_name="platforms",
        item_key="platform_id",
        item_value=str(path_or_ref),
    )
    if not entry:
        raise FileNotFoundError(f"Unknown platform_base_context id: {path_or_ref}")
    return _load_json(_resolve_path(entry["path"]))


def load_connection_binding_context(path_or_ref: str | Path) -> dict:
    candidate = Path(path_or_ref)
    if candidate.suffix.lower() == ".json":
        return _load_json(_resolve_path(candidate))

    entry = _find_index_entry(
        CONNECTION_BINDING_CONTEXT_ROOT / "index.json",
        bucket_name="bindings",
        item_key="binding_id",
        item_value=str(path_or_ref),
    )
    if not entry:
        raise FileNotFoundError(f"Unknown connection_binding_context id: {path_or_ref}")
    return _load_json(_resolve_path(entry["path"]))


def load_device_attachment_context(path_or_ref: str | Path) -> dict:
    candidate = Path(path_or_ref)
    if candidate.suffix.lower() == ".json":
        return _load_json(_resolve_path(candidate))

    entry = _find_index_entry(
        DEVICE_ATTACHMENT_CONTEXT_ROOT / "index.json",
        bucket_name="devices",
        item_key="device_id",
        item_value=str(path_or_ref),
    )
    if not entry:
        raise FileNotFoundError(f"Unknown device_attachment_context id: {path_or_ref}")
    return _load_json(_resolve_path(entry["path"]))


def build_board_context(
    task_package: dict,
    platform_base_context: dict,
    connection_binding_context: dict,
    device_attachment_context: dict,
) -> dict:
    """Build a board-context-like view for compatibility with existing pipeline code."""
    default_overrides = device_attachment_context.get("default_overrides", {})
    return {
        "task_package_id": task_package["package_id"],
        "rtos": platform_base_context["rtos"],
        "rtos_bundle": platform_base_context.get("rtos_bundle"),
        "board": platform_base_context["board"],
        "board_alias": platform_base_context.get("board_alias"),
        "integration": platform_base_context.get("integration"),
        "integration_style": platform_base_context.get("integration_style"),
        "mcu_family": platform_base_context.get("mcu_family"),
        "mcu": platform_base_context.get("mcu"),
        "bus_type": connection_binding_context["connection_type"],
        "bus_instance": connection_binding_context["bus_instance"],
        "bus_instance_kind": connection_binding_context.get("bus_instance_kind"),
        "bus_symbol": connection_binding_context.get("bus_symbol"),
        "bus_backend": connection_binding_context.get("backend"),
        "address_mode": connection_binding_context.get("address_mode"),
        "mode": connection_binding_context.get("mode"),
        "irq": default_overrides.get("irq", "absent"),
        "dma": default_overrides.get("dma", "absent"),
        "reset_gpio": default_overrides.get("reset_gpio"),
        "power_gpio": default_overrides.get("power_gpio"),
        "fixed_attachment": connection_binding_context.get("fixed_attachment", {}),
        "device_attachment_context_id": device_attachment_context["device_id"],
        "device_addressing": device_attachment_context.get("addressing"),
    }


def build_fixed_task_context(
    platform_base_context: dict,
    connection_binding_context: dict,
    device_attachment_context: dict,
) -> dict:
    """Build the package-local fixed_task_context from authoritative layers."""
    return {
        "platform": {
            "platform_id": platform_base_context["platform_id"],
            "rtos": platform_base_context["rtos"],
            "rtos_bundle": platform_base_context.get("rtos_bundle"),
            "board": platform_base_context["board"],
            "board_alias": platform_base_context.get("board_alias"),
            "integration": platform_base_context.get("integration"),
            "integration_style": platform_base_context.get("integration_style"),
            "mcu_family": platform_base_context.get("mcu_family"),
            "mcu": platform_base_context.get("mcu"),
        },
        "connection": {
            "binding_id": connection_binding_context["binding_id"],
            "platform_id": connection_binding_context.get("platform_id"),
            "connection_type": connection_binding_context["connection_type"],
            "mode": connection_binding_context.get("mode"),
            "bus_instance": connection_binding_context["bus_instance"],
            "bus_instance_kind": connection_binding_context.get("bus_instance_kind"),
            "bus_symbol": connection_binding_context.get("bus_symbol"),
            "backend": connection_binding_context.get("backend"),
            "address_mode": connection_binding_context.get("address_mode"),
            "fixed_attachment": connection_binding_context.get("fixed_attachment", {}),
            "helper_usage_patterns": connection_binding_context.get(
                "helper_usage_patterns", []
            ),
        },
        "device": {
            "device_id": device_attachment_context["device_id"],
            "attachment_type": device_attachment_context.get("attachment_type"),
            "required_bus_type": device_attachment_context.get("required_bus_type"),
            "address_mode": device_attachment_context.get("address_mode"),
            "addressing": device_attachment_context.get("addressing"),
            "required_attachment": device_attachment_context.get("required_attachment", {}),
            "optional_attachment": device_attachment_context.get("optional_attachment", {}),
            "default_overrides": device_attachment_context.get("default_overrides", {}),
            "protocol_hints": device_attachment_context.get("protocol_hints", {}),
        },
    }


def resolve_task_package_context(task_package_ref: str | Path) -> dict:
    """Expand one task package into its three referenced context layers."""
    task_package_path = resolve_task_package_path(task_package_ref)
    task_package = dict(_load_json(task_package_path))

    platform = load_platform_base_context(task_package["platform_base_context"])
    connection = load_connection_binding_context(task_package["connection_binding_context"])
    device_attachment = load_device_attachment_context(task_package["device_attachment_context"])
    task_package["fixed_task_context"] = build_fixed_task_context(
        platform_base_context=platform,
        connection_binding_context=connection,
        device_attachment_context=device_attachment,
    )
    board_context = build_board_context(
        task_package=task_package,
        platform_base_context=platform,
        connection_binding_context=connection,
        device_attachment_context=device_attachment,
    )

    return {
        "task_package_path": str(task_package_path),
        "task_package": task_package,
        "platform_base_context": platform,
        "connection_binding_context": connection,
        "device_attachment_context": device_attachment,
        "board_context": board_context,
    }


def resolve_run_fixed_context(run_config) -> dict | None:
    """Resolve fixed context for a pipeline run when it declares a task package."""
    task_package_path = getattr(run_config, "task_package_path", None)
    if not task_package_path:
        return None
    return resolve_task_package_context(task_package_path)


def build_target_binding(run_config, board_context: dict, fixed_context: dict | None) -> dict:
    """Build the Stage-A target binding payload used by the new RTOS pipeline."""
    if fixed_context:
        return {
            "run_id": run_config.run_id,
            "device_id": run_config.device_id,
            "rtos_id": run_config.rtos_id,
            "bus_type": board_context.get("bus_type"),
            "mode": board_context.get("mode"),
            "task_package_id": fixed_context["task_package"]["package_id"],
            "platform_base_context_id": fixed_context["platform_base_context"]["platform_id"],
            "connection_binding_context_id": fixed_context["connection_binding_context"]["binding_id"],
            "device_attachment_context_id": fixed_context["device_attachment_context"]["device_id"],
        }

    return {
        "run_id": run_config.run_id,
        "device_id": run_config.device_id,
        "rtos_id": run_config.rtos_id,
        "bus_type": board_context.get("bus_type"),
        "mode": board_context.get("mode"),
        "board_context_path": (
            str(run_config.board_context_path)
            if getattr(run_config, "board_context_path", None)
            else None
        ),
        "task_package_id": None,
        "platform_base_context_id": None,
        "connection_binding_context_id": None,
        "device_attachment_context_id": None,
    }
