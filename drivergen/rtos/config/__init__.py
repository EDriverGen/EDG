"""RTOS configuration loaders."""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

CONFIG_DIR = Path(__file__).resolve().parent

THRESHOLDS_PATH = CONFIG_DIR / "thresholds.json"
BUS_TAXONOMY_PATH = CONFIG_DIR / "bus_taxonomy.json"
SLOT_TEMPLATES_DIR = CONFIG_DIR / "slot_templates"
SCOPE_MAP_DIR = CONFIG_DIR / "scope_map"


@dataclass
class RtosConfig:
    """Container for the merged RTOS configuration set."""

    thresholds: dict
    bus_taxonomy: dict
    slot_templates: dict[str, dict] = field(default_factory=dict)


def _read_json(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"RTOS config missing: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


@lru_cache(maxsize=1)
def load_thresholds() -> dict:
    """Return the parsed contents of ``thresholds.json``."""
    return _read_json(THRESHOLDS_PATH)


@lru_cache(maxsize=1)
def load_bus_taxonomy() -> dict:
    """Return the parsed contents of ``bus_taxonomy.json``."""
    return _read_json(BUS_TAXONOMY_PATH)


@lru_cache(maxsize=1)
def load_slot_templates() -> dict[str, dict]:
    """Return ``{connection_type: template_dict}`` for slot templates."""
    templates: dict[str, dict] = {}
    if not SLOT_TEMPLATES_DIR.exists():
        return templates
    for path in sorted(SLOT_TEMPLATES_DIR.glob("*.json")):
        templates[path.stem] = _read_json(path)
    return templates


def load_rtos_config() -> RtosConfig:
    """Load the entire config bundle in one call."""
    return RtosConfig(
        thresholds=load_thresholds(),
        bus_taxonomy=load_bus_taxonomy(),
        slot_templates=load_slot_templates(),
    )


__all__ = [
    "RtosConfig",
    "CONFIG_DIR",
    "THRESHOLDS_PATH",
    "BUS_TAXONOMY_PATH",
    "SLOT_TEMPLATES_DIR",
    "SCOPE_MAP_DIR",
    "load_thresholds",
    "load_bus_taxonomy",
    "load_slot_templates",
    "load_rtos_config",
]
