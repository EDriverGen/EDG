"""Core dataclass definitions for the RTOS context pipeline."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

# Constants


# SymbolBinding.source_kind
SOURCE_KIND_MANIFEST_REPO = "manifest_repo"
"""Symbol comes from a real RTOS / vendor SDK / BSP repository
(everything indexed via ``data/rtos/manifest.json``)."""

SOURCE_KIND_TASK_PACKAGE_HELPER = "task_package_helper"
"""Legacy symbol source for task-supplied helper hooks.

New extraction should prefer manifest-backed RTOS symbols; this value is
kept so older artifacts and ledgers still deserialize.
"""

SOURCE_KIND_STUB = "stub"
"""Symbol exists only in our hand-written stub tree.  Not allowed for
codegen, only used as a link placeholder for diagnostics."""


# SymbolBinding.verification
VERIFICATION_SOURCE_DECLARED = "source_declared"
"""Manifest-source has a parsed FunctionDecl / MacroDef / TypedefDef."""

VERIFICATION_CONTRACT_DECLARED = "contract_declared"
"""Legacy helper hook declared by task-level context and flagged
``requires_runtime_provision``."""

VERIFICATION_STUB_ONLY = "stub_only"
"""Only seen in the stub tree.  Not allowed for codegen."""


# Source roots and task spec


@dataclass(frozen=True)
class SourceRoot:
    """A single RTOS / vendor SDK / BSP / docs git tree the pipeline is allowed to scan."""

    root_id: str
    """Stable identifier of the form
    ``<manifest_entry_id>:<component_id_or_kind>:<index>``.

    Examples:
        * ``bundle-a:kernel:0``
        * ``bundle-a:vendor_sdk:1``
        * ``bundle-b:single:0``
    """

    path: Path
    """Absolute filesystem path to the repository root."""

    roles: frozenset[str]
    """Coarse roles this root is allowed to play.  Drawn from a fixed
    vocabulary: ``kernel``, ``runtime``, ``driver_framework``,
    ``vendor_hal``, ``board_integration``, ``docs``, ``exemplar``.
    """

    priority: float = 1.0
    """Relative priority when ranking candidate files.  Higher = explored
    first; vendor SDK roots typically outrank kernel roots when looking
    for a bus HAL.  Default 1.0; ScopeMap or task code may override."""

    rtos_scope_id: str = ""
    """Canonical RTOS id used to look up the corresponding ScopeMap entry."""

    sha: str = ""
    """Short git HEAD SHA, or content fingerprint when git is unavailable.
    Empty string is a sentinel meaning ``not yet computed`` and never
    appears in cache keys."""


@dataclass
class BusIntent:
    """Normalised description of *what bus / connection style* the device needs."""

    canonical_bus: str
    """One of: ``i2c``, ``spi``, ``uart``, ``gpio``, ``adc``, ``pwm``,
    ``can``, ``unknown``."""

    connection_type: str
    """Verbatim from the task package, e.g. ``i2c_polling`` /
    ``gpio_timing`` / ``spi_dma``."""

    mode: str | None = None
    """``polling`` / ``interrupt`` / ``dma`` / ``timing`` / ``None``."""

    backend: str | None = None
    """Free-form backend label — e.g. ``vendor_hal``, ``rtos_device``,
    ``posix``, ``board_ops``.  Used by ScopeMap matching but never as a
    keyword search term."""

    address_mode: str | None = None
    """``7bit`` / ``10bit`` / ``cs`` / ``None``."""

    bus_instance: str | None = None
    """Symbolic bus instance, e.g. ``i2c1`` / ``default_gpio_slot``."""

    default_query_intents: list[str] = field(default_factory=list)
    """Connection-type-level baseline query intents from
    ``bus_taxonomy.json/connection_types.<ct>.default_query_intents``.
    slot derivation (:func:`drivergen.rtos.slot_derivation.build_slot_plan`)
    unions these into every base slot's ``query_intents`` so editing the
    taxonomy JSON is enough to propagate new vendor-specific keywords
    (e.g. ``"i2c master receive dma"``) without re-touching every slot
    template.

    Empty list when the connection_type is not in the taxonomy or has
    no baseline intents declared."""


@dataclass
class SlotGoal:
    """One concrete fact the driver-generation step must know to write a working driver."""

    slot_id: str
    """Stable dotted name, e.g. ``gpio.write`` / ``i2c.transfer`` /
    ``runtime.delay_us`` / ``timing.measure_pulse_width`` /
    ``integration.pin_binding`` / ``task_helper.<name>``."""

    layer: str
    """One of: ``runtime``, ``bus``, ``board``, ``timing``,
    ``integration``, ``build``, ``exemplar``, ``task_helper``."""

    required: bool
    """True for slots that must be covered before the artifact is
    considered usable; ``False`` for nice-to-have slots that only
    affect binding confidence."""

    canonical_bus: str | None = None
    """If set, this slot only applies to that bus."""

    query_intents: list[str] = field(default_factory=list)
    """Free-text phrases used by the deterministic ranker and the LLM
    routers to score candidates against this slot.

    Example for ``gpio.write``::

        ["gpio write", "set pin", "write pin", "output high low"]
    """

    expected_kinds: list[str] = field(default_factory=list)
    """Symbol kinds that may legitimately satisfy this slot:
    ``function``, ``macro``, ``typedef``, ``struct``, ``enum``,
    ``task_package_helper``."""

    preferred_root_roles: list[str] = field(default_factory=list)
    """If non-empty, candidates from these root roles are boosted."""

    negative_root_roles: list[str] = field(default_factory=list)
    """Candidates from these root roles are penalised (or rejected, if
    the validator's root-role gate is on strict)."""

    min_evidence: int = 1
    """Minimum number of distinct EvidenceSpans required to consider
    the slot covered."""

    origin: str = "template"
    """How this slot ended up in the SlotPlan.  Free-form for now;
    canonical values are ``template``, ``read_sequence``,
    ``device_attachment``, ``address_rule``, ``board_context``.  Multiple
    origins may be joined with ``+``
    when slot derivation merges duplicates (see
    :func:`drivergen.rtos.slot_derivation.build_slot_plan`)."""

    source_kinds_allowed: list[str] = field(
        default_factory=lambda: [SOURCE_KIND_MANIFEST_REPO]
    )
    """Which symbol source kinds may satisfy this slot."""


@dataclass
class SlotPlan:
    """Ordered set of :class:`SlotGoal`s the extractor must satisfy for
    one task.  Built from a connection-type YAML template plus
    derivation rules over device_ir / fixed_context / board_context.
    """

    slots: list[SlotGoal] = field(default_factory=list)
    connection_type: str = ""

    derivation_summary: dict[str, list[str]] = field(default_factory=dict)
    """``slot_id -> [origin tokens]`` — useful for diagnostics so we can
    explain *why* a particular slot is in the plan (e.g. ``gpio.write``
    came from ``template+read_sequence``)."""

    @property
    def required_slots(self) -> list[SlotGoal]:
        return [s for s in self.slots if s.required]


@dataclass
class TaskSpec:
    """The single normalised description of one driver-generation task, consumed by every stage after task-spec construction."""

    rtos_id: str
    board: str | None
    mcu_family: str | None
    integration: str | None
    integration_style: str | None

    bus_intent: BusIntent

    connection_binding: dict
    """Verbatim ``connection_binding_context`` from the task package
    (preserves things like ``bus_instance``, ``address_mode``, freeform
    notes)."""

    device_attachment: dict
    """Verbatim ``device_attachment_context`` (signals, required /
    optional attachments)."""

    device_id: str | None = None
    device_transaction_shape: str | None = None
    """One of the transaction shape labels (``register_pointer_then_read`` /
    ``command_write_then_delay_then_read`` / ``direct_read`` /
    ``unknown``).  Optional — :class:`SlotPlan` derivation may set it
    after inspecting ``device_ir.read_sequence``."""

    source_roots: list[SourceRoot] = field(default_factory=list)
    slot_plan: SlotPlan | None = None


# Repo index bundle


@dataclass
class SymbolSketch:
    """Lightweight symbol record produced by the regex-based pre-index."""

    name: str
    kind: str
    """``function`` / ``macro`` / ``typedef`` / ``struct`` / ``enum``."""

    signature: str | None = None
    """Function signature if confidently extractable; ``None`` for
    macros / type definitions / when the regex couldn't reconstruct
    a signature."""

    file: str = ""
    """Path **relative to the source root**, not absolute."""

    root_id: str = ""

    root_roles: frozenset[str] = field(default_factory=frozenset)

    path_tokens: list[str] = field(default_factory=list)
    """Lower-case word tokens of the file path, used by the ranker."""

    file_kind: str | None = None
    """Containing file's coarse kind: ``header`` / ``source`` /
    ``config`` / ``doc`` / ``example``. Optional for cached records."""

    dir_role_hint: str | None = None
    """ScopeMap-derived directory role for the containing file, if any."""

    include_context: list[str] = field(default_factory=list)
    """Headers ``#include``d by the file containing this symbol."""

    nearby_text: str | None = None
    """Up to ~300 characters of surrounding source — feeds the LLM
    Symbol Binder when the deterministic ranker leaves the slot in the
    ambiguous range."""


@dataclass(frozen=True)
class IncludeEdge:
    """One ``#include`` relationship between two files."""

    src_root_id: str
    src_path: str
    dst_path: str


@dataclass
class FileCard:
    """Per-file metadata used by directory routing, file selection, and deep-parse scheduling."""

    root_id: str
    path: str
    """Path relative to the source root."""

    abs_path: str
    """Absolute path on disk.  Cached here so downstream stages don't
    have to re-resolve the root."""

    file_kind: str
    """``header`` / ``source`` / ``build`` / ``config`` / ``doc`` /
    ``example``."""

    dir_role_hint: str | None = None
    """ScopeMap-derived hint for the directory this file lives in:
    ``kernel`` / ``vendor_hal`` / ``board`` / ``docs`` / ``exemplar``.
    ``None`` when the directory is outside any ScopeMap include.
    """

    exported_symbols: list[str] = field(default_factory=list)

    candidate_symbols: list[SymbolSketch] = field(default_factory=list)

    bus_hits: dict[str, int] = field(default_factory=dict)
    """``canonical_bus`` -> hit count from regex passes."""

    runtime_hits: dict[str, int] = field(default_factory=dict)
    """Various runtime keyword bucket -> count."""

    board_hits: dict[str, int] = field(default_factory=dict)

    mcu_affinity_score: float = 0.0
    """Higher = more likely to belong to the target MCU family.
    Derived from path tokens against ``TaskSpec.mcu_family``."""

    ecosystem_score: float = 1.0
    """Higher = stronger evidence the file belongs to the target RTOS
    ecosystem.  Polluting compatibility trees get penalised below 1.0."""

    include_deps: list[str] = field(default_factory=list)
    short_summary: str = ""


@dataclass
class DirectoryCard:
    """Aggregated card for one directory subtree."""

    root_id: str
    dir_path: str
    """Path relative to the source root."""

    file_count: int = 0
    code_file_count: int = 0

    role_hint: str | None = None
    """ScopeMap-derived role: ``kernel`` / ``vendor_hal`` /
    ``board`` / ``docs`` / ``exemplar`` / ``None``."""

    bus_hits: dict[str, int] = field(default_factory=dict)
    runtime_hits: dict[str, int] = field(default_factory=dict)
    board_hits: dict[str, int] = field(default_factory=dict)

    top_symbols: list[str] = field(default_factory=list)
    children: list[str] = field(default_factory=list)


@dataclass
class RepoIndexBundle:
    """Multi-root snapshot of a complete repo index.  Crosses the
    cache boundary; one of these is shared across every device that
    has the same ``(rtos_id, board, source_roots, scope_map)``.
    """

    roots: list[SourceRoot] = field(default_factory=list)
    file_cards: list[FileCard] = field(default_factory=list)
    dir_cards: list[DirectoryCard] = field(default_factory=list)
    symbol_sketches: list[SymbolSketch] = field(default_factory=list)
    include_edges: list[IncludeEdge] = field(default_factory=list)

    root_shas: dict[str, str] = field(default_factory=dict)
    """``root_id -> sha`` snapshot at index time, used for cache
    invalidation."""

    scope_map_hash: str = ""
    indexer_version: str = ""

    # Convenience query helpers

    def cards_in_root(self, root_id: str) -> list[FileCard]:
        return [c for c in self.file_cards if c.root_id == root_id]

    def cards_in_dir(self, root_id: str, dir_path: str) -> list[FileCard]:
        norm = dir_path.replace("\\", "/").rstrip("/") + "/"
        return [
            c
            for c in self.file_cards
            if c.root_id == root_id and c.path.startswith(norm)
        ]

    def root_by_id(self, root_id: str) -> SourceRoot | None:
        for r in self.roots:
            if r.root_id == root_id:
                return r
        return None


# Symbol binding and evidence


@dataclass
class RankedSymbolCandidate:
    """One :class:`SymbolSketch` plus the score / reasons computed by
    the deterministic ranker.  Fed into the symbol binder when
    the score lands in the ambiguous range.
    """

    sketch: SymbolSketch
    score: float
    match_reasons: list[str] = field(default_factory=list)
    """Free-form tokens explaining why the candidate scored what it
    did — e.g. ``["name_intent_match", "vendor_hal_role"]`` — used for
    diagnostics, not for scoring."""


@dataclass(frozen=True)
class EvidenceSpan:
    """One concrete piece of source-level evidence that backs a
    :class:`SymbolBinding`.  Frozen so spans can dedupe inside sets.
    """

    root_id: str
    path: str
    start_line: int | None = None
    end_line: int | None = None
    kind: str = "declaration"
    """``declaration`` / ``implementation`` / ``usage_example`` /
    ``include_dep``."""


@dataclass
class SymbolBinding:
    """The fully-validated answer to *"how is slot X satisfied?"*."""

    slot_id: str
    symbol: str
    kind: str
    """One of the values that the deep parser actually emits:
    ``function`` / ``macro`` / ``typedef`` / ``struct`` / ``enum``,
    plus ``task_package_helper`` for stage-6.1 hook bindings.
    The validator's ``_gate_symbol_existence`` indexes parsed symbols
    by ``(name, kind)`` so this vocabulary must stay in sync with the
    deep parser; see ``drivergen/rtos/validator.py``."""

    source_kind: str
    """One of :data:`SOURCE_KIND_MANIFEST_REPO`,
    :data:`SOURCE_KIND_TASK_PACKAGE_HELPER`, :data:`SOURCE_KIND_STUB`."""

    verification: str
    """One of :data:`VERIFICATION_SOURCE_DECLARED`,
    :data:`VERIFICATION_CONTRACT_DECLARED`,
    :data:`VERIFICATION_STUB_ONLY`."""

    signature: str | None = None
    signature_source: str | None = None
    """``parser`` / ``task_package`` / ``stub_hint`` / ``inferred`` /
    ``None``."""

    declared_in: str | None = None
    """``<root_id>:<rel_path>`` style location of the declaration."""

    implemented_in: str | None = None

    required_headers: list[str] = field(default_factory=list)
    """Headers the codegen LLM must ``#include`` to use this symbol."""

    required_types: list[str] = field(default_factory=list)
    return_semantics: str | None = None

    semantic_role: str | None = None
    """Free-form natural-language description of what the symbol does
    in the slot's context (e.g. ``"write digital output pin"``).
    Filled by the symbol binder."""

    confidence: float = 0.0

    allowed_for_codegen: bool = False
    """``True`` only after the Evidence Validator's gates have passed
    for this binding.  ``stub`` source-kind bindings are always False."""

    requires_runtime_provision: bool = False
    """Set when ``source_kind == task_package_helper`` — signals to the
    L0 link step that a stub-tree weak occurrence is required."""

    evidence: list[EvidenceSpan] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


# Coverage and validation


@dataclass
class SlotCoverageReport:
    """Snapshot of slot coverage at the end of one extraction round."""

    covered_required: list[str] = field(default_factory=list)
    covered_optional: list[str] = field(default_factory=list)
    missing_required: list[str] = field(default_factory=list)
    missing_optional: list[str] = field(default_factory=list)
    ambiguous: list[str] = field(default_factory=list)
    """Slots that have multiple candidates but no high-confidence
    binding yet.  Drive next-round symbol binder calls."""

    @property
    def all_required_covered(self) -> bool:
        return not self.missing_required


@dataclass
class RoundRecord:
    """One iteration of the extraction loop, persisted to the
    :class:`ExtractionLedger` so failure cases can be replayed.
    """

    round: int
    deterministic: bool
    """``True`` when no LLM client was used for this record."""

    covered_slots: list[str] = field(default_factory=list)
    missing_slots: list[str] = field(default_factory=list)
    new_files_added: list[str] = field(default_factory=list)
    rejected_files: list[dict] = field(default_factory=list)
    """List of ``{"path": ..., "reason": ...}``."""

    llm_calls: dict[str, int] = field(default_factory=dict)
    """``call_kind -> count``.  Call kinds: ``directory_router``,
    ``file_selector``, ``symbol_binder``, ``gap_diagnoser``,
    ``transaction_translator``."""

    token_usage: dict[str, int] = field(default_factory=dict)
    """``input_tokens`` / ``output_tokens`` / per-call-kind breakdowns."""


@dataclass
class ExtractionLedger:
    rounds: list[RoundRecord] = field(default_factory=list)


@dataclass
class EvidenceValidationReport:
    """Output of the Evidence Validator's gate run."""

    passed_gates: list[str] = field(default_factory=list)
    failed_gates: list[dict] = field(default_factory=list)
    """``{"gate_id": ..., "slot_id": ..., "symbol": ..., "message": ...}``."""

    warnings: list[dict] = field(default_factory=list)
    """``{"category": ..., "message": ..., "slot_id"?: ...}``."""

    @property
    def is_clean(self) -> bool:
        return not self.failed_gates


# Top-level artifact


@dataclass
class RtosEvidenceArtifact:
    """The single file that downstream code generation consumes."""

    version: str = "2.0"

    task_spec: TaskSpec | None = None

    source_roots: list[SourceRoot] = field(default_factory=list)
    """Pinned snapshot — useful when reading an artifact months later
    without re-resolving the manifest."""

    slots: dict[str, dict] = field(default_factory=dict)
    """``slot_id -> {status, required, bindings, confidence}``.

    ``bindings`` is a list of symbol names (keys into
    :attr:`symbols`) — keeping it shallow makes JSON inspection
    easier and avoids duplication."""

    symbols: dict[str, SymbolBinding] = field(default_factory=dict)
    """``slot_id -> SymbolBinding``.  The map is keyed by slot id so two
    slots binding the same symbol name don't collide.  Look up by
    symbol name via the auxiliary ``_symbol_index`` dict on the
    serialized artifact (``{symbol_name: [slot_id, ...]}``) when you
    need the reverse direction."""

    slot_fulfillments: dict[str, dict] = field(default_factory=dict)
    """Non-symbol fulfillment payloads keyed by slot id.  Examples:
    task-context integration values, multi-symbol type/status evidence,
    and transaction-template fulfillment."""

    context_bindings: dict[str, dict] = field(default_factory=dict)
    """Subset of :attr:`slot_fulfillments` derived from task context."""

    multi_symbol_evidence: dict[str, dict] = field(default_factory=dict)
    """Subset of :attr:`slot_fulfillments` that carries several source
    symbols instead of a single canonical binding."""

    types: dict[str, dict] = field(default_factory=dict)
    """``type_name -> {kind, declared_in, used_by}`` for any struct /
    typedef / enum that the bindings depend on."""

    integration: dict = field(default_factory=dict)
    """``bus_instance``, ``pin_bindings``, ``required_init``,
    ``build_sources``, ``include_dirs`` — anything the code generator
    needs to hook into a concrete board configuration."""

    files: dict[str, list[dict]] = field(default_factory=dict)
    """Compatibility view, organised the way prompt builders like to
    iterate: ``api_definition`` / ``implementation`` / ``board_config``
    / ``exemplar``.  Each entry is a small dict referencing already-
    bound symbols, *not* a raw parsed dump."""

    transaction_templates: list[dict] = field(default_factory=list)
    """Each template: ``{"name": ..., "derivation": ..., "confidence":
    ..., "steps": [...]}``.  Steps reference slot ids and source
    indexes from device_ir; produced by the Transaction Template
    Translator."""

    validation: EvidenceValidationReport | None = None

    ledger: ExtractionLedger | None = None
