"""Non-symbol slot fulfillment for RTOS evidence artifacts."""

from __future__ import annotations

import re
from typing import Any, Iterable, Mapping

from .slot_guard import assess_symbol_fit
from .types import RankedSymbolCandidate, SlotGoal, SlotPlan, SymbolBinding, TaskSpec


_TYPE_KINDS = frozenset({"struct", "typedef", "enum"})
_STATUS_KINDS = frozenset({"macro", "enum", "typedef"})
_MIN_RANK_EVIDENCE_SCORE = 10.0
_STATUS_TOKENS = frozenset(
    {
        "ok",
        "eok",
        "success",
        "status",
        "result",
        "ret",
        "return",
        "err",
        "error",
        "errno",
        "fail",
        "failed",
        "timeout",
        "timedout",
        "busy",
        "invalid",
        "eio",
        "einval",
        "enodev",
        "nomem",
        "enomem",
    }
)
_SPLIT_RE = re.compile(r"[^A-Za-z0-9]+")
_IDENT_RE = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
_C_BUILTIN_TYPES = frozenset(
    {
        "void",
        "char",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
        "bool",
        "_Bool",
        "size_t",
        "ssize_t",
        "uintptr_t",
        "intptr_t",
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
        "uint_fast8_t",
        "uint_fast16_t",
        "uint_fast32_t",
        "uint_fast64_t",
        "int_fast8_t",
        "int_fast16_t",
        "int_fast32_t",
        "int_fast64_t",
    }
)
_C_QUALIFIERS = frozenset(
    {
        "const",
        "volatile",
        "restrict",
        "static",
        "extern",
        "inline",
        "__inline",
        "__inline__",
        "__weak",
        "__packed",
        "struct",
        "enum",
        "union",
    }
)


def _present(value: Any) -> bool:
    if value is None:
        return False
    if value == "":
        return False
    if isinstance(value, (list, tuple, set, frozenset)):
        return any(_present(v) for v in value)
    if isinstance(value, Mapping):
        return any(_present(v) for v in value.values())
    return True


def _clean_mapping(value: Any) -> dict:
    if not isinstance(value, Mapping):
        return {}
    return {str(k): v for k, v in value.items() if _present(v)}


def _tokens(value: str) -> set[str]:
    raw = value or ""
    parts = {p.lower() for p in _SPLIT_RE.split(raw) if p}
    # Keep the full normalized identifier as an additional token.
    normalized = re.sub(r"[^A-Za-z0-9]+", "_", raw).strip("_").lower()
    if normalized:
        parts.add(normalized)
    return parts


def _candidate_payload(cand: RankedSymbolCandidate) -> dict:
    sym = cand.sketch
    declared_in = f"{sym.root_id}:{sym.file}" if sym.root_id or sym.file else None
    return {
        "symbol": sym.name,
        "kind": sym.kind,
        "declared_in": declared_in,
        "root_id": sym.root_id,
        "file": sym.file,
        "file_kind": getattr(sym, "file_kind", None),
        "dir_role_hint": getattr(sym, "dir_role_hint", None),
        "score": round(float(cand.score), 3),
        "match_reasons": list(cand.match_reasons or []),
    }


def _eligible_candidates(
    slot: SlotGoal,
    ranks: dict[str, list[RankedSymbolCandidate]],
    *,
    allowed_kinds: Iterable[str],
) -> list[RankedSymbolCandidate]:
    allowed = set(allowed_kinds)
    out: list[RankedSymbolCandidate] = []
    for cand in ranks.get(slot.slot_id, []) or []:
        if cand.sketch.kind not in allowed:
            continue
        assessment = assess_symbol_fit(slot, cand)
        if assessment.hard_reject:
            continue
        out.append(cand)
    out.sort(key=lambda c: c.score, reverse=True)
    return out


def _confidence_from_candidates(candidates: list[RankedSymbolCandidate]) -> float:
    if not candidates:
        return 0.0
    # Rank scores are heuristic and unbounded; squash them into an
    # evidence confidence without pretending they are probabilities.
    top = max(float(c.score) for c in candidates)
    return round(max(0.5, min(0.9, 0.45 + top / 80.0)), 3)


