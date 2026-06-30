"""Iterative-deepening role-stratified LLM scope triage."""

from __future__ import annotations

import logging
import re
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from ..llm_infra import call_llm_json

logger = logging.getLogger(__name__)


# Bump when prompt or role definitions should invalidate cached scope maps.
PROMPT_VERSION = "DriverGen-scope-generic"


# Role catalogue


@dataclass(frozen=True)
class RoleSpec:
    name: str
    short_label: str
    in_scope_definition: str
    drop_principle: str
    positive_keywords: tuple[str, ...] = ()
    negative_keywords: tuple[str, ...] = ()
    extra_disambiguations: tuple[str, ...] = ()
    mcu_family_relevance: str = ""


UNIVERSAL_DROP_NAMES = (
    "doc", "docs",
    "test", "tests",
    "demo", "demos", "example", "examples", "sample", "samples",
    "tools", "scripts",
    "build", "out",
)


ROLES: dict[str, RoleSpec] = {
    "kernel": RoleSpec(
        name="kernel",
        short_label="kernel/runtime",
        in_scope_definition=(
            "Native kernel/runtime primitives that driver code may call: "
            "threads, scheduling, locks, events, delays, timers, queues, "
            "memory pools, atomics, and critical sections."
        ),
        drop_principle=(
            "Drop bus frameworks, board support, vendor HAL code, foreign "
            "API adapters, examples, tests, docs, build glue, and ports "
            "for non-target architectures."
        ),
        positive_keywords=(
            "thread", "task", "sched", "mutex", "sem", "event",
            "delay", "timer", "queue", "heap", "atomic", "critical",
        ),
        negative_keywords=(
            "adapter", "compat", "wrapper",
            "hal", "bsp", "driver", "drivers", "i2c", "spi", "uart", "gpio",
        ),
        mcu_family_relevance=(
            "Use the target MCU only to choose among architecture or port "
            "siblings. Generic kernel API directories are MCU-independent."
        ),
    ),
    "driver_framework_i2c": RoleSpec(
        name="driver_framework_i2c",
        short_label="driver framework / I2C",
        in_scope_definition=(
            "The I2C bus framework exposed to driver authors: public bus "
            "headers, bus core code, target MCU glue needed for linking, "
            "and shared support used by the bus implementation."
        ),
        drop_principle=(
            "Drop pure kernel internals, unrelated buses, foreign adapters, "
            "board-only pin maps, non-target MCU families, examples, tests, "
            "docs, and build glue."
        ),
        positive_keywords=(
            "i2c", "twi", "smbus", "master", "transfer", "bus", "hal", "api",
        ),
        negative_keywords=(
            "adapter", "compat", "wrapper",
            "spi", "uart", "can", "usb", "ethernet", "thread", "sched",
        ),
        mcu_family_relevance=(
            "When directories are split by MCU family or platform, keep the "
            "branch matching the target and drop sibling families."
        ),
    ),
}


# Role-name mapping for ad-hoc bus roles.
def role_for_bus(bus_kind: str) -> RoleSpec:
    """Return the RoleSpec to drive triage for *bus_kind*."""
    bus_kind = (bus_kind or "").lower()
    if bus_kind == "i2c":
        return ROLES["driver_framework_i2c"]
    base = ROLES["driver_framework_i2c"]
    bus_caps = bus_kind.upper()

    return RoleSpec(
        name=f"driver_framework_{bus_kind}",
        short_label=f"driver framework / {bus_caps}",
        in_scope_definition=base.in_scope_definition.replace("I2C", bus_caps).replace(
            "i2c", bus_kind
        ),
        drop_principle=base.drop_principle.replace("I2C", bus_caps).replace(
            "i2c", bus_kind
        ),
        positive_keywords=(bus_kind, bus_caps) + tuple(
            k for k in base.positive_keywords
            if k.lower() not in ("i2c", "iic", "twi")
        ),
        negative_keywords=base.negative_keywords + (
            "i2c" if bus_kind != "i2c" else "",
        ),
        extra_disambiguations=tuple(
            d.replace("I2C", bus_caps).replace("i2c", bus_kind)
            for d in base.extra_disambiguations
        ),
        mcu_family_relevance=base.mcu_family_relevance.replace("I2C", bus_caps),
    )

# Targeting


def _normalise_mcu(s: str) -> str:
    return "".join(ch for ch in s.lower() if ch.isalnum())


