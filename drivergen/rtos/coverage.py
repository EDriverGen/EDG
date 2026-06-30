"""pipeline step - SlotCoverageReport builder."""

from __future__ import annotations

from .types import (
    RankedSymbolCandidate,
    SlotCoverageReport,
    SlotPlan,
    SymbolBinding,
)


def report_slot_coverage(
    *,
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    ranks: dict[str, list[RankedSymbolCandidate]] | None = None,
    fulfilled_slot_ids: set[str] | None = None,
) -> SlotCoverageReport:
    """Build a :class:`SlotCoverageReport` for the current state."""
    covered_required: list[str] = []
    covered_optional: list[str] = []
    missing_required: list[str] = []
    missing_optional: list[str] = []
    ambiguous: list[str] = []

    fulfilled = set(bindings) | set(fulfilled_slot_ids or set())

    for slot in slot_plan.slots:
        if slot.slot_id in fulfilled:
            (covered_required if slot.required else covered_optional).append(slot.slot_id)
            continue

        # Slot is not bound — track in the right "missing" bucket.
        (missing_required if slot.required else missing_optional).append(slot.slot_id)

        # Mark as ambiguous when we DID have candidates to consider.
        if ranks and ranks.get(slot.slot_id):
            ambiguous.append(slot.slot_id)

    return SlotCoverageReport(
        covered_required=covered_required,
        covered_optional=covered_optional,
        missing_required=missing_required,
        missing_optional=missing_optional,
        ambiguous=ambiguous,
    )


__all__ = ["report_slot_coverage"]