def _is_builtin_or_qualifier(token: str) -> bool:
    lowered = token.lower()
    if token in _C_BUILTIN_TYPES or token in _C_QUALIFIERS:
        return True
    return bool(re.fullmatch(r"(?:rt_)?u?int(?:8|16|32|64)?_t", lowered))


def _looks_type_like(token: str) -> bool:
    if not token or _is_builtin_or_qualifier(token):
        return False
    lowered = token.lower()
    if lowered.endswith("_t"):
        return True
    if any(marker in lowered for marker in ("typedef", "config", "driver", "handle", "state")):
        return True
    if any(marker in lowered for marker in ("status", "error", "err", "result", "msg")):
        return True
    return any(ch.isupper() for ch in token) and any(ch.islower() for ch in token)


def _looks_status_type_like(token: str) -> bool:
    if not _looks_type_like(token):
        return False
    lowered = token.lower()
    return any(marker in lowered for marker in ("status", "error", "err", "result", "msg"))


def _type_tokens_from_signature(
    signature: str,
    function_name: str | None,
    *,
    return_only: bool = False,
) -> list[str]:
    """Best-effort C signature type-token extractor."""
    if not signature:
        return []
    signature = re.sub(r"/\*.*?\*/", " ", signature, flags=re.DOTALL)
    signature = re.sub(r"//.*", " ", signature)

    out: list[str] = []
    before_params, _, after_open = signature.partition("(")
    params, _, _ = after_open.rpartition(")")

    # Return type: tokens before the function name.
    prefix = before_params
    if function_name and function_name in before_params:
        prefix = before_params.split(function_name, 1)[0]
    for token in _IDENT_RE.findall(prefix):
        if _looks_type_like(token) and token != function_name:
            out.append(token)

    if return_only:
        return out

    # Parameter types: for "TYPE name", drop the final identifier as
    # the parameter name.  Keep struct/enum payload names.
    for raw_param in params.split(","):
        raw_tokens = [t for t in _IDENT_RE.findall(raw_param) if t not in _C_QUALIFIERS]
        tokens = [t for t in raw_tokens if not _is_builtin_or_qualifier(t)]
        if not tokens:
            continue
        if (
            len(tokens) == 1
            and raw_tokens
            and tokens[0] == raw_tokens[-1]
            and any(_is_builtin_or_qualifier(t) for t in raw_tokens[:-1])
        ):
            # Skip leftover parameter names after builtin type filtering.
            continue
        type_tokens = tokens[:-1] if len(tokens) > 1 else tokens
        for token in type_tokens:
            if _looks_type_like(token) and token != function_name:
                out.append(token)

    seen: set[str] = set()
    deduped: list[str] = []
    for token in out:
        if token in seen:
            continue
        seen.add(token)
        deduped.append(token)
    return deduped


def _signature_type_payloads(
    *,
    slot: SlotGoal,
    bindings: dict[str, SymbolBinding],
    status_only: bool = False,
) -> list[dict]:
    domain = (slot.canonical_bus or slot.slot_id.split(".", 1)[0]).lower()
    out: list[dict] = []
    for binding in bindings.values():
        b_slot = (binding.slot_id or "").lower()
        if b_slot == slot.slot_id.lower():
            continue
        if (binding.kind or "").lower() != "function":
            continue
        if status_only:
            if not b_slot.startswith(
                ("runtime.", "timing.", "i2c.", "spi.", "uart.", "gpio.", "adc.", "pwm.", "can.")
            ):
                continue
        elif domain and not b_slot.startswith(f"{domain}."):
            continue
        if not binding.signature:
            continue
        for token in _type_tokens_from_signature(
            binding.signature,
            binding.symbol,
            return_only=status_only,
        ):
            if status_only and not _looks_status_type_like(token):
                continue
            out.append(
                {
                    "symbol": token,
                    "kind": "type_ref",
                    "declared_in": binding.declared_in,
                    "source_slot": binding.slot_id,
                    "source_symbol": binding.symbol,
                    "signature": binding.signature,
                    "score": binding.confidence,
                    "match_reasons": ["bound-api-signature-type-ref"],
                }
            )
    seen: set[str] = set()
    deduped: list[dict] = []
    for item in out:
        symbol = item["symbol"]
        if symbol in seen:
            continue
        seen.add(symbol)
        deduped.append(item)
    return deduped


_STATUS_RETURN_TYPE_NAMES = {
    "int",
    "ssize_t",
    "status_t",
    "error_t",
    "err_t",
    "result_t",
}