def canonical_mcu_family(mcu_family: str | None) -> str | None:
    """Return a stable cache token for a user-provided MCU family."""
    if not mcu_family:
        return None
    norm = _normalise_mcu(mcu_family)
    return norm or None


# Tree enumeration


_SOURCE_EXTS: tuple[str, ...] = (".h", ".c", ".cpp", ".cxx", ".cc", ".S", ".s")


@dataclass(frozen=True)
class DirSnapshot:
    rel_path: str
    n_subdirs: int
    subdir_names: tuple[str, ...]
    direct_files: tuple[str, ...]
    direct_file_count: int
    subdir_samples: tuple[str, ...]


def _list_direct_source_files(child: Path, max_files: int) -> list[str]:
    direct = sorted(
        f.name
        for f in child.iterdir()
        if f.is_file() and f.suffix in _SOURCE_EXTS
    )
    return direct[:max_files]


def _count_direct_source_files(child: Path) -> int:
    return sum(
        1
        for f in child.iterdir()
        if f.is_file() and f.suffix in _SOURCE_EXTS
    )


def _subdir_breadth_samples(child: Path, max_files: int) -> list[str]:
    """One representative file per immediate sub-dir, BFS-style."""
    out: list[str] = []
    immediate_subdirs = sorted(
        d for d in child.iterdir()
        if d.is_dir() and not d.name.startswith(".") and d.name != "__pycache__"
    )
    for sub in immediate_subdirs:
        first_in_sub: str | None = None
        for f in sorted(sub.rglob("*.h")):
            if f.is_file():
                first_in_sub = f.name
                break
        if not first_in_sub:
            for f in sorted(sub.rglob("*.c")):
                if f.is_file():
                    first_in_sub = f.name
                    break
        if first_in_sub:
            out.append(f"{sub.name}/{first_in_sub}")
        if len(out) >= max_files:
            break
    return out


def enumerate_layer(
    rtos_root: Path,
    parent_rel: str,
    sample_size: int = 5,
) -> list[DirSnapshot]:
    parent_abs = rtos_root / parent_rel if parent_rel else rtos_root
    if not parent_abs.is_dir():
        return []
    out: list[DirSnapshot] = []
    for child in sorted(parent_abs.iterdir()):
        if not child.is_dir():
            continue
        if child.name.startswith(".") or child.name == "__pycache__":
            continue
        rel = (Path(parent_rel) / child.name).as_posix() if parent_rel else child.name
        sub_names: list[str] = []
        for grand in sorted(child.iterdir()):
            if grand.is_dir() and not grand.name.startswith("."):
                sub_names.append(grand.name)
            if len(sub_names) >= 12:
                break
        direct_files = _list_direct_source_files(child, max_files=sample_size)
        direct_total = _count_direct_source_files(child)
        subdir_samples = _subdir_breadth_samples(child, max_files=sample_size)
        n_sub = sum(
            1
            for g in child.iterdir()
            if g.is_dir() and not g.name.startswith(".")
        )
        out.append(
            DirSnapshot(
                rel_path=rel,
                n_subdirs=n_sub,
                subdir_names=tuple(sub_names),
                direct_files=tuple(direct_files),
                direct_file_count=direct_total,
                subdir_samples=tuple(subdir_samples),
            )
        )
    return out


# Prompts


