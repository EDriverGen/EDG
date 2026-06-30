"""pipeline step - slot template loader."""

from __future__ import annotations

import logging
from typing import Mapping

from .config import load_slot_templates
from .types import (
    SOURCE_KIND_MANIFEST_REPO,
    SOURCE_KIND_STUB,
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SlotGoal,
)

logger = logging.getLogger(__name__)


_VALID_LAYERS = frozenset(
    {"runtime", "bus", "board", "timing", "integration", "build", "exemplar", "task_helper"}
)

_VALID_EXPECTED_KINDS = frozenset(
    {
        "function",
        "macro",
        "typedef",
        "struct",
        "enum",
        "config",
        "task_package_helper",
    }
)

_VALID_SOURCE_KINDS = frozenset(
    {
        SOURCE_KIND_MANIFEST_REPO,
        SOURCE_KIND_TASK_PACKAGE_HELPER,
        SOURCE_KIND_STUB,
    }
)


def _coerce_str_list(value, *, field: str, slot_id: str) -> list[str]:
    if value is None:
        return []
    if not isinstance(value, list):
        raise ValueError(
            f"slot '{slot_id}': field '{field}' must be a list, got {type(value).__name__}"
        )
    out: list[str] = []
    for item in value:
        if not isinstance(item, str):
            raise ValueError(
                f"slot '{slot_id}': field '{field}' must contain only strings"
            )
        s = item.strip()
        if s:
            out.append(s)
    return out


def _validate_against_vocabulary(
    values: list[str], *, vocabulary: frozenset[str], field: str, slot_id: str
) -> list[str]:
    bad = [v for v in values if v not in vocabulary]
    if bad:
        raise ValueError(
            f"slot '{slot_id}': field '{field}' contains unknown values "
            f"{bad}; valid options are {sorted(vocabulary)}"
        )
    return values


def _build_slot_goal(raw: Mapping, *, connection_type: str) -> SlotGoal:
    slot_id = raw.get("slot_id")
    if not slot_id or not isinstance(slot_id, str):
        raise ValueError(
            f"slot template for '{connection_type}' has an entry without a "
            f"valid 'slot_id'"
        )

    layer = raw.get("layer")
    if not isinstance(layer, str) or layer not in _VALID_LAYERS:
        raise ValueError(
            f"slot '{slot_id}': layer='{layer}' invalid; valid options are "
            f"{sorted(_VALID_LAYERS)}"
        )

    required = bool(raw.get("required", False))

    canonical_bus = raw.get("canonical_bus")
    if canonical_bus is not None and not isinstance(canonical_bus, str):
        raise ValueError(f"slot '{slot_id}': canonical_bus must be string or null")

    query_intents = _coerce_str_list(
        raw.get("query_intents"), field="query_intents", slot_id=slot_id
    )

    expected_kinds = _validate_against_vocabulary(
        _coerce_str_list(
            raw.get("expected_kinds"), field="expected_kinds", slot_id=slot_id
        ),
        vocabulary=_VALID_EXPECTED_KINDS,
        field="expected_kinds",
        slot_id=slot_id,
    )

    preferred_root_roles = _coerce_str_list(
        raw.get("preferred_root_roles"),
        field="preferred_root_roles",
        slot_id=slot_id,
    )
    negative_root_roles = _coerce_str_list(
        raw.get("negative_root_roles"),
        field="negative_root_roles",
        slot_id=slot_id,
    )

    min_evidence_raw = raw.get("min_evidence", 1)
    if not isinstance(min_evidence_raw, int) or min_evidence_raw < 1:
        raise ValueError(
            f"slot '{slot_id}': min_evidence must be a positive integer"
        )

    source_kinds_allowed = _validate_against_vocabulary(
        _coerce_str_list(
            raw.get("source_kinds_allowed") or [SOURCE_KIND_MANIFEST_REPO],
            field="source_kinds_allowed",
            slot_id=slot_id,
        ),
        vocabulary=_VALID_SOURCE_KINDS,
        field="source_kinds_allowed",
        slot_id=slot_id,
    )

    return SlotGoal(
        slot_id=slot_id,
        layer=layer,
        required=required,
        canonical_bus=canonical_bus,
        query_intents=query_intents,
        expected_kinds=expected_kinds,
        preferred_root_roles=preferred_root_roles,
        negative_root_roles=negative_root_roles,
        min_evidence=min_evidence_raw,
        origin="template",
        source_kinds_allowed=source_kinds_allowed,
    )


def load_slot_template(connection_type: str) -> list[SlotGoal]:
    """Load the base SlotGoal list for *connection_type*."""
    templates = load_slot_templates()
    raw = templates.get(connection_type)
    if raw is None:
        # Soft miss — caller decides whether to warn (e.g. build_slot_plan
        # tries a synthesised key like ``i2c_polling`` before giving up).
        logger.debug(
            "No slot template registered for connection_type '%s'; "
            "available: %s",
            connection_type,
            sorted(templates.keys()),
        )
        return []

    base_slots = raw.get("base_slots") or []
    if not isinstance(base_slots, list):
        raise ValueError(
            f"slot template for '{connection_type}' has non-list 'base_slots'"
        )

    out: list[SlotGoal] = []
    seen: set[str] = set()
    for entry in base_slots:
        if not isinstance(entry, dict):
            raise ValueError(
                f"slot template for '{connection_type}': base_slots entry "
                f"must be an object, got {type(entry).__name__}"
            )
        goal = _build_slot_goal(entry, connection_type=connection_type)
        if goal.slot_id in seen:
            raise ValueError(
                f"slot template for '{connection_type}': duplicate slot_id "
                f"'{goal.slot_id}'"
            )
        seen.add(goal.slot_id)
        out.append(goal)
    return out


__all__ = ["load_slot_template"]
