"""pipeline step - source-kind classifier."""

from __future__ import annotations

import logging
from dataclasses import dataclass

from .types import SlotPlan, SymbolBinding, TaskSpec

logger = logging.getLogger(__name__)


@dataclass
class HelperClassification:
    """Diagnostic record for one slot the classifier looked at."""

    slot_id: str
    accepted: bool
    helper_name: str | None
    reason: str


def apply_task_package_helpers(
    *,
    bindings: dict[str, SymbolBinding],
    still_deferred: list[str],
    slot_plan: SlotPlan,
    task_spec: TaskSpec,
    gap_fallback_helpers: dict[str, str] | None = None,
) -> tuple[dict[str, SymbolBinding], list[str], list[HelperClassification]]:
    """Return all slots as still unbound without using fixed-context helpers."""

    classifications = [
        HelperClassification(
            slot_id=slot_id,
            accepted=False,
            helper_name=None,
            reason="fixed_context_helper_promotion_disabled",
        )
        for slot_id in still_deferred
    ]
    logger.info(
        "Source-kind classifier: fixed-context helper promotion disabled "
        "(remaining unbound: %d)",
        len(still_deferred),
    )
    return {}, list(still_deferred), classifications


__all__ = [
    "HelperClassification",
    "apply_task_package_helpers",
]