def _return_type_name(signature: str, function_name: str | None) -> str | None:
    if not signature:
        return None
    signature = re.sub(r"/\*.*?\*/", " ", signature, flags=re.DOTALL)
    signature = re.sub(r"//.*", " ", signature)
    before_params, _, _ = signature.partition("(")
    prefix = before_params
    if function_name and function_name in before_params:
        prefix = before_params.split(function_name, 1)[0]
    tokens = [t for t in _IDENT_RE.findall(prefix) if t not in _C_QUALIFIERS]
    if not tokens:
        return None
    # Keep the final identifier of the return-type prefix.  For
    # "const enum foo_status" this is foo_status; for "int" it is int.
    return tokens[-1]


def _looks_status_return_type(type_name: str | None) -> bool:
    if not type_name:
        return False
    lowered = type_name.lower()
    if lowered in _STATUS_RETURN_TYPE_NAMES:
        return True
    return any(marker in lowered for marker in ("status", "error", "err", "result"))


def _return_status_payloads(bindings: dict[str, SymbolBinding]) -> list[dict]:
    out: list[dict] = []
    for binding in bindings.values():
        b_slot = (binding.slot_id or "").lower()
        if not b_slot.startswith(
            ("runtime.", "timing.", "i2c.", "spi.", "uart.", "gpio.", "adc.", "pwm.", "can.")
        ):
            continue
        if (binding.kind or "").lower() != "function" or not binding.signature:
            continue
        return_type = _return_type_name(binding.signature, binding.symbol)
        if not _looks_status_return_type(return_type):
            continue
        out.append(
            {
                "symbol": "int_status_return" if return_type == "int" else str(return_type),
                "kind": "type_ref",
                "declared_in": binding.declared_in,
                "source_slot": binding.slot_id,
                "source_symbol": binding.symbol,
                "signature": binding.signature,
                "score": binding.confidence,
                "match_reasons": ["bound-api-return-status-convention"],
            }
        )
    seen: set[tuple[str, str | None]] = set()
    deduped: list[dict] = []
    for item in out:
        key = (str(item.get("symbol")), item.get("source_symbol"))
        if key in seen:
            continue
        seen.add(key)
        deduped.append(item)
    return deduped


def _context_bus_instance(slot: SlotGoal, task_spec: TaskSpec) -> dict | None:
    if slot.slot_id != "integration.bus_instance_binding":
        return None
    cb = task_spec.connection_binding or {}
    bi = task_spec.bus_intent
    value = cb.get("bus_instance") or getattr(bi, "bus_instance", None)
    bus_symbol = cb.get("bus_symbol")
    if not _present(value) and not _present(bus_symbol):
        return None
    fields = {
        "bus_instance": value,
        "bus_symbol": bus_symbol,
        "backend": cb.get("backend") or getattr(bi, "backend", None),
        "mode": cb.get("mode") or getattr(bi, "mode", None),
        "address_mode": cb.get("address_mode") or getattr(bi, "address_mode", None),
    }
    return {
        "slot_id": slot.slot_id,
        "fulfillment_kind": "context_binding",
        "source": "task_context.connection_binding",
        "confidence": 0.95,
        "value": value or bus_symbol,
        "fields": {k: v for k, v in fields.items() if _present(v)},
        "notes": [
            "Concrete bus instance comes from the task package / board context, not from an RTOS API symbol."
        ],
    }


def _context_pin_binding(slot: SlotGoal, task_spec: TaskSpec) -> dict | None:
    if slot.slot_id != "integration.pin_binding":
        return None
    cb = task_spec.connection_binding or {}
    da = task_spec.device_attachment or {}
    fixed = _clean_mapping(cb.get("fixed_attachment") or {})

    required_attachment = _clean_mapping(da.get("required_attachment") or {})
    pinish = {
        key: value
        for key, value in required_attachment.items()
        if any(token in key.lower() for token in ("pin", "gpio", "line", "signal"))
        and _present(value)
    }
    if not fixed and not pinish:
        return None
    return {
        "slot_id": slot.slot_id,
        "fulfillment_kind": "context_binding",
        "source": "task_context.connection_binding",
        "confidence": 0.95,
        "value": fixed or pinish,
        "fields": {
            "fixed_attachment": fixed,
            "required_attachment": pinish,
        },
        "notes": [
            "Concrete signal-to-pin mapping is a board/task binding; repo symbols are only supporting definitions."
        ],
    }


