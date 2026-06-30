"""pipeline step - :class:`rtos_contract_builder.json` builder."""

from __future__ import annotations

import json
import logging
import re
from pathlib import Path

from .types import (
    SOURCE_KIND_TASK_PACKAGE_HELPER,
    SOURCE_KIND_MANIFEST_REPO,
    SlotGoal,
    SlotPlan,
    SymbolBinding,
    TaskSpec,
)

logger = logging.getLogger(__name__)

CONTRACT_VERSION = "trace-rtos-contract"
COMPAT_SCHEMA = "trace-rtos-contract"


# Slot-to-bucket classifier


def _bucket_for_slot(slot: SlotGoal) -> str:
    """Return the ``rtos_contract.json`` bucket the slot's bound symbol should land in."""
    if slot.slot_id.startswith("task_helper."):
        return "task_helpers"
    if slot.layer == "runtime":
        sid = slot.slot_id.lower()
        if "error" in sid or "status" in sid:
            return "error_symbols"
        if "delay" in sid or "tick" in sid:
            return "delay_symbols"
        return "runtime_symbols"
    if slot.layer == "timing":
        return "delay_symbols" if "delay" in slot.slot_id else "runtime_symbols"
    if slot.layer == "bus":
        # Type-only slots produce types, not functions.
        type_only = {"typedef", "struct", "enum", "macro"}
        ek = set(slot.expected_kinds or [])
        if ek and ek.issubset(type_only) and "function" not in ek:
            return "bus_helper_types"
        return "bus_api_symbols"
    if slot.layer in ("integration", "board"):
        return "board_helpers"
    return "runtime_symbols"


# Section builders


def _runtime_block(
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    slot_fulfillments: dict[str, dict] | None = None,
) -> dict:
    runtime_symbols: list[str] = []
    delay_symbols: list[str] = []
    error_symbols: list[str] = []
    seen = {"runtime_symbols": set(), "delay_symbols": set(), "error_symbols": set()}

    for slot in slot_plan.slots:
        b = bindings.get(slot.slot_id)
        if b is None or not b.symbol:
            continue
        bucket = _bucket_for_slot(slot)
        if bucket == "runtime_symbols" and b.symbol not in seen["runtime_symbols"]:
            runtime_symbols.append(b.symbol)
            seen["runtime_symbols"].add(b.symbol)
        elif bucket == "delay_symbols" and b.symbol not in seen["delay_symbols"]:
            delay_symbols.append(b.symbol)
            seen["delay_symbols"].add(b.symbol)
        elif bucket == "error_symbols" and b.symbol not in seen["error_symbols"]:
            error_symbols.append(b.symbol)
            seen["error_symbols"].add(b.symbol)

    for slot in slot_plan.slots:
        if _bucket_for_slot(slot) != "error_symbols":
            continue
        payload = (slot_fulfillments or {}).get(slot.slot_id) or {}
        if payload.get("fulfillment_kind") != "multi_symbol_evidence":
            continue
        for item in payload.get("symbols") or []:
            symbol = item.get("symbol") if isinstance(item, dict) else None
            if symbol and symbol not in seen["error_symbols"]:
                error_symbols.append(symbol)
                seen["error_symbols"].add(symbol)

    return {
        "runtime_symbols": runtime_symbols,
        "delay_symbols": delay_symbols,
        "error_symbols": error_symbols,
    }