SYSTEM_PROMPT_TEMPLATE = """You are a path-level scope triage assistant for an embedded RTOS source-code indexer.

The downstream consumer is a driver-code generator. It indexes source trees
to find candidate symbols for runtime and bus-access slots. Your verdicts
decide which directories enter that candidate pool.

For one functional role per request, emit a verdict for each listed directory:

  - "KEEP": confirmed in scope; index the subtree.
  - "DROP": confirmed out of scope; exclude the subtree.
  - "RECURSE": mixed or insufficient evidence; inspect one level deeper.

Use only path names, subdirectory names, and sample basenames. The downstream
indexer handles symbol-level evidence.

Role: {role_short}

In-scope definition:
{in_scope_definition}

Drop principle:
{drop_principle}

{role_keywords_block}{extra_disambiguations_block}{mcu_context_block}
## Decision Rules

Apply these obvious out-of-scope directory names before role-specific reasoning:

  {universal_drop_list}

If any immediate child of a directory would be dropped for this role, the parent
must be RECURSE instead of KEEP. KEEP a parent only when every visible child is
clearly in scope.

Prefer DROP for adapters, compatibility layers, wrappers, board-only pin maps,
non-target MCU or architecture branches, demos, tests, docs, build glue, linker
scripts, and startup-only code. Prefer KEEP when directory names or samples
clearly expose the requested role's API surface.

Architecture and vendor port trees are often mixed. At broad parent levels such
as `arch/<arch>/` or `arch/<arch>/src/`, use RECURSE when kernel ports and
chip-family ports may be siblings. At the sibling level, keep only the branch
that matches the requested role and target MCU, and drop unrelated siblings.

When a RECURSE directory has loose source files directly under it, set
`self_files` explicitly. Use "KEEP" when those loose files appear to share the
same role as the kept children, "DROP" when they are clearly off-role, and "N/A"
when there are no loose source files or the verdict is KEEP/DROP.

Return JSON only:

{{
  "decisions": [
    {{
      "path": "<repo-relative directory path>",
      "verdict": "KEEP" | "DROP" | "RECURSE",
      "self_files": "KEEP" | "DROP" | "N/A",
      "reason": "<one short sentence>"
    }}
  ]
}}

Output exactly one decision for each input directory. Do not invent paths or
omit requested paths.
"""


USER_PROMPT_TEMPLATE = """RTOS: {rtos_id}
Role: {role_short}
{mcu_user_block}Round: {round_no} of <= {max_rounds}

Already KEPT from earlier rounds (in-scope, no need to re-decide):
{kept_block}

Directories to decide on this round. Each entry shows:
  - subdir count and names
  - "loose files at this level": source files sitting directly in the
    directory's own root (NOT inside a subdir). When a directory has both
    sub-dirs AND loose files, pay attention — RECURSE'ing the parent
    drops the loose files unless you set `self_files`:"KEEP".
  - "sub-dir samples": one representative source file per immediate subdir
    (BFS depth-1) to give breadth-aware evidence.

{tree_block}

For EACH directory above, emit one decision object with:
  - "verdict": KEEP / DROP / RECURSE
  - "self_files": KEEP / DROP / N/A — REQUIRED when verdict=RECURSE AND the
    directory has any loose files at this level; use N/A otherwise. Default
    to KEEP when in doubt: a RECURSE'd parent's loose source files are
    almost always part of the same subsystem as its kept children.
  - "reason": one short sentence

Output JSON only.
"""


ORPHAN_REVIEW_SYSTEM_PROMPT = """You are auditing a final scope-triage result for an embedded RTOS source-code indexer.

The earlier rounds were per-layer ("KEEP / DROP / RECURSE" with an optional `self_files` verdict for loose files at a RECURSE'd parent). For some parent directories, loose files were tentatively kept before child directories were fully resolved. After triage completed, every immediate sub-directory of those parents ended up DROP, leaving the loose files as orphans with no surviving descendant.

Your job: for EACH such orphan parent, decide whether the loose source files at that level (NOT inside any sub-directory) are actually in-scope for the requested ROLE, or should also be dropped.

## Role being audited

Role: {role_short}

In-scope definition:
{in_scope_definition}

Drop principle:
{drop_principle}

{role_keywords_block}

## Decision rule

Look at the loose-file basenames and the parent's path:
  - KEEP if the basenames clearly expose the requested role's API surface.
  - DROP if the basenames are clearly off-role for the requested role.

## Output format

Return JSON:

{{
  "revisions": [
    {{
      "path": "<parent path>",
      "self_files": "KEEP" | "DROP",
      "reason": "<one short sentence>"
    }},
    ...
  ]
}}

Output exactly one entry per orphan parent in the input list.
"""


ORPHAN_REVIEW_USER_PROMPT = """RTOS: {rtos_id}
Role: {role_short}
{mcu_user_block}
The triage finished. The following parent directories had ALL sub-directories
DROP'd, but the LOOSE source files at the parent itself were tentatively kept.
Re-decide whether the loose files belong to role `{role_short}` or not.

For each parent below, the loose-file basenames are listed. Reason about
whether they expose the role's API surface (KEEP) or are off-role
(DROP).

{orphan_block}

Output JSON only.
"""


def _format_keyword_list(keywords: tuple[str, ...]) -> str:
    if not keywords:
        return "(none)"
    return ", ".join(f"`{k}`" for k in keywords)