def _context_addressing(
    slot: SlotGoal,
    task_spec: TaskSpec,
    device_ir: Mapping[str, Any] | None,
) -> dict | None:
    sid = slot.slot_id.lower()
    if not sid.endswith(".addressing"):
        return None
    da = task_spec.device_attachment or {}
    bi = task_spec.bus_intent
    ar = (device_ir or {}).get("address_rule") if isinstance(device_ir, Mapping) else None
    ar = ar if isinstance(ar, Mapping) else {}
    ar_type = str(ar.get("type") or "").strip().lower()
    if ar_type == "none":
        ar = {}

    addressing = da.get("addressing")
    address_mode = da.get("address_mode") or getattr(bi, "address_mode", None)
    fields = {
        "address_rule": dict(ar) if ar else {},
        "addressing": addressing,
        "address_mode": address_mode,
    }
    fields = {k: v for k, v in fields.items() if _present(v)}
    if not fields:
        return None
    return {
        "slot_id": slot.slot_id,
        "fulfillment_kind": "context_binding",
        "source": "device_ir.address_rule+task_context.device_attachment",
        "confidence": 0.9,
        "value": fields.get("address_rule") or fields.get("addressing") or address_mode,
        "fields": fields,
        "notes": [
            "Device addressing is a device/task fact; the RTOS API only consumes the chosen address value."
        ],
    }


def _multi_symbol_key_types(
    slot: SlotGoal,
    ranks: dict[str, list[RankedSymbolCandidate]],
    bindings: dict[str, SymbolBinding],
) -> dict | None:
    sid = slot.slot_id.lower()
    expected = set(slot.expected_kinds or [])
    if not (sid.endswith(".key_types") or ("type" in sid and expected & _TYPE_KINDS)):
        return None
    signature_symbols = _signature_type_payloads(slot=slot, bindings=bindings)
    if signature_symbols:
        return {
            "slot_id": slot.slot_id,
            "fulfillment_kind": "multi_symbol_evidence",
            "source": "bound_symbol_signatures",
            "confidence": 0.85,
            "symbols": signature_symbols[:8],
            "notes": [
                "This slot is derived from the type references used by already-bound bus APIs."
            ],
        }

    candidates = [
        c
        for c in _eligible_candidates(slot, ranks, allowed_kinds=expected & _TYPE_KINDS or _TYPE_KINDS)
        if c.score >= _MIN_RANK_EVIDENCE_SCORE
    ]
    if not candidates:
        return None
    symbols = [_candidate_payload(c) for c in candidates[:8]]
    return {
        "slot_id": slot.slot_id,
        "fulfillment_kind": "multi_symbol_evidence",
        "source": "ranked_candidates",
        "confidence": _confidence_from_candidates(candidates),
        "symbols": symbols,
        "notes": [
            "This slot represents a small type vocabulary rather than one canonical call target."
        ],
    }


def _status_name_matches(name: str) -> bool:
    tokens = _tokens(name)
    if tokens & _STATUS_TOKENS:
        return True
    lowered = (name or "").lower()
    return any(
        marker in lowered
        for marker in ("no_error", "is_error", "_err", "err_", "_ok", "ok_", "timeout")
    )