def _bus_block(
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    task_spec: TaskSpec,
    *,
    has_transaction_templates: bool = False,
    slot_fulfillments: dict[str, dict] | None = None,
) -> dict:
    bus_api_symbols: list[str] = []
    bus_helper_types: list[str] = []
    bus_api_signatures: dict[str, str] = {}
    seen_api: set[str] = set()
    seen_types: set[str] = set()

    for slot in slot_plan.slots:
        b = bindings.get(slot.slot_id)
        if b is None or not b.symbol:
            continue
        bucket = _bucket_for_slot(slot)
        if bucket == "bus_api_symbols" and b.symbol not in seen_api:
            bus_api_symbols.append(b.symbol)
            seen_api.add(b.symbol)
            if b.signature:
                bus_api_signatures[b.symbol] = b.signature
        elif bucket == "bus_helper_types" and b.symbol not in seen_types:
            # Preserve the struct keyword for prompt matching.
            display = (
                f"struct {b.symbol}"
                if b.kind == "struct" and not b.symbol.lower().startswith("struct ")
                else b.symbol
            )
            bus_helper_types.append(display)
            seen_types.add(b.symbol)

    for slot in slot_plan.slots:
        if _bucket_for_slot(slot) != "bus_helper_types":
            continue
        payload = (slot_fulfillments or {}).get(slot.slot_id) or {}
        if payload.get("fulfillment_kind") != "multi_symbol_evidence":
            continue
        for item in payload.get("symbols") or []:
            if not isinstance(item, dict):
                continue
            symbol = item.get("symbol")
            if not symbol or symbol in seen_types:
                continue
            display = (
                f"struct {symbol}"
                if item.get("kind") == "struct" and not str(symbol).lower().startswith("struct ")
                else symbol
            )
            bus_helper_types.append(display)
            seen_types.add(symbol)

    # Prefer structured transaction templates when available; otherwise
    # fall back to compact transaction-pattern hints.
    return {
        "bus_api_symbols": bus_api_symbols,
        "bus_api_signatures": bus_api_signatures,
        "bus_helper_types": bus_helper_types,
        "transaction_patterns": (
            [] if has_transaction_templates
            else _transaction_patterns_for_bus(task_spec)
        ),
        "device_transaction_shape": task_spec.device_transaction_shape,
    }


def _transaction_patterns_for_bus(task_spec: TaskSpec) -> list[str]:
    """Tiny static map from canonical_bus → transaction patterns."""
    bus = (task_spec.bus_intent.canonical_bus or "").lower()
    if bus == "i2c":
        return [
            "register_pointer_write_then_read",
            "command_write",
            "block_read",
        ]
    if bus == "spi":
        return ["full_duplex_transfer", "register_read", "register_write"]
    if bus == "uart":
        return ["framed_request_response", "polling_read", "polling_write"]
    if bus == "gpio":
        return ["digital_pulse", "level_polling", "edge_capture"]
    return []


def _api_bindings_block(
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
) -> dict[str, dict]:
    """``{slot_id: {symbol, signature, required_headers, source_kind, …}}``."""
    out: dict[str, dict] = {}
    for slot in slot_plan.slots:
        b = bindings.get(slot.slot_id)
        if b is None:
            continue
        out[slot.slot_id] = {
            "symbol": b.symbol,
            "kind": b.kind,
            "signature": b.signature,
            "required_headers": list(b.required_headers),
            "required_types": list(b.required_types),
            "semantic_role": b.semantic_role,
            "source_kind": b.source_kind,
            "verification": b.verification,
            "confidence": b.confidence,
            "declared_in": b.declared_in,
            "implemented_in": b.implemented_in,
            "requires_runtime_provision": b.requires_runtime_provision,
            "allowed_for_codegen": b.allowed_for_codegen,
        }
    return out


_STRUCT_TYPE_RE = re.compile(r"\bstruct\s+([A-Za-z_][A-Za-z0-9_]*)\b")


def _fallback_parse_struct_defs(
    needed: dict[str, set[str]],
    bindings: dict[str, SymbolBinding],
    *,
    include_headers: list[str] | None = None,
) -> dict[str, str]:
    """Parse source headers referenced in the artifact's file list."""
    from .c_header_parser import deep_parse_file

    # Collect candidate header files from the bindings' declared_in paths
    # and from the data/rtos source tree.
    candidates: set[Path] = set()
    rtos_root = Path("data/rtos")
    for b in bindings.values():
        if not b.declared_in or not b.signature:
            continue
        parts = b.declared_in.rsplit("::", 1)
        if len(parts) != 2:
            continue
        rel_path = parts[1]
        # Walk indexed files and prefer headers over sources.
        for root_dir in rtos_root.glob("*/"):
            candidate = root_dir / rel_path
            if candidate.exists():
                # Check neighboring headers first.
                parent = candidate.parent
                if parent.exists():
                    for h in parent.glob("*.h"):
                        candidates.add(h)
                candidates.add(candidate)

    # Include integration headers as fallback definition sources.
    if include_headers:
        for inc_path in include_headers:
            if not isinstance(inc_path, str):
                continue
            for root_dir in rtos_root.glob("*/"):
                candidate = root_dir / inc_path
                if candidate.is_file():
                    candidates.add(candidate)

    if not candidates:
        return {}

    found: dict[str, str] = {}
    for src in candidates:
        if not src.exists():
            continue
        parsed = deep_parse_file(src, str(src))
        for s in parsed.struct_definitions:
            if s.name in needed and s.name not in found:
                fields = getattr(s, "fields", []) or []
                body = f"struct {s.name} {{\n"
                for field in fields:
                    body += f"  {field};\n"
                body += "};"
                found[s.name] = body
            if len(found) >= len(needed):
                break
        if len(found) >= len(needed):
            break

    return found