def build_system_prompt(
    role: RoleSpec,
    mcu_family: str | None = None,
) -> str:
    role_keywords_block = ""
    if role.positive_keywords or role.negative_keywords:
        role_keywords_block = (
            "\n## Role-specific keyword evidence\n\n"
            f"Strong KEEP signals (positive keywords; expect to see these in "
            f"sample basenames or subdir names): {_format_keyword_list(role.positive_keywords)}\n\n"
            f"Strong DROP signals (negative keywords): "
            f"{_format_keyword_list(role.negative_keywords)}\n"
        )

    extras = ""
    if role.extra_disambiguations:
        extras_lines = ["\n## Role-specific disambiguations\n"]
        for line in role.extra_disambiguations:
            extras_lines.append(f"- {line}")
        extras = "\n".join(extras_lines) + "\n"

    mcu_block = ""
    target_mcu = (mcu_family or "").strip()
    if target_mcu and role.mcu_family_relevance:
        mcu_block = (
            f"\n## Target MCU\n\n{target_mcu}\n\n"
            f"{role.mcu_family_relevance}\n\n"
            "Use the directory listing as the source of truth. Keep the "
            "closest matching MCU, chip-family, or architecture branch; "
            "drop clearly unrelated sibling branches.\n"
        )

    return SYSTEM_PROMPT_TEMPLATE.format(
        role_short=role.short_label,
        in_scope_definition=role.in_scope_definition,
        drop_principle=role.drop_principle,
        role_keywords_block=role_keywords_block,
        extra_disambiguations_block=extras,
        mcu_context_block=mcu_block,
        universal_drop_list=", ".join(f"`{n}`" for n in UNIVERSAL_DROP_NAMES),
    )


def render_dir_snapshot(snap: DirSnapshot) -> str:
    sub_str = ", ".join(snap.subdir_names[:10])
    if snap.n_subdirs > 10:
        sub_str += f", ... (+{snap.n_subdirs - 10} more)"

    direct_total = snap.direct_file_count
    if direct_total == 0:
        direct_line = "loose files at this level: (none)"
    else:
        listed = ", ".join(snap.direct_files)
        more = direct_total - len(snap.direct_files)
        if more > 0:
            listed += f", ... (+{more} more)"
        direct_line = f"loose files at this level ({direct_total}): {listed}"

    if snap.subdir_samples:
        sample_str = ", ".join(snap.subdir_samples)
    else:
        sample_str = "(no source samples from sub-dirs)"

    return (
        f"  {snap.rel_path}/\n"
        f"      subdirs ({snap.n_subdirs}): {sub_str if sub_str else '(none)'}\n"
        f"      {direct_line}\n"
        f"      sub-dir samples: {sample_str}"
    )


def build_user_prompt(
    rtos_id: str,
    role: RoleSpec,
    round_no: int,
    max_rounds: int,
    kept_paths: list[str],
    snapshots: list[DirSnapshot],
    mcu_family: str | None = None,
) -> str:
    if kept_paths:
        kept_block = "\n".join(f"  - {p}/" for p in kept_paths)
    else:
        kept_block = "  (none — first round, starting from RTOS root)"
    tree_block = "\n\n".join(render_dir_snapshot(s) for s in snapshots)
    mcu_user_block = ""
    target_mcu = (mcu_family or "").strip()
    if target_mcu:
        mcu_user_block = f"Target MCU family: {target_mcu}\n"
    return USER_PROMPT_TEMPLATE.format(
        rtos_id=rtos_id,
        role_short=role.short_label,
        mcu_user_block=mcu_user_block,
        round_no=round_no,
        max_rounds=max_rounds,
        kept_block=kept_block,
        tree_block=tree_block,
    )


# Schema


def build_schema() -> dict[str, Any]:
    return {
        "type": "object",
        "properties": {
            "decisions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string"},
                        "verdict": {
                            "type": "string",
                            "enum": ["KEEP", "DROP", "RECURSE"],
                        },
                        "self_files": {
                            "type": "string",
                            "enum": ["KEEP", "DROP", "N/A"],
                        },
                        "reason": {"type": "string", "minLength": 4},
                    },
                    "required": ["path", "verdict", "reason"],
                    "additionalProperties": False,
                },
            }
        },
        "required": ["decisions"],
        "additionalProperties": False,
    }


_BUS_FALLBACK_KEYWORDS: dict[str, tuple[str, ...]] = {
    "i2c": ("i2c", "twi", "smbus"),
    "spi": ("spi", "qspi"),
    "uart": ("uart", "usart", "serial"),
    "gpio": ("gpio", "pin", "ioport"),
}