def _multi_symbol_error_status(
    slot: SlotGoal,
    ranks: dict[str, list[RankedSymbolCandidate]],
    bindings: dict[str, SymbolBinding],
) -> dict | None:
    sid = slot.slot_id.lower()
    expected = set(slot.expected_kinds or [])
    wants_status = (
        sid == "runtime.error_status"
        or ("error" in sid or "status" in sid)
        and bool(expected & _STATUS_KINDS)
    )
    if not wants_status:
        return None
    signature_symbols = _signature_type_payloads(
        slot=slot,
        bindings=bindings,
        status_only=True,
    )
    if signature_symbols:
        return {
            "slot_id": slot.slot_id,
            "fulfillment_kind": "multi_symbol_evidence",
            "source": "bound_symbol_signatures",
            "confidence": 0.8,
            "symbols": signature_symbols[:8],
            "notes": [
                "Status/error evidence is derived from return/status types used by bound APIs."
            ],
        }
    return_status_symbols = _return_status_payloads(bindings)
    if return_status_symbols:
        return {
            "slot_id": slot.slot_id,
            "fulfillment_kind": "multi_symbol_evidence",
            "source": "bound_api_return_status_convention",
            "confidence": 0.75,
            "symbols": return_status_symbols[:8],
            "notes": [
                "Status/error handling is represented by the return convention of already-bound RTOS/bus APIs."
            ],
        }

    candidates = [
        c
        for c in _eligible_candidates(slot, ranks, allowed_kinds=expected & _STATUS_KINDS or _STATUS_KINDS)
        if c.score >= _MIN_RANK_EVIDENCE_SCORE and _status_name_matches(c.sketch.name)
    ]
    if not candidates:
        return None
    symbols = [_candidate_payload(c) for c in candidates[:8]]
    return {
        "slot_id": slot.slot_id,
        "fulfillment_kind": "multi_symbol_evidence",
        "source": "ranked_candidates",
        "confidence": _confidence_from_candidates(candidates),
        "symbols": symbols,
        "notes": [
            "Status/error handling is represented as an evidence set; the downstream driver chooses the appropriate value per path."
        ],
    }


def _ops_in_order(steps: list[dict], predicates: list) -> list[dict] | None:
    pos = 0
    matched: list[dict] = []
    for step in steps:
        if pos >= len(predicates):
            break
        if predicates[pos](step):
            matched.append(step)
            pos += 1
    return matched if pos == len(predicates) else None


def _step_is_bound(
    step: Mapping[str, Any],
    bindings: dict[str, SymbolBinding],
    fulfilled_slots: set[str] | None = None,
) -> bool:
    if step.get("bound_symbol"):
        return True
    slot_id = step.get("slot_id")
    return bool(slot_id and (slot_id in bindings or slot_id in (fulfilled_slots or set())))


def _transaction_template_fulfillment(
    slot: SlotGoal,
    bindings: dict[str, SymbolBinding],
    transaction_templates: list[dict],
    fulfilled_slots: set[str] | None = None,
) -> dict | None:
    sid = slot.slot_id.lower()
    if not sid.endswith(".command_write_then_delay_then_read"):
        return None
    bus = (slot.canonical_bus or sid.split(".", 1)[0]).lower()
    if not bus:
        return None

    fulfilled_slots = fulfilled_slots or set()

    def is_write(step: Mapping[str, Any]) -> bool:
        return step.get("op") == f"{bus}_write" and _step_is_bound(
            step, bindings, fulfilled_slots
        )

    def is_delay(step: Mapping[str, Any]) -> bool:
        return str(step.get("op") or "").startswith("delay_") and _step_is_bound(
            step, bindings, fulfilled_slots
        )

    def is_read(step: Mapping[str, Any]) -> bool:
        return step.get("op") == f"{bus}_read" and _step_is_bound(
            step, bindings, fulfilled_slots
        )

    for template in transaction_templates or []:
        steps = template.get("steps") if isinstance(template, Mapping) else None
        if not isinstance(steps, list):
            continue
        matched = _ops_in_order(steps, [is_write, is_delay, is_read])
        if not matched:
            continue
        return {
            "slot_id": slot.slot_id,
            "fulfillment_kind": "transaction_template",
            "source": "device_ir.transaction_template",
            "confidence": float(template.get("confidence") or 0.75),
            "template_id": template.get("template_id"),
            "component_slots": [m.get("slot_id") for m in matched if m.get("slot_id")],
            "component_symbols": [m.get("bound_symbol") for m in matched if m.get("bound_symbol")],
            "matched_ops": [m.get("op") for m in matched],
            "notes": [
                "Composite command/write-delay-read behavior is represented by a transaction template over primitive APIs."
            ],
        }
    return None


def _multi_symbol_payload_for_slot(
    slot: SlotGoal,
    ranks: dict[str, list[RankedSymbolCandidate]],
    bindings: dict[str, SymbolBinding],
) -> dict | None:
    return _multi_symbol_key_types(slot, ranks, bindings) or _multi_symbol_error_status(
        slot, ranks, bindings
    )