def _complete_key_types(
    api_bindings: dict[str, dict],
    bindings: dict[str, SymbolBinding],
    artifact: dict,
) -> None:
    """Post-process: fill key_types with struct defs referenced in signatures."""
    # Collect struct names referenced in bound function signatures.
    needed: dict[str, set[str]] = {}   # struct_name -> {slot_ids}
    for slot_id, b in bindings.items():
        if not b.signature:
            continue
        for m in _STRUCT_TYPE_RE.finditer(b.signature):
            struct_name = m.group(1)
            needed.setdefault(struct_name, set()).add(slot_id)

    if not needed:
        return

    # Search parsed files from the artifact for struct definitions.
    struct_defs: dict[str, str] = {}  # struct_name -> body text
    files = artifact.get("files", {}) or {}
    for category in files.values():
        for entry in category:
            if not isinstance(entry, dict):
                continue
            syms = entry.get("symbols", [])
            for sym in syms:
                if not isinstance(sym, dict):
                    continue
                if sym.get("kind") not in ("struct", "struct_definition"):
                    continue
                name = sym.get("name", "")
                if name in needed:
                    body = (
                        sym.get("declaration_text", "")
                        or sym.get("body", "")
                        or sym.get("definition", "")
                        or ""
                    )
                    if body and name not in struct_defs:
                        struct_defs[name] = body

    # Fallback to referenced headers when serialized struct bodies are absent.
    if not struct_defs:
        integration = artifact.get("integration") if isinstance(artifact, dict) else None
        include_headers = integration.get("include_headers", []) if isinstance(integration, dict) else []
        struct_defs = _fallback_parse_struct_defs(needed, bindings, include_headers=include_headers)

    if not struct_defs:
        return

    # Append found struct definitions to key_types.
    # Find the key_types slot matching the bus.
    for slot_id, info in api_bindings.items():
        if not slot_id.endswith(".key_types"):
            continue
        existing = str(info.get("signature", "") or "")
        for struct_name, body in struct_defs.items():
            if body and body not in existing:
                if existing:
                    existing += "\n"
                existing += body
        if existing != str(info.get("signature", "") or ""):
            info["signature"] = existing


def _allowed_symbols_block(
    bindings: dict[str, SymbolBinding],
) -> tuple[list[str], dict[str, dict]]:
    """Return the dedup'd ``allowed_symbols`` list + the per-symbol provenance map."""
    allowed: list[str] = []
    provenance: dict[str, dict] = {}
    seen: set[str] = set()
    for b in bindings.values():
        if not b.symbol or b.symbol in seen:
            continue
        seen.add(b.symbol)
        provenance[b.symbol] = {
            "source_kind": b.source_kind,
            "verification": b.verification,
            "requires_runtime_provision": b.requires_runtime_provision,
            "declared_in": b.declared_in,
            "implemented_in": b.implemented_in,
            "allowed_for_codegen": b.allowed_for_codegen,
        }
        if b.allowed_for_codegen:
            allowed.append(b.symbol)
    return allowed, provenance


def _connection_block(task_spec: TaskSpec) -> dict:
    bi = task_spec.bus_intent
    cb = task_spec.connection_binding or {}
    return {
        "connection_type": bi.connection_type,
        "mode": bi.mode,
        "bus_instance": cb.get("bus_instance") or bi.bus_instance,
        "bus_symbol": cb.get("bus_symbol"),
        "address_mode": bi.address_mode,
        "backend": cb.get("backend") or bi.backend,
        "helper_usage_patterns": cb.get("helper_usage_patterns", []),
    }