def _fallback_text_for_snapshot(snap: DirSnapshot) -> str:
    parts = [snap.rel_path, *snap.subdir_names, *snap.direct_files, *snap.subdir_samples]
    return " ".join(parts).lower().replace("-", "_")


def _has_keyword(text: str, keyword: str) -> bool:
    needle = keyword.lower().replace("-", "_")
    if len(needle) <= 3:
        return bool(re.search(rf"(^|[^a-z0-9]){re.escape(needle)}([^a-z0-9]|$)", text))
    return needle in text


def _fallback_keep_snapshot_for_role(snap: DirSnapshot, role: RoleSpec) -> bool:
    """Conservative non-LLM fallback when scope triage returns invalid JSON."""
    text = _fallback_text_for_snapshot(snap)
    path_parts = {
        part.lower().replace("-", "_")
        for part in Path(snap.rel_path).parts
        if part
    }
    if path_parts & {name.replace("-", "_") for name in UNIVERSAL_DROP_NAMES}:
        return False
    if any(_has_keyword(text, kw) for kw in role.negative_keywords):
        return False

    if role.name.startswith("driver_framework_"):
        bus = role.name[len("driver_framework_") :]
        for kw in _BUS_FALLBACK_KEYWORDS.get(bus, (bus,)):
            if _has_keyword(text, kw):
                return True
        return False

    return any(_has_keyword(text, kw) for kw in role.positive_keywords)


def _apply_no_payload_fallback(
    *,
    state: TriageState,
    snapshots: list[DirSnapshot],
    round_no: int,
) -> list[dict[str, Any]]:
    decisions: list[dict[str, Any]] = []
    for s in snapshots:
        keep = _fallback_keep_snapshot_for_role(s, state.role)
        if keep:
            state.kept_terminal.add(s.rel_path)
            verdict = "KEEP"
        else:
            state.dropped.add(s.rel_path)
            verdict = "DROP"
        decisions.append(
            {
                "path": s.rel_path,
                "verdict": verdict,
                "reason": (
                    "deterministic fallback after invalid LLM payload"
                    if round_no > 1
                    else "deterministic fallback at root after invalid LLM payload"
                ),
            }
        )
    return decisions


# Triage driver


@dataclass
class TriageState:
    rtos_id: str
    role: RoleSpec
    max_rounds: int
    kept_terminal: set[str] = field(default_factory=set)
    kept_self_files: set[str] = field(default_factory=set)
    dropped: set[str] = field(default_factory=set)
    frontier: set[str] = field(default_factory=set)
    audit: list[dict[str, Any]] = field(default_factory=list)