def _helper_alias_fulfillment(
    slot: SlotGoal,
    bindings: dict[str, SymbolBinding],
) -> dict | None:
    sid = slot.slot_id.lower()
    if sid.startswith("task_helper."):
        return None
    if sid.endswith((".key_types", ".addressing", ".command_write_then_delay_then_read")):
        return None

    domain, _, op = sid.partition(".")
    if not op:
        return None
    helper_bindings = [
        b
        for b in bindings.values()
        if (b.slot_id or "").lower().startswith("task_helper.") and b.symbol
    ]
    if not helper_bindings:
        return None

    def helper_payload(binding: SymbolBinding) -> dict:
        return {
            "symbol": binding.symbol,
            "kind": binding.kind,
            "declared_in": binding.declared_in,
            "source_slot": binding.slot_id,
            "source_kind": binding.source_kind,
            "signature": binding.signature,
            "score": binding.confidence,
            "match_reasons": ["task-helper-alias"],
        }

    for binding in helper_bindings:
        name = binding.symbol or ""
        norm = re.sub(r"[^A-Za-z0-9]+", "_", name).strip("_").lower()
        tokens = _tokens(name)
        source_slot = (binding.slot_id or "").lower()
        source_tokens = _tokens(source_slot)
        all_tokens = tokens | source_tokens

        if domain in {"i2c", "spi", "uart", "gpio"}:
            op_tokens = {"read"} if op == "read" else {"write"} if op == "write" else {"transfer"}
            if op == "acquire_or_bind":
                op_tokens = {"bind", "binding", "find", "open", "get", "ready"}
            if domain in all_tokens and (all_tokens & op_tokens or any(t in norm for t in op_tokens)):
                return {
                    "slot_id": slot.slot_id,
                    "fulfillment_kind": "helper_alias",
                    "source": "task_helper.binding",
                    "confidence": min(0.82, float(binding.confidence or 0.75)),
                    "symbols": [helper_payload(binding)],
                    "notes": [
                        "A task-package helper with matching bus/operation semantics covers this primitive slot."
                    ],
                }

        if sid == "runtime.delay_ms":
            delayish = (
                "delay" in norm
                or "msleep" in norm
                or "mdelay" in norm
                or "sleep" in norm
            )
            if delayish and "us" not in all_tokens and "micro" not in all_tokens:
                return {
                    "slot_id": slot.slot_id,
                    "fulfillment_kind": "helper_alias",
                    "source": "task_helper.binding",
                    "confidence": min(0.82, float(binding.confidence or 0.75)),
                    "symbols": [helper_payload(binding)],
                    "notes": [
                        "A task-package millisecond delay/sleep helper covers this runtime delay slot."
                    ],
                }

    return None


def build_slot_fulfillments(
    *,
    task_spec: TaskSpec,
    slot_plan: SlotPlan,
    bindings: dict[str, SymbolBinding],
    ranks: dict[str, list[RankedSymbolCandidate]] | None = None,
    transaction_templates: list[dict] | None = None,
    device_ir: Mapping[str, Any] | None = None,
) -> dict[str, dict]:
    """Return non-symbol fulfillment payloads keyed by slot id."""
    ranks = ranks or {}
    out: dict[str, dict] = {}
    for slot in slot_plan.slots:
        if slot.slot_id in bindings:
            payload = _multi_symbol_payload_for_slot(slot, ranks, bindings)
            if payload is not None:
                out[slot.slot_id] = payload
            continue
        payload = (
            _context_bus_instance(slot, task_spec)
            or _context_pin_binding(slot, task_spec)
            or _context_addressing(slot, task_spec, device_ir)
            or _helper_alias_fulfillment(slot, bindings)
            or _multi_symbol_payload_for_slot(slot, ranks, bindings)
        )
        if payload is not None:
            out[slot.slot_id] = payload

    fulfilled = set(bindings) | set(out)
    for slot in slot_plan.slots:
        if slot.slot_id in fulfilled:
            continue
        payload = _transaction_template_fulfillment(
            slot,
            bindings,
            transaction_templates or [],
            fulfilled_slots=fulfilled,
        )
        if payload is not None:
            out[slot.slot_id] = payload
            fulfilled.add(slot.slot_id)
    return out


def fulfilled_slot_ids(
    bindings: dict[str, SymbolBinding],
    slot_fulfillments: dict[str, dict] | None = None,
) -> set[str]:
    return set(bindings) | set(slot_fulfillments or {})


__all__ = ["build_slot_fulfillments", "fulfilled_slot_ids"]