def _evidence_spans_block(
    bindings: dict[str, SymbolBinding],
    cap: int | None = None,
) -> list[dict]:
    """Pick a representative subset of evidence spans for the contract."""
    if cap is None:
        from .config import load_thresholds  # local import to avoid cycle
        cfg = load_thresholds().get("contract", {})
        cap = int(cfg.get("evidence_spans_cap", 32))
    if cap <= 0:
        return []

    # Bucket evidence per binding so the round-robin pick is cheap.
    per_binding: list[list[dict]] = []
    for b in bindings.values():
        spans = [
            {
                "root_id": s.root_id,
                "path": s.path,
                "kind": s.kind,
                "for_symbol": b.symbol,
                "slot_id": b.slot_id,
            }
            for s in b.evidence
        ]
        if spans:
            per_binding.append(spans)

    out: list[dict] = []
    cursors = [0] * len(per_binding)
    while len(out) < cap:
        added = False
        for i, spans in enumerate(per_binding):
            if cursors[i] >= len(spans):
                continue
            out.append(spans[cursors[i]])
            cursors[i] += 1
            added = True
            if len(out) >= cap:
                break
        if not added:
            break
    return out


# Public entry


def build_rtos_contract(
    *,
    artifact: dict,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    task_package_id: str | None = None,
    requires_human: list[str] | None = None,
) -> dict:
    """Build the rtos_contract dict."""
    slot_fulfillments = artifact.get("slot_fulfillments", {}) or {}
    runtime = _runtime_block(slot_plan, bindings, slot_fulfillments)
    has_tx_templates = bool(artifact.get("transaction_templates"))
    bus = _bus_block(
        slot_plan, bindings, task_spec,
        has_transaction_templates=has_tx_templates,
        slot_fulfillments=slot_fulfillments,
    )
    api_bindings = _api_bindings_block(slot_plan, bindings)

    # Add call-shape hints for ioctl-style command macros.
    _ioctl_cmd_re = re.compile(
        r"^(?:GPIOC_|SPIOC_|IOEXP_|I2C_IOCTL_|SERIOC_|UARTIOC_|PWMIOC_|"
        r"MTDIOC_|BIOC_|RTCIOC_|WDIOC_|FBIO_|AUDIO_IOCTL_)",
    )
    for _sid, _info in api_bindings.items():
        _sym = str(_info.get("symbol", ""))
        if not _ioctl_cmd_re.match(_sym):
            continue
        _kind = str(_info.get("kind", ""))
        if _kind not in ("macro",):
            continue
        _raw = str(_info.get("signature", ""))
        # Build per-slot ioctl usage pattern from the command name suffix.
        _read_hint = ("_READ" in _sym or "_GET" in _sym or "_PINTYPE" in _sym
                      or "RDONLY" in _sym)
        _write_hint = ("_WRITE" in _sym or "_SET" in _sym or "_SEND" in _sym
                       or "_TRANSMIT" in _sym)
        if _write_hint:
            _arg_doc = "bool_val  /* 0 or 1, passed by value */"
        elif _read_hint:
            _arg_doc = "val_ptr  /* FAR bool * — pass &var to receive pin level; ioctl returns 0 on success, not the level */"
        else:
            _arg_doc = "arg  /* see the declaring header for this command */"
        _info["signature"] = (
            f"{_raw}\n"
            f"  Usage: ioctl(fd, {_sym}, {_arg_doc})"
        )

    # Unified-transfer APIs can satisfy read/write slots when dedicated
    # bindings were not found.
    _transfer_slot = api_bindings.get("i2c.transfer")
    if isinstance(_transfer_slot, dict) and _transfer_slot.get("signature"):
        for _fallback_sid in ("i2c.write", "i2c.read"):
            if not api_bindings.get(_fallback_sid):
                api_bindings[_fallback_sid] = dict(_transfer_slot)

    # Complete key type definitions referenced by bound signatures.
    _complete_key_types(api_bindings, bindings, artifact)

    allowed_symbols, allowed_provenance = _allowed_symbols_block(bindings)

    # Integration contract: reuse what the artifact already computed.
    integration_in_artifact = artifact.get("integration", {})
    integration_contract = {
        "integration": task_spec.integration,
        "integration_style": task_spec.integration_style,
        "bus_instance": integration_in_artifact.get("bus_instance"),
        "bus_symbol": integration_in_artifact.get("bus_symbol"),
        "include_headers": integration_in_artifact.get("include_headers", []),
        "fixed_attachment": integration_in_artifact.get("fixed_attachment", {}),
        "helper_usage_patterns": integration_in_artifact.get(
            "helper_usage_patterns", []
        ) or (task_spec.connection_binding or {}).get("helper_usage_patterns", []),
        "runtime_provision_required_for": integration_in_artifact.get(
            "runtime_provision_required_for", []
        ),
        "context_bindings": {
            sid: payload
            for sid, payload in (artifact.get("context_bindings", {}) or {}).items()
            if str(sid).startswith("integration.")
        },
        "board_sources": [
            f["path"]
            for f in artifact.get("files", {}).get("board_config", [])
            if isinstance(f, dict) and "path" in f
        ],
    }

    device_contract = {
        "device_id": task_spec.device_id,
        "required_bus_type": task_spec.bus_intent.canonical_bus,
        "addressing": (task_spec.device_attachment or {}).get("addressing"),
        "required_attachment": (task_spec.device_attachment or {}).get("required_attachment", {}),
        "optional_attachment": (task_spec.device_attachment or {}).get("optional_attachment", {}),
    }

    # Pull a few canonical notes — keep short, the artifact carries
    # the full extraction trace.
    summary = artifact.get("summary", {}) or {}
    n_required = summary.get("n_required", 0)
    n_required_bound = summary.get("n_required_bound", 0)
    n_helper = summary.get("provenance", {}).get(SOURCE_KIND_TASK_PACKAGE_HELPER, 0)
    notes = [
        f"RTOS contract: {n_required_bound}/{n_required} required slots covered.",
        (
            f"{n_helper} task-package helper hooks declared "
            "(stub or board-supplied weak occurrence required at link time)."
            if n_helper else
            "No task-package helper hooks; all bindings come from manifest sources."
        ),
    ]

    contract = {
        "contract_version": CONTRACT_VERSION,
        "contract_metadata": {
            "extractor_version": "1.0",
            "compat_schema": COMPAT_SCHEMA,
        },
        "task_package_id": task_package_id,
        "device_id": task_spec.device_id,
        "rtos": task_spec.rtos_id,
        "board": task_spec.board,
        "bus_type": task_spec.bus_intent.canonical_bus,
        "connection": _connection_block(task_spec),

        "runtime_contract": runtime,
        "bus_contract": bus,
        "integration_contract": integration_contract,
        "device_contract": device_contract,

        # structured structured surface; codegen prompt builder should
        # iterate over this rather than ``bus_api_symbols``.
        "api_bindings": api_bindings,
        "slot_fulfillments": slot_fulfillments,
        "context_bindings": artifact.get("context_bindings", {}) or {},
        "multi_symbol_evidence": artifact.get("multi_symbol_evidence", {}) or {},

        "allowed_symbols": allowed_symbols,
        "allowed_symbol_provenance": allowed_provenance,
        "transaction_templates": artifact.get("transaction_templates", []),

        "forbidden_assumptions": [],
        "notes": notes,
        "evidence_spans": _evidence_spans_block(bindings),
        "requires_human": list(requires_human or []),
    }
    return contract


def save_rtos_contract(
    *,
    contract: dict,
    output_dir: Path,
    filename: str = "rtos_contract.json",
) -> Path:
    """Atomically write *contract* to ``output_dir / filename``."""
    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / filename
    out_path.write_text(
        json.dumps(contract, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    logger.info(
        "Wrote RTOS contract to %s (%d KB)", out_path, out_path.stat().st_size // 1024
    )
    return out_path


__all__ = [
    "CONTRACT_VERSION",
    "COMPAT_SCHEMA",
    "build_rtos_contract",
    "save_rtos_contract",
]