def run_triage_for_role(
    *,
    rtos_root: Path,
    rtos_id: str,
    role: RoleSpec,
    provider: Any,
    budget,
    max_rounds: int = 5,
    sample_size: int = 5,
    mcu_family: str | None = None,
) -> TriageState:
    state = TriageState(rtos_id=rtos_id, role=role, max_rounds=max_rounds)
    state.frontier = {""}
    target_mcu = (mcu_family or "").strip() or None

    schema = build_schema()
    system_prompt = build_system_prompt(role, mcu_family=target_mcu)
    used_no_payload_fallback = False

    for round_no in range(1, max_rounds + 1):
        snapshots: list[DirSnapshot] = []
        for parent in sorted(state.frontier):
            snapshots.extend(
                enumerate_layer(rtos_root, parent, sample_size=sample_size)
            )
        if not snapshots:
            break

        kept_for_prompt = sorted(state.kept_terminal)
        user_prompt = build_user_prompt(
            rtos_id=rtos_id,
            role=role,
            round_no=round_no,
            max_rounds=max_rounds,
            kept_paths=kept_for_prompt,
            snapshots=snapshots,
            mcu_family=target_mcu,
        )

        t0 = time.time()
        payload, telemetry = call_llm_json(
            provider=provider,
            call_kind="directory_router",
            task_name=f"scope_triage__{rtos_id}__{role.name}__r{round_no}",
            schema=schema,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            budget=budget,
        )
        dt = time.time() - t0

        round_record: dict[str, Any] = {
            "round": round_no,
            "frontier_in": sorted(state.frontier),
            "decisions": [],
            "elapsed_s": round(dt, 2),
            "telemetry": telemetry,
            "user_prompt_chars": len(user_prompt),
            "system_prompt_chars": len(system_prompt),
            "n_snapshots": len(snapshots),
        }

        if not payload or not isinstance(payload, dict):
            fallback_decisions = _apply_no_payload_fallback(
                state=state,
                snapshots=snapshots,
                round_no=round_no,
            )
            logger.error(
                "[round %d/%s] LLM returned no valid payload; applied deterministic fallback: %s",
                round_no,
                role.name,
                telemetry,
            )
            round_record["error"] = "no_payload_or_skipped"
            round_record["fallback"] = "role_keyword_filter"
            round_record["decisions"] = fallback_decisions
            state.audit.append(round_record)
            used_no_payload_fallback = True
            break

        decisions_in = payload.get("decisions", []) or []
        seen_paths: set[str] = set()
        new_frontier: set[str] = set()
        snapshot_index = {s.rel_path: s for s in snapshots}
        for d in decisions_in:
            p = d.get("path", "").strip().strip("/")
            v = d.get("verdict", "")
            r = d.get("reason", "")
            sf = d.get("self_files", "N/A")
            if not p or v not in ("KEEP", "DROP", "RECURSE"):
                continue
            seen_paths.add(p)
            decision_rec: dict[str, Any] = {"path": p, "verdict": v, "reason": r}
            if v == "KEEP":
                state.kept_terminal.add(p)
            elif v == "DROP":
                state.dropped.add(p)
            else:
                new_frontier.add(p)
                snap = snapshot_index.get(p)
                has_loose = bool(snap and snap.direct_file_count > 0)
                if has_loose:
                    if sf == "DROP":
                        decision_rec["self_files"] = "DROP"
                    else:
                        state.kept_self_files.add(p)
                        decision_rec["self_files"] = "KEEP" if sf == "KEEP" else (
                            "KEEP(default)" if sf in ("N/A", "") else f"KEEP(coerced from {sf})"
                        )
            round_record["decisions"].append(decision_rec)

        omitted = [s.rel_path for s in snapshots if s.rel_path not in seen_paths]
        if omitted:
            round_record["llm_omitted_defaulted_keep"] = omitted
            for p in omitted:
                state.kept_terminal.add(p)

        state.frontier = new_frontier
        state.audit.append(round_record)

        logger.info(
            "[%s/%s round %d] kept=%d drop=%d recurse=%d dt=%.1fs",
            rtos_id,
            role.name,
            round_no,
            sum(1 for d in round_record["decisions"] if d["verdict"] == "KEEP"),
            sum(1 for d in round_record["decisions"] if d["verdict"] == "DROP"),
            sum(1 for d in round_record["decisions"] if d["verdict"] == "RECURSE"),
            dt,
        )

        if not new_frontier:
            break

    state.kept_terminal |= state.frontier
    state.frontier = set()

    if not used_no_payload_fallback:
        revise_orphan_self_files(
            rtos_root=rtos_root,
            rtos_id=rtos_id,
            role=role,
            state=state,
            provider=provider,
            budget=budget,
            mcu_family=target_mcu,
            sample_size=sample_size,
        )

    return state


def _build_orphan_block(
    rtos_root: Path,
    orphan_paths: list[str],
    sample_size: int,
) -> str:
    lines: list[str] = []
    for p in orphan_paths:
        abs_p = rtos_root / p if p else rtos_root
        loose: list[str] = []
        if abs_p.is_dir():
            loose = sorted(
                f.name for f in abs_p.iterdir()
                if f.is_file() and f.suffix in _SOURCE_EXTS
            )
        listed = loose[: max(sample_size * 4, 12)]
        more = len(loose) - len(listed)
        listed_str = ", ".join(listed) if listed else "(none)"
        if more > 0:
            listed_str += f", ... (+{more} more)"
        lines.append(f"  {p}/ ({len(loose)} loose source files): {listed_str}")
    return "\n".join(lines)


def revise_orphan_self_files(
    *,
    rtos_root: Path,
    rtos_id: str,
    role: RoleSpec,
    state: TriageState,
    provider: Any,
    budget,
    mcu_family: str | None,
    sample_size: int,
) -> None:
    minimal_kept = state.kept_terminal - {""}
    orphans = [
        p
        for p in sorted(state.kept_self_files - state.kept_terminal)
        if p
        and not any(
            (kept == p or kept.startswith(p + "/")) for kept in minimal_kept
        )
    ]
    if not orphans:
        return

    role_keywords_block = ""
    if role.positive_keywords or role.negative_keywords:
        role_keywords_block = (
            "## Role keyword evidence\n\n"
            f"Positive (likely on-role): {_format_keyword_list(role.positive_keywords)}\n\n"
            f"Negative (off-role): {_format_keyword_list(role.negative_keywords)}\n"
        )

    sys_prompt = ORPHAN_REVIEW_SYSTEM_PROMPT.format(
        role_short=role.short_label,
        in_scope_definition=role.in_scope_definition,
        drop_principle=role.drop_principle,
        role_keywords_block=role_keywords_block,
    )

    mcu_user_block = ""
    target_mcu = (mcu_family or "").strip()
    if target_mcu:
        mcu_user_block = f"Target MCU family: {target_mcu}\n"

    user_prompt = ORPHAN_REVIEW_USER_PROMPT.format(
        rtos_id=rtos_id,
        role_short=role.short_label,
        mcu_user_block=mcu_user_block,
        orphan_block=_build_orphan_block(rtos_root, orphans, sample_size),
    )

    schema = {
        "type": "object",
        "properties": {
            "revisions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {"type": "string"},
                        "self_files": {
                            "type": "string",
                            "enum": ["KEEP", "DROP"],
                        },
                        "reason": {"type": "string", "minLength": 4},
                    },
                    "required": ["path", "self_files", "reason"],
                    "additionalProperties": False,
                },
            }
        },
        "required": ["revisions"],
        "additionalProperties": False,
    }

    payload, telemetry = call_llm_json(
        provider=provider,
        call_kind="directory_router",
        task_name=f"scope_triage__{rtos_id}__{role.name}__orphan_review",
        schema=schema,
        system_prompt=sys_prompt,
        user_prompt=user_prompt,
        budget=budget,
    )

    review_record: dict[str, Any] = {
        "round": "orphan_review",
        "orphan_parents": orphans,
        "decisions": [],
        "telemetry": telemetry,
    }

    if not payload or not isinstance(payload, dict):
        review_record["error"] = "no_payload_or_skipped"
        state.audit.append(review_record)
        return

    seen: set[str] = set()
    for d in payload.get("revisions", []) or []:
        p = d.get("path", "").strip().strip("/")
        sf = d.get("self_files", "")
        r = d.get("reason", "")
        if p not in orphans or sf not in ("KEEP", "DROP"):
            continue
        seen.add(p)
        review_record["decisions"].append({"path": p, "self_files": sf, "reason": r})
        if sf == "DROP":
            state.kept_self_files.discard(p)

    omitted = [p for p in orphans if p not in seen]
    if omitted:
        review_record["llm_omitted_kept_unchanged"] = omitted

    state.audit.append(review_record)


def derive_scope_fragment(state: TriageState) -> dict[str, Any]:
    minimal_kept = sorted(state.kept_terminal)
    minimal_dropped = sorted(state.dropped)

    self_files_kept = sorted(state.kept_self_files - state.kept_terminal)
    self_files_orphans = [
        p for p in self_files_kept
        if not any(
            (kept == p or kept.startswith(p + "/"))
            for kept in minimal_kept
        )
    ]

    include_patterns: list[str] = [f"{p}/**" for p in minimal_kept if p]
    self_file_patterns: list[str] = []
    for p in self_files_kept:
        if not p:
            continue
        for ext in ("h", "c", "cpp", "cxx", "cc", "S", "s"):
            self_file_patterns.append(f"{p}/*.{ext}")

    return {
        "rtos_id": state.rtos_id,
        "role": state.role.name,
        "_kept_terminal_count": len(minimal_kept),
        "_dropped_count": len(minimal_dropped),
        "_self_files_kept_dirs": self_files_kept,
        "_self_files_orphan_parents": self_files_orphans,
        "include_dir_patterns": include_patterns,
        "include_file_patterns": self_file_patterns,
        "exclude_dir_patterns": [f"{p}/**" for p in minimal_dropped if p],
    }


__all__ = [
    "PROMPT_VERSION",
    "RoleSpec",
    "DirSnapshot",
    "TriageState",
    "ROLES",
    "UNIVERSAL_DROP_NAMES",
    "role_for_bus",
    "canonical_mcu_family",
    "enumerate_layer",
    "build_system_prompt",
    "build_user_prompt",
    "build_schema",
    "render_dir_snapshot",
    "run_triage_for_role",
    "revise_orphan_self_files",
    "derive_scope_fragment",
]
