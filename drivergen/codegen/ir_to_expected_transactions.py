"""Mechanical derivation of ``expected_transactions`` from DeviceIR."""
from __future__ import annotations

import dataclasses
import re
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
    ClassifyResult,
)
from .spi_protocol import SpiProtocolHints, spi_protocol_hints


# Public result model

@dataclasses.dataclass(frozen=True)
class ExpectedTransaction:
    """Single derived transaction prefix (immutable for diffing)."""
    phase: str
    addr_or_pin: str
    write_prefix_any_of: Tuple[Tuple[str, ...], ...]
    read_any: bool
    note: str
    source: str
    forbid_write_prefix: bool = False

    def to_dict(self) -> Dict[str, Any]:
        out: Dict[str, Any] = {
            "phase":               self.phase,
            "addr_or_pin":         self.addr_or_pin,
            "write_prefix_any_of": [list(p) for p in self.write_prefix_any_of],
            "note":                self.note,
            "source":              self.source,
        }
        if self.read_any:
            out["read_any"] = True
        if self.forbid_write_prefix:
            out["forbid_write_prefix"] = True
        return out


# Helpers

_HEX_BYTE_RE = re.compile(r"\b0[xX][0-9A-Fa-f]{1,4}\b")
_I2C_ADDRESS_SELECTOR_TOKENS = {
    "addr",
    "address",
    "pin",
    "default",
    "low",
    "high",
    "gnd",
    "vcc",
}


def _canonical_hex_byte(value: Any) -> Optional[str]:
    """Normalise a hex-ish value to ``0xNN`` / ``0xNNNN`` form."""
    if isinstance(value, int):
        if 0 <= value <= 0xFFFF:
            width = 2 if value <= 0xFF else 4
            return f"0x{value:0{width}X}"
        return None
    if isinstance(value, str):
        s = value.strip()
        if not s:
            return None
        if s.lower().startswith("0x"):
            try:
                n = int(s, 16)
            except ValueError:
                return None
            if 0 <= n <= 0xFFFF:
                width = 2 if n <= 0xFF else 4
                return f"0x{n:0{width}X}"
            return None
        # Plain decimal-looking string ("1", "23").
        if s.isdigit():
            try:
                n = int(s, 10)
            except ValueError:
                return None
            if 0 <= n <= 0xFFFF:
                width = 2 if n <= 0xFF else 4
                return f"0x{n:0{width}X}"
        return None
    return None


def _primary_i2c_address(device_ir: Mapping[str, Any]) -> Optional[str]:
    """Pick the canonical 7-bit I2C address from device_ir.address_rule."""
    addr_rule = device_ir.get("address_rule")
    if not isinstance(addr_rule, Mapping):
        return None

    opts = addr_rule.get("address_7bit_options")
    if isinstance(opts, list) and opts:
        canon = _canonical_hex_byte(opts[0])
        if canon is not None:
            return canon
        if isinstance(opts[0], str):
            m = _HEX_BYTE_RE.search(opts[0])
            if m:
                return _canonical_hex_byte(m.group(0))

    scalar = addr_rule.get("address_7bit")
    if scalar is not None:
        canon = _canonical_hex_byte(scalar)
        if canon is not None:
            return canon
        if isinstance(scalar, str):
            m = _HEX_BYTE_RE.search(scalar)
            if m:
                return _canonical_hex_byte(m.group(0))

    nested = addr_rule.get("addresses")
    if isinstance(nested, list) and nested:
        ordered = [
            entry for entry in nested
            if isinstance(entry, Mapping) and entry.get("is_default") is True
        ]
        ordered.extend(entry for entry in nested if entry not in ordered)
        first = ordered[0] if ordered else nested[0]
        if isinstance(first, Mapping):
            # Support the common address key variants used by extracted IR.
            for key in ("addr", "address", "value"):
                val = first.get(key)
                canon = _canonical_hex_byte(val)
                if canon is not None:
                    return canon
                if isinstance(val, str):
                    m = _HEX_BYTE_RE.search(val)
                    if m:
                        return _canonical_hex_byte(m.group(0))

    allowed = addr_rule.get("allowed")
    if isinstance(allowed, list) and allowed:
        for entry in allowed:
            if not isinstance(entry, Mapping):
                continue
            for key in ("value", "address", "addr"):
                val = entry.get(key)
                canon = _canonical_hex_byte(val)
                if canon is not None:
                    return canon
                if isinstance(val, str):
                    m = _HEX_BYTE_RE.search(val)
                    if m:
                        return _canonical_hex_byte(m.group(0))

    rng = addr_rule.get("address_7bit_range")
    if isinstance(rng, str):
        m = _HEX_BYTE_RE.search(rng)
        if m:
            return _canonical_hex_byte(m.group(0))

    base = addr_rule.get("address_7bit_base")
    if isinstance(base, str):
        # Strip variable placeholders before interpreting binary masks.
        b = base.strip()
        if b.lower().startswith("0b"):
            b = b[2:]
        bits = re.sub(r"[A-Za-z]\d*", "0", b)
        if bits and all(c in "01" for c in bits):
            try:
                return _canonical_hex_byte(int(bits, 2))
            except ValueError:
                pass
        # Or plain hex in the field.
        m = _HEX_BYTE_RE.search(base)
        if m:
            return _canonical_hex_byte(m.group(0))

    # Last-resort fallback: scan every string value for a hex literal.
    for v in addr_rule.values():
        if isinstance(v, str):
            m = _HEX_BYTE_RE.search(v)
            if m:
                canon = _canonical_hex_byte(m.group(0))
                if canon is not None:
                    return canon
    return None


def _name_matches_tokens(name: str, tokens: Iterable[str]) -> bool:
    """Case-insensitive token match within a register / command name."""
    lower = name.lower()
    return any(t.lower() in lower for t in tokens)


def _address_entry_value(entry: Mapping[str, Any]) -> Optional[str]:
    for key in ("addr", "address", "value", "address_7bit"):
        val = entry.get(key)
        canon = _canonical_hex_byte(val)
        if canon is not None:
            return canon
        if isinstance(val, str):
            m = _HEX_BYTE_RE.search(val)
            if m:
                return _canonical_hex_byte(m.group(0))
    return None


def _tokenise_for_address_score(value: Any) -> set[str]:
    text = _normalise_flow_token(str(value or ""))
    if not text:
        return set()
    tokens = {tok for tok in text.split("_") if tok}
    expanded = set(tokens)
    for tok in list(tokens):
        if tok in {"accel", "accelerometer", "acceleration", "a"}:
            expanded.update({"accel", "accelerometer", "acceleration"})
        if tok in {"mag", "magnetometer", "magnetic", "m"}:
            expanded.update({"mag", "magnetometer", "magnetic"})
        if tok in {"gyro", "gyroscope"}:
            expanded.update({"gyro", "gyroscope"})
        if tok in {"pressure", "prs"}:
            expanded.update({"pressure", "prs"})
        if tok in {"temp", "temperature", "tmp"}:
            expanded.update({"temp", "temperature", "tmp"})
    return expanded


def _semantic_i2c_address_tokens(tokens: Iterable[str]) -> set[str]:
    out: set[str] = set()
    for token in tokens:
        if len(token) <= 1:
            continue
        if token in _I2C_ADDRESS_SELECTOR_TOKENS:
            continue
        if re.fullmatch(r"\d+(?:v(?:cc|ddio?)?)?", token):
            continue
        out.add(token)
    return out


def _i2c_address_candidates(
    device_ir: Mapping[str, Any],
) -> Tuple[Tuple[str, set[str]], ...]:
    """Return address candidates with human-readable descriptor tokens."""
    addr_rule = device_ir.get("address_rule")
    if not isinstance(addr_rule, Mapping):
        return tuple()
    raw = addr_rule.get("addresses")
    if not isinstance(raw, list):
        return tuple()

    out: List[Tuple[str, set[str]]] = []
    seen: set[str] = set()
    for entry in raw:
        if not isinstance(entry, Mapping):
            continue
        addr = _address_entry_value(entry)
        if addr is None or addr in seen:
            continue
        seen.add(addr)
        text_parts = [
            entry.get("name"),
            entry.get("description"),
            entry.get("condition"),
            entry.get("role"),
        ]
        tokens = set()
        for part in text_parts:
            tokens.update(_tokenise_for_address_score(part))
        tokens = _semantic_i2c_address_tokens(tokens)
        out.append((addr, tokens))
    return tuple(out)


def _context_tokens_for_i2c_address(value: Any) -> set[str]:
    text = " ".join(_walk_flow_strings(value))
    tokens = _tokenise_for_address_score(text)
    norm = _normalise_flow_token(text)
    # Some register names encode sub-device hints as suffixes.
    if re.search(r"(?:^|_)a(?:$|_)", norm):
        tokens.update({"accel", "accelerometer", "acceleration"})
    if re.search(r"(?:^|_)m(?:$|_)", norm) or "mg" in tokens:
        tokens.update({"mag", "magnetometer", "magnetic"})
    return tokens


def _infer_i2c_addr_for_flow_step(
    device_ir: Mapping[str, Any],
    *,
    default_addr: str,
    flow: Mapping[str, Any],
    step: Optional[Mapping[str, Any]] = None,
) -> str:
    candidates = _i2c_address_candidates(device_ir)
    if len(candidates) <= 1:
        return default_addr
    context = {
        "flow_id": flow.get("flow_id"),
        "channels": flow.get("channels"),
        "notes": flow.get("notes"),
        "outputs": flow.get("outputs"),
        "step": step,
    }
    context_tokens = _context_tokens_for_i2c_address(context)
    if not context_tokens:
        return default_addr

    scored: List[Tuple[int, str]] = []
    for addr, tokens in candidates:
        if not tokens:
            continue
        score = len(context_tokens & tokens)
        # Also allow substring-style matching for descriptors like
        # "magnetometer" vs flow_id "read_mag".
        for token in tokens:
            if len(token) >= 4 and any(
                token in ctx or ctx in token
                for ctx in context_tokens
                if len(ctx) >= 3
            ):
                score += 1
        if score > 0:
            scored.append((score, addr))
    if not scored:
        return default_addr
    scored.sort(reverse=True)
    if len(scored) > 1 and scored[0][0] == scored[1][0]:
        return default_addr
    return scored[0][1]


def _extract_opcode(entry: Mapping[str, Any]) -> Optional[str]:
    """Pull out the opcode / command / address hex from a register entry."""
    for key in ("opcode", "command", "cmd", "cmd16", "cmd_byte",
                 "pointer",
                 "address", "register_address", "addr", "register",
                 # Keep generic value last so more specific fields win.
                 "value"):
        v = entry.get(key)
        canon = _canonical_hex_byte(v)
        if canon is not None:
            return canon
    return None


def _hex_word_in_text(hex_lit: str, text: str) -> bool:
    """Return True iff ``hex_lit`` appears as a WHOLE hex word in ``text``."""
    if not hex_lit:
        return False
    start = 0
    n = len(hex_lit)
    while True:
        idx = text.find(hex_lit, start)
        if idx < 0:
            return False
        # Left boundary prevents matching inside a longer token.
        if idx > 0:
            prev = text[idx - 1]
            if prev.isalnum() or prev == "_":
                start = idx + 1
                continue
        # Right boundary prevents matching inside a longer hex literal.
        if idx + n < len(text):
            nxt = text[idx + n]
            if nxt in "0123456789abcdefABCDEF":
                start = idx + 1
                continue
        return True


def _phase_text_bag(device_ir: Mapping[str, Any], phase_key: str) -> str:
    """Concat the free-form text of a phase (init_sequence / read_sequence)."""
    bag: List[str] = []
    for entry in device_ir.get(phase_key) or []:
        if isinstance(entry, Mapping):
            for k in ("action", "details", "description", "summary", "mode"):
                v = entry.get(k)
                if isinstance(v, str):
                    bag.append(v)
            inner = entry.get("steps")
            if isinstance(inner, list):
                for item in inner:
                    if isinstance(item, str):
                        bag.append(item)
        elif isinstance(entry, str):
            bag.append(entry)
    return " ".join(bag)


def _flow_phase(flow: Mapping[str, Any]) -> str:
    kind = str(flow.get("kind") or "").strip().lower()
    if kind in {"init", "probe", "calibration", "power"}:
        return "init"
    if kind == "write":
        return "write_cycle"
    return "read_cycle"


def _literal_prefix_from_transaction(transaction: Mapping[str, Any]) -> Tuple[str, ...]:
    raw = transaction.get("bytes")
    if not isinstance(raw, list):
        return tuple()
    out: List[str] = []
    for entry in raw:
        canon = _canonical_hex_byte(entry)
        if canon is not None:
            out.append(canon)
            continue
        if entry == "DATA":
            break
    return tuple(out)


def _operation_flow_transactions(
    device_ir: Mapping[str, Any],
    *,
    addr_or_pin: str,
    eval_class: str,
) -> List[ExpectedTransaction]:
    """Derive transactions from IR-J operation_flows when present."""
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list) or not flows:
        return []
    selected_flows = _select_codegen_operation_flows(
        device_ir,
        flows,
        eval_class=eval_class,
    )
    if not selected_flows:
        return []

    out: List[ExpectedTransaction] = []
    seen: set[Tuple[str, str, Tuple[Tuple[str, ...], ...], bool, bool]] = set()

    def append(tx: ExpectedTransaction) -> None:
        key = (
            tx.phase,
            tx.addr_or_pin,
            tx.write_prefix_any_of,
            tx.read_any,
            tx.forbid_write_prefix,
        )
        if key in seen:
            return
        seen.add(key)
        out.append(tx)

    access_model = device_ir.get("access_model")
    access_kind = (
        str(access_model.get("kind") or "")
        if isinstance(access_model, Mapping)
        else ""
    )
    direct_read_model = access_kind == "command_then_direct_read"

    for flow_index, flow in selected_flows:
        phase = _flow_phase(flow)
        flow_id = str(flow.get("flow_id") or f"flow_{flow_index}")
        steps = flow.get("steps")
        if not isinstance(steps, list):
            continue
        step_index = 0
        while step_index < len(steps):
            step = steps[step_index]
            if not isinstance(step, Mapping):
                step_index += 1
                continue
            op = str(step.get("op") or "").strip()
            transaction = step.get("transaction")
            source = f"operation_flows[{flow_index}].steps[{step_index}]"
            role = str(step.get("role") or op or "flow_step")
            step_addr_or_pin = _infer_i2c_addr_for_flow_step(
                device_ir,
                default_addr=addr_or_pin,
                flow=flow,
                step=step,
            )
            if isinstance(transaction, Mapping):
                kind = str(transaction.get("kind") or "")
                if kind == "write":
                    prefix, next_step_index = _expected_write_prefix_from_flow_steps(
                        steps,
                        step_index,
                        access_model=access_model,
                    )
                    if prefix:
                        # Broadcast transactions may carry the address byte.
                        actual_addr = step_addr_or_pin
                        actual_prefix = prefix
                        if prefix and prefix[0] == "0x00" and len(prefix) >= 2:
                            flow_id_lower = flow_id.lower()
                            if ("general_call" in flow_id_lower
                                    or "broadcast" in flow_id_lower
                                    or "general_call" in str(flow.get("notes", "")).lower()
                                    or "general call" in str(transaction.get("notes", "")).lower()
                                    or "general call" in str(step.get("notes", "")).lower()):
                                actual_addr = "0x00"
                                actual_prefix = prefix[1:]  # strip address byte
                        append(ExpectedTransaction(
                            phase=phase,
                            addr_or_pin=actual_addr,
                            write_prefix_any_of=(actual_prefix,),
                            read_any=False,
                            note=f"{flow_id}: {_joined_step_roles(steps, step_index, next_step_index)}",
                            source=source,
                        ))
                    step_index = next_step_index
                    continue
                elif kind == "write_then_read":
                    prefix = _literal_prefix_from_transaction(transaction)
                    append(ExpectedTransaction(
                        phase=phase,
                        addr_or_pin=step_addr_or_pin,
                        write_prefix_any_of=(prefix,) if prefix else tuple(),
                        read_any=True,
                        note=f"{flow_id}: {role}",
                        source=source,
                    ))
                elif kind == "read":
                    append(ExpectedTransaction(
                        phase=phase,
                        addr_or_pin=step_addr_or_pin,
                        write_prefix_any_of=tuple(),
                        read_any=True,
                        note=f"{flow_id}: direct read {role}",
                        source=source,
                        forbid_write_prefix=direct_read_model,
                    ))
                step_index += 1
                continue

            # Register-only flow steps still provide useful prefixes.
            reg = _canonical_hex_byte(step.get("register"))
            if op in {"poll_until", "clear", "select_page"} and reg is not None:
                append(ExpectedTransaction(
                    phase=phase,
                    addr_or_pin=step_addr_or_pin,
                    write_prefix_any_of=((reg,),),
                    read_any=(op == "poll_until"),
                    note=f"{flow_id}: {role}",
                    source=source,
                ))
            step_index += 1

    return out


_FLOW_SOURCE_RE = re.compile(r"operation_flows\[(\d+)\]\.steps\[(\d+)\]")


def _register_address_lookup(device_ir: Mapping[str, Any]) -> Dict[str, int]:
    out: Dict[str, int] = {}
    registers = device_ir.get("registers_or_commands")
    if not isinstance(registers, list):
        return out
    for entry in registers:
        if not isinstance(entry, Mapping):
            continue
        op = _extract_opcode(entry)
        if op is None:
            continue
        try:
            value = int(op, 0)
        except ValueError:
            continue
        names = [
            entry.get("name"),
            entry.get("id"),
            entry.get("symbol"),
            entry.get("register"),
        ]
        for raw_name in names:
            key = _normalise_flow_token(str(raw_name or ""))
            if key:
                out[key] = value
    return out


def _flow_step_from_source(
    device_ir: Mapping[str, Any],
    source: str,
) -> Optional[Mapping[str, Any]]:
    match = _FLOW_SOURCE_RE.fullmatch(str(source or ""))
    if not match:
        return None
    flow_index = int(match.group(1))
    step_index = int(match.group(2))
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list) or flow_index >= len(flows):
        return None
    flow = flows[flow_index]
    if not isinstance(flow, Mapping):
        return None
    steps = flow.get("steps")
    if not isinstance(steps, list) or step_index >= len(steps):
        return None
    step = steps[step_index]
    return step if isinstance(step, Mapping) else None


def _spi_register_addr_for_step(
    device_ir: Mapping[str, Any],
    step: Mapping[str, Any],
    *,
    register_lookup: Mapping[str, int],
    protocol: SpiProtocolHints,
) -> Optional[int]:
    transaction = step.get("transaction")
    if isinstance(transaction, Mapping):
        pointer = _normalise_flow_token(str(transaction.get("pointer_target") or ""))
        if pointer and pointer in register_lookup:
            return int(register_lookup[pointer])
        register = _normalise_flow_token(str(transaction.get("register") or ""))
        if register and register in register_lookup:
            return int(register_lookup[register])
        prefix = _literal_prefix_from_transaction(transaction)
        if prefix:
            try:
                return int(prefix[0], 0) & protocol.addr_mask
            except ValueError:
                return None
    register_name = _normalise_flow_token(str(step.get("register") or ""))
    if register_name and register_name in register_lookup:
        return int(register_lookup[register_name])
    return None


def _spi_flow_tx_length(step: Mapping[str, Any]) -> int:
    transaction = step.get("transaction")
    if not isinstance(transaction, Mapping):
        return 1
    raw_length = transaction.get("length")
    if isinstance(raw_length, int) and raw_length > 0:
        return raw_length
    raw_bytes = transaction.get("bytes")
    if isinstance(raw_bytes, list):
        non_command_bytes = max(0, len(raw_bytes) - 1)
        if non_command_bytes > 0:
            return non_command_bytes
    return 1


def _format_hex_byte(value: int) -> str:
    return f"0x{int(value) & 0xFF:02X}"


def _normalise_spi_register_flow_transactions(
    txs: Sequence[ExpectedTransaction],
    device_ir: Mapping[str, Any],
    *,
    task_package: Optional[Mapping[str, Any]],
) -> List[ExpectedTransaction]:
    protocol = spi_protocol_hints(
        device_ir=device_ir,
        task_package=task_package,
        default_proto="register",
    )
    if not protocol.is_register or not protocol.explicit:
        return list(txs)

    register_lookup = _register_address_lookup(device_ir)
    out: List[ExpectedTransaction] = []
    for tx in txs:
        step = _flow_step_from_source(device_ir, tx.source)
        if step is None:
            out.append(tx)
            continue
        transaction = step.get("transaction")
        kind = str(transaction.get("kind") or "") if isinstance(transaction, Mapping) else ""
        if kind not in {"write", "write_then_read"}:
            out.append(tx)
            continue
        register_addr = _spi_register_addr_for_step(
            device_ir,
            step,
            register_lookup=register_lookup,
            protocol=protocol,
        )
        if register_addr is None:
            out.append(tx)
            continue
        if kind == "write_then_read":
            command = protocol.read_command(
                register_addr,
                length=_spi_flow_tx_length(step),
            )
            out.append(dataclasses.replace(
                tx,
                write_prefix_any_of=((_format_hex_byte(command),),),
            ))
            continue
        command = protocol.write_command(register_addr)
        original = tx.write_prefix_any_of[0] if tx.write_prefix_any_of else tuple()
        suffix = original[1:] if len(original) > 1 else tuple()
        out.append(dataclasses.replace(
            tx,
            write_prefix_any_of=((_format_hex_byte(command), *suffix),),
        ))
    return out


def _expected_write_prefix_from_flow_steps(
    steps: Sequence[Any],
    start_index: int,
    *,
    access_model: Any,
) -> Tuple[Tuple[str, ...], int]:
    """Build the expected literal prefix for one executable bus write."""

    if start_index >= len(steps):
        return tuple(), start_index + 1
    first_step = steps[start_index]
    if not isinstance(first_step, Mapping):
        return tuple(), start_index + 1
    first_tx = first_step.get("transaction")
    if not isinstance(first_tx, Mapping) or str(first_tx.get("kind") or "") != "write":
        return tuple(), start_index + 1
    if _is_runtime_payload_write_step(first_step):
        return tuple(), start_index + 1

    prefix = list(_literal_prefix_from_transaction(first_tx))
    if not prefix:
        return tuple(), start_index + 1

    address_bytes = _address_byte_count(access_model)
    if address_bytes <= len(prefix):
        return tuple(prefix), start_index + 1
    if not _looks_like_address_or_pointer_step(first_step):
        return tuple(prefix), start_index + 1

    next_index = start_index + 1
    while next_index < len(steps) and len(prefix) < address_bytes:
        next_step = steps[next_index]
        if not isinstance(next_step, Mapping):
            break
        next_tx = next_step.get("transaction")
        if not isinstance(next_tx, Mapping) or str(next_tx.get("kind") or "") != "write":
            break
        if _is_runtime_payload_write_step(next_step):
            break
        if not _looks_like_address_or_pointer_step(next_step):
            break
        next_prefix = _literal_prefix_from_transaction(next_tx)
        if not next_prefix:
            break
        prefix.extend(next_prefix)
        next_index += 1
    return tuple(prefix), next_index


def _address_byte_count(access_model: Any) -> int:
    if not isinstance(access_model, Mapping):
        return 0
    kind = str(access_model.get("kind") or "").strip().lower()
    raw_count = access_model.get("address_bytes")
    if not isinstance(raw_count, int) or raw_count <= 1:
        return 0
    if kind in {
        "memory",
        "register_pointer",
        "register_auto_increment",
        "packet",
    }:
        return raw_count
    return 0


def _looks_like_address_or_pointer_step(step: Mapping[str, Any]) -> bool:
    text = _normalise_flow_token(" ".join(_walk_flow_strings({
        "role": step.get("role"),
        "register": step.get("register"),
        "notes": step.get("notes"),
        "transaction": step.get("transaction"),
    })))
    return any(
        token in text
        for token in (
            "address",
            "addr",
            "pointer",
            "offset",
            "index",
            "word_address",
            "memory_address",
            "register_address",
            "high",
            "low",
            "msb",
            "lsb",
        )
    )


def _is_runtime_payload_write_step(step: Mapping[str, Any]) -> bool:
    transaction = step.get("transaction")
    tx_bytes = transaction.get("bytes") if isinstance(transaction, Mapping) else None
    if isinstance(tx_bytes, list) and any(item == "DATA" or item is None for item in tx_bytes):
        return not bool(_literal_prefix_from_transaction(transaction))
    if isinstance(transaction, Mapping):
        prefix = _literal_prefix_from_transaction(transaction)
        if prefix and any(
            transaction.get(key)
            for key in (
                "pointer_target",
                "register",
                "address",
                "register_address",
                "pointer",
            )
        ):
            return False
    text = _normalise_flow_token(" ".join(_walk_flow_strings({
        "role": step.get("role"),
        "notes": step.get("notes"),
        "transaction": transaction,
    })))
    if not text:
        return False
    if any(token in text for token in ("pointer", "address", "addr", "register_address")):
        return False
    return any(
        token in text
        for token in (
            "data",
            "payload",
            "frame",
            "sample",
            "value_to_write",
            "byte_to_write",
            "runtime",
            "variable",
        )
    )


def _joined_step_roles(
    steps: Sequence[Any],
    start_index: int,
    stop_index: int,
) -> str:
    roles: List[str] = []
    for step in steps[start_index:stop_index]:
        if not isinstance(step, Mapping):
            continue
        role = str(step.get("role") or step.get("op") or "write").strip()
        if role and role not in roles:
            roles.append(role)
    return " + ".join(roles) if roles else "write"


def _select_codegen_operation_flows(
    device_ir: Mapping[str, Any],
    flows: Sequence[Any],
    *,
    eval_class: str = "",
) -> List[Tuple[int, Mapping[str, Any]]]:
    """Pick executable core flows for codegen/test expectations."""

    indexed = [
        (idx, flow)
        for idx, flow in enumerate(flows)
        if isinstance(flow, Mapping)
    ]
    if not indexed:
        return []

    by_id = {
        _normalise_flow_token(str(flow.get("flow_id") or "")): (idx, flow)
        for idx, flow in indexed
        if _normalise_flow_token(str(flow.get("flow_id") or ""))
    }
    eval_class_str = str(eval_class or "")
    target_channels = _declared_read_channel_ids(device_ir, eval_class=eval_class_str)
    target_flow_ids = _declared_read_channel_flow_ids(device_ir, eval_class=eval_class_str)
    selected: set[int] = set()

    for idx, flow in indexed:
        flow_id = _normalise_flow_token(str(flow.get("flow_id") or ""))
        if eval_class_str == EVAL_CLASS_DISPLAY and _is_display_readback_flow(flow):
            continue
        if flow_id and flow_id in target_flow_ids:
            selected.add(idx)

    if not selected:
        _select_best_read_flows_by_channel(indexed, target_channels, selected)

    if not selected and str(eval_class or "") != EVAL_CLASS_DISPLAY:
        _select_best_fallback_read_flow(indexed, selected)

    for idx, flow in indexed:
        if _is_required_setup_or_identity_flow(flow):
            selected.add(idx)

    if _eval_class_exposes_public_write_surface(eval_class):
        for idx, flow in indexed:
            if eval_class_str == EVAL_CLASS_RTC:
                if _is_rtc_public_time_write_surface(flow):
                    selected.add(idx)
            elif eval_class_str == EVAL_CLASS_DISPLAY:
                if _is_display_public_write_surface(flow):
                    selected.add(idx)
            elif _is_public_write_surface(flow):
                selected.add(idx)

    _add_referenced_flow_dependencies(indexed, by_id, selected)
    if eval_class_str == EVAL_CLASS_DISPLAY:
        selected = {
            idx
            for idx, flow in indexed
            if idx in selected and not _is_display_readback_flow(flow)
        }

    if not selected:
        return indexed
    return [(idx, flow) for idx, flow in indexed if idx in selected]


def _eval_class_exposes_public_write_surface(eval_class: str) -> bool:
    """Return True when codegen should require standalone write flows."""

    return str(eval_class or "") in {
        EVAL_CLASS_MEMORY,
        EVAL_CLASS_DISPLAY,
        EVAL_CLASS_RTC,
    }


def _is_required_setup_or_identity_flow(flow: Mapping[str, Any]) -> bool:
    """Return True for setup/probe flows that the driver should perform."""
    kind = str(flow.get("kind") or "").strip().lower()
    if kind == "init":
        return _flow_has_executable_bus_transaction(flow)
    if kind != "probe":
        return False
    text = _flow_text(flow)
    if _looks_optional_for_codegen(text):
        return False
    identity_tokens = (
        "who_am_i",
        "whoami",
        "chip_id",
        "device_id",
        "device_identity",
        "devid",
        "product_id",
        "part_id",
        "id_register",
    )
    return (
        _flow_has_executable_bus_transaction(flow)
        and any(token in text for token in identity_tokens)
    )


def _flow_has_executable_bus_transaction(flow: Mapping[str, Any]) -> bool:
    steps = flow.get("steps")
    if not isinstance(steps, list):
        return False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        tx = step.get("transaction")
        if isinstance(tx, Mapping) and str(tx.get("kind") or "") in {
            "write",
            "write_then_read",
            "read",
        }:
            return True
        if _canonical_hex_byte(step.get("register")) is not None:
            return True
    return False


def _select_best_read_flows_by_channel(
    indexed: Sequence[Tuple[int, Mapping[str, Any]]],
    target_channels: set[str],
    selected: set[int],
) -> None:
    if not target_channels:
        return
    for channel in sorted(target_channels):
        best_idx: Optional[int] = None
        best_score = 0
        for idx, flow in indexed:
            if str(flow.get("kind") or "").strip().lower() != "read":
                continue
            if channel not in _flow_channel_ids(flow):
                continue
            score = _read_flow_codegen_score(flow, target_channels)
            if score > best_score:
                best_idx = idx
                best_score = score
        if best_idx is not None and best_score > 0:
            selected.add(best_idx)


def _select_best_fallback_read_flow(
    indexed: Sequence[Tuple[int, Mapping[str, Any]]],
    selected: set[int],
) -> None:
    best_idx: Optional[int] = None
    best_score = 0
    for idx, flow in indexed:
        if str(flow.get("kind") or "").strip().lower() != "read":
            continue
        score = _read_flow_codegen_score(flow, set())
        if score > best_score:
            best_idx = idx
            best_score = score
    if best_idx is not None and best_score > 0:
        selected.add(best_idx)


def _add_referenced_flow_dependencies(
    indexed: Sequence[Tuple[int, Mapping[str, Any]]],
    by_id: Mapping[str, Tuple[int, Mapping[str, Any]]],
    selected: set[int],
) -> None:
    changed = True
    while changed:
        changed = False
        for idx, flow in indexed:
            if idx not in selected:
                continue
            preconditions = flow.get("preconditions")
            if not isinstance(preconditions, list):
                continue
            dependency_text = _normalise_flow_token(" ".join(
                str(item or "") for item in preconditions
            ))
            if not dependency_text:
                continue
            for dep_id, (dep_idx, _dep_flow) in by_id.items():
                if dep_idx in selected:
                    continue
                if _contains_flow_token(dependency_text, dep_id):
                    selected.add(dep_idx)
                    changed = True


def _declared_read_channel_ids(
    device_ir: Mapping[str, Any],
    *,
    eval_class: str = "",
) -> set[str]:
    out: set[str] = set()
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = _normalise_flow_token(str(channel.get("id") or ""))
        if not _read_channel_exposed_by_codegen_eval(channel_id, channel, eval_class=eval_class):
            continue
        if channel_id:
            out.add(channel_id)
    return out


def _declared_read_channel_flow_ids(
    device_ir: Mapping[str, Any],
    *,
    eval_class: str = "",
) -> set[str]:
    out: set[str] = set()
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = _normalise_flow_token(str(channel.get("id") or ""))
        if not _read_channel_exposed_by_codegen_eval(channel_id, channel, eval_class=eval_class):
            continue
        channel_text = _normalise_flow_token(" ".join(_walk_flow_strings(channel)))
        sources = channel.get("source_bytes")
        if (
            not isinstance(sources, list)
            and not channel.get("formula_id")
            and _looks_optional_for_codegen(channel_text)
        ):
            continue
        flow_id = _normalise_flow_token(str(channel.get("flow_id") or ""))
        if flow_id and (
            flow_id.startswith("set_")
            or flow_id.startswith("write_")
            or _looks_optional_for_codegen(flow_id)
        ):
            continue
        if flow_id:
            out.add(flow_id)
    return out


def _read_channel_exposed_by_codegen_eval(
    channel_id: str,
    channel: Mapping[str, Any],
    *,
    eval_class: str = "",
) -> bool:
    eval_class_str = str(eval_class or "")
    if eval_class_str == EVAL_CLASS_RTC:
        return _is_rtc_time_channel_id(channel_id)
    if eval_class_str == EVAL_CLASS_DISPLAY:
        flow_id = _normalise_flow_token(str(channel.get("flow_id") or ""))
        channel_text = _normalise_flow_token(" ".join(_walk_flow_strings(channel)))
        if _looks_like_status_or_diagnostic(channel_text):
            return True
        if flow_id and _looks_like_status_or_diagnostic(flow_id):
            return True
        return not _is_display_frame_channel_id(channel_id)
    return True


def _flow_channel_ids(flow: Mapping[str, Any]) -> set[str]:
    out: set[str] = set()
    channels = flow.get("channels")
    if isinstance(channels, list):
        for channel in channels:
            channel_id = _normalise_flow_token(str(channel or ""))
            if channel_id:
                out.add(channel_id)
    outputs = flow.get("outputs")
    if isinstance(outputs, list):
        for output in outputs:
            if not isinstance(output, Mapping):
                continue
            channel_id = _normalise_flow_token(str(output.get("channel") or ""))
            if channel_id:
                out.add(channel_id)
    return out


def _read_flow_codegen_score(
    flow: Mapping[str, Any],
    target_channels: set[str],
) -> int:
    score = 10
    covered = _flow_channel_ids(flow)
    outputs = flow.get("outputs")
    output_count = len(outputs) if isinstance(outputs, list) else 0
    if output_count:
        score += 30 + output_count * 5
    if target_channels:
        score += len(covered & target_channels) * 25
        if target_channels <= covered:
            score += 25
    steps = flow.get("steps")
    if isinstance(steps, list):
        if any(_step_has_read_transaction(step) for step in steps):
            score += 20
        if any(_step_has_completion_wait(step) for step in steps):
            score += 5
    text = _flow_text(flow)
    if _looks_optional_for_codegen(text):
        score -= 50
    if _looks_like_status_or_diagnostic(text):
        score -= 60
    if "single" in text or "default" in text:
        score += 5
    return max(score, 0)


def _step_has_read_transaction(step: Any) -> bool:
    if not isinstance(step, Mapping):
        return False
    if str(step.get("op") or "") in {"read", "write_then_read", "sample_signal", "measure_pulse"}:
        return True
    tx = step.get("transaction")
    return isinstance(tx, Mapping) and str(tx.get("kind") or "") in {"read", "write_then_read"}


def _step_has_completion_wait(step: Any) -> bool:
    if not isinstance(step, Mapping):
        return False
    return str(step.get("op") or "") in {
        "delay",
        "poll_until",
        "wait_until_ready",
        "wait_signal",
    }


def _is_public_write_surface(flow: Mapping[str, Any]) -> bool:
    if str(flow.get("kind") or "").strip().lower() != "write":
        return False
    if _flow_channel_ids(flow):
        return False
    text = _flow_text(flow)
    if _looks_optional_for_codegen(text) or _looks_like_status_or_diagnostic(text):
        return False
    return any(
        token in text
        for token in (
            "byte_write",
            "page_write",
            "block_write",
            "word_write",
            "write_cycle",
            "program",
            "erase",
            "store",
            "memory",
            "frame",
            "display",
            "command",
            "payload",
            "data",
        )
    )


def _is_display_public_write_surface(flow: Mapping[str, Any]) -> bool:
    if str(flow.get("kind") or "").strip().lower() != "write":
        return False
    text = _flow_text(flow)
    if _looks_optional_for_codegen(text) or _looks_like_status_or_diagnostic(text):
        return False
    channels = _flow_channel_ids(flow)
    if channels and not any(_is_display_frame_channel_id(channel) for channel in channels):
        return False
    return any(
        token in text
        for token in (
            "write_display",
            "write_frame",
            "display_data",
            "frame_data",
            "framebuffer",
            "frame_buffer",
            "gddram",
            "pixel",
            "display",
            "frame",
            "payload",
            "data",
        )
    )


def _is_rtc_public_time_write_surface(flow: Mapping[str, Any]) -> bool:
    if str(flow.get("kind") or "").strip().lower() != "write":
        return False
    text = _flow_text(flow)
    if _looks_optional_for_codegen(text) or _looks_like_status_or_diagnostic(text):
        return False
    channels = _flow_channel_ids(flow)
    time_channels = {
        "seconds", "second", "minutes", "minute", "hours", "hour",
        "day", "date", "month", "year", "weekday",
    }
    return (
        bool(channels & time_channels)
        or "write_time" in text
        or "set_time" in text
        or "time_register" in text
    )


def _is_display_readback_flow(flow: Mapping[str, Any]) -> bool:
    if str(flow.get("kind") or "").strip().lower() != "read":
        return False
    text = _flow_text(flow)
    if _looks_like_status_or_diagnostic(text):
        return False
    channels = _flow_channel_ids(flow)
    if any(_is_display_frame_channel_id(channel) for channel in channels):
        return True
    return any(
        token in text
        for token in (
            "read_display",
            "read_frame",
            "display_read",
            "frame_read",
            "read_gddram",
            "gddram_read",
            "display_memory",
            "framebuffer",
            "frame_buffer",
            "pixel_data",
        )
    )


def _is_display_frame_channel_id(channel_id: str) -> bool:
    channel = _normalise_flow_token(channel_id)
    if not channel:
        return False
    if _looks_like_status_or_diagnostic(channel):
        return False
    return any(
        token in channel
        for token in (
            "display",
            "frame",
            "framebuffer",
            "gddram",
            "gram",
            "pixel",
            "image",
            "buffer",
        )
    )


def _is_rtc_time_channel_id(channel_id: str) -> bool:
    channel = _normalise_flow_token(channel_id)
    if not channel:
        return False
    if _looks_optional_for_codegen(channel) or _looks_like_status_or_diagnostic(channel):
        return False
    time_tokens = {
        "seconds",
        "second",
        "sec",
        "minutes",
        "minute",
        "min",
        "hours",
        "hour",
        "hr",
        "day",
        "date",
        "month",
        "year",
        "weekday",
        "week_day",
        "day_of_week",
        "century",
    }
    return any(_contains_flow_token(channel, token) for token in time_tokens)


def _looks_optional_for_codegen(text: str) -> bool:
    return any(
        token in text
        for token in (
            "optional",
            "alarm",
            "scroll",
            "scrolling",
            "diagnostic",
            "plausibility",
            "recovery",
            "communication_lost",
            "interface_reset",
            "general_call",
            "heater",
            "break_command",
            "not_selected",
            "disabled",
        )
    )


def _looks_like_status_or_diagnostic(text: str) -> bool:
    return any(
        token in text
        for token in (
            "status_register",
            "read_status",
            "clear_status",
            "interrupt_status",
            "fault_status",
            "who_am_i",
            "chip_id",
            "manufacturer_id",
        )
    )


def _flow_text(flow: Mapping[str, Any]) -> str:
    return _normalise_flow_token(" ".join(_walk_flow_strings(flow)))


def _walk_flow_strings(value: Any):
    if isinstance(value, str):
        yield value
    elif isinstance(value, Mapping):
        for item in value.values():
            yield from _walk_flow_strings(item)
    elif isinstance(value, list):
        for item in value:
            yield from _walk_flow_strings(item)


def _normalise_flow_token(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", str(value or "").lower()).strip("_")


def _contains_flow_token(text: str, token: str) -> bool:
    return f"_{token}_" in f"_{text}_"


# I2C derivation

# Routing keywords are intentionally small and favor unambiguous signals.
_INIT_NAME_TOKENS: Tuple[str, ...] = (
    "power", "wake", "standby", "enable", "disable",
    "reset", "rst", "init", "ctrl", "control",
    "config", "cfg", "setup", "calibration", "calib",
    "id", "whoami", "who_am_i", "chip_id",
)
_READ_NAME_TOKENS: Tuple[str, ...] = (
    "measure", "convert", "sample", "acquire", "fetch",
    "output", "mode", "data", "_out", "out_", "trigger",
    "temp", "press", "humidity", "hum", "accel", "gyro",
    "mag", "lux", "light", "range", "distance",
)
_WRITE_NAME_TOKENS: Tuple[str, ...] = (
    "store", "program", "erase", "write",
    "set_time", "set_date", "set_alarm",
)


def _classify_opcode_phase(
    entry: Mapping[str, Any],
    init_text: str,
    read_text: str,
) -> str:
    """Decide init / read_cycle / write_cycle for one register entry."""
    op = _extract_opcode(entry)
    if op is not None:
        init_lower = init_text.lower()
        read_lower = read_text.lower()
        # Compare canonical and no-leading-zero hex forms.
        full = op.lower()
        stripped = full
        # Keep the value equivalent while removing redundant leading zeroes.
        if full.startswith("0x"):
            body = full[2:].lstrip("0") or "0"
            stripped = "0x" + body
        variants = {full, stripped}
        for phase_key, t in (("init", init_lower),
                              ("read_cycle", read_lower)):
            for v in variants:
                if _hex_word_in_text(v, t):
                    return phase_key

    name_only = str(entry.get("name", "")).lower()
    name_plus_desc = " ".join(
        str(entry.get(k, "")) for k in ("name", "description", "kind")
    ).lower()

    # Read tokens may appear in either name or description.
    if _name_matches_tokens(name_plus_desc, _READ_NAME_TOKENS):
        return "read_cycle"
    # Init tokens are name-only to avoid prose false positives.
    if _name_matches_tokens(name_only, _INIT_NAME_TOKENS):
        return "init"
    if _name_matches_tokens(name_plus_desc, _WRITE_NAME_TOKENS):
        return "write_cycle"
    return "read_cycle"


def _derive_i2c(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> List[ExpectedTransaction]:
    addr = _primary_i2c_address(device_ir)
    if addr is None:
        # Missing addresses produce an empty structured result.
        return []

    flow_txs = _operation_flow_transactions(
        device_ir,
        addr_or_pin=addr,
        eval_class=classify_result.eval_class,
    )
    if flow_txs:
        return flow_txs

    init_text = _phase_text_bag(device_ir, "init_sequence")
    read_text = _phase_text_bag(device_ir, "read_sequence")

    # Group opcodes so equivalent read-cycle commands become alternatives.
    buckets: Dict[str, List[Tuple[str, int, Mapping[str, Any]]]] = {
        "init": [], "read_cycle": [], "write_cycle": [],
    }
    for idx, entry in enumerate(device_ir.get("registers_or_commands") or []):
        if not isinstance(entry, Mapping):
            continue
        op = _extract_opcode(entry)
        if op is None:
            continue
        phase = _classify_opcode_phase(entry, init_text, read_text)
        buckets[phase].append((op, idx, entry))

    out: List[ExpectedTransaction] = []

    # Init phase: each opcode is independently required.
    for op, idx, entry in buckets["init"]:
        out.append(ExpectedTransaction(
            phase="init",
            addr_or_pin=addr,
            write_prefix_any_of=((op,),),
            read_any=False,
            note=f"init: {entry.get('name', '<unnamed>')}",
            source=f"registers_or_commands[{idx}]",
        ))

    # Read cycle: alternative measurement commands collapse into one row.
    if buckets["read_cycle"]:
        variants = tuple((op,) for (op, _idx, _entry) in buckets["read_cycle"])
        sources = ", ".join(
            f"registers_or_commands[{idx}]"
            for (_op, idx, _entry) in buckets["read_cycle"]
        )
        names = sorted({
            str(entry.get("name", ""))
            for (_op, _idx, entry) in buckets["read_cycle"]
            if entry.get("name")
        })
        note = "read_cycle: one of " + ", ".join(names) if names else "read_cycle opcode"
        out.append(ExpectedTransaction(
            phase="read_cycle",
            addr_or_pin=addr,
            write_prefix_any_of=variants,
            read_any=True,
            note=note,
            source=sources or "read_sequence",
        ))
    else:
        # A read sequence without an opcode still requires a data read.
        if device_ir.get("read_sequence"):
            out.append(ExpectedTransaction(
                phase="read_cycle",
                addr_or_pin=addr,
                write_prefix_any_of=tuple(),
                read_any=True,
                note="read_cycle: read-only data fetch (no write prefix)",
                source="read_sequence",
            ))

    # Write cycle: keep each opcode separate like init.
    for op, idx, entry in buckets["write_cycle"]:
        out.append(ExpectedTransaction(
            phase="write_cycle",
            addr_or_pin=addr,
            write_prefix_any_of=((op,),),
            read_any=False,
            note=f"write_cycle: {entry.get('name', '<unnamed>')}",
            source=f"registers_or_commands[{idx}]",
        ))

    # Eval-class-specific fallbacks.
    if classify_result.eval_class == EVAL_CLASS_MEMORY and not out:
        # Flat memories may expose no named register opcodes.
        out.append(ExpectedTransaction(
            phase="read_cycle", addr_or_pin=addr,
            write_prefix_any_of=tuple(), read_any=True,
            note="memory: word-address followed by N byte read",
            source="eval_class=memory",
        ))
        out.append(ExpectedTransaction(
            phase="write_cycle", addr_or_pin=addr,
            write_prefix_any_of=tuple(), read_any=False,
            note="memory: word-address followed by N byte write",
            source="eval_class=memory",
        ))

    return out


# SPI / UART / GPIO derivation

def _derive_spi(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
    task_package: Optional[Mapping[str, Any]] = None,
) -> List[ExpectedTransaction]:
    # SPI doesn't have bus addresses in the I2C sense; we use a fixed
    # "spi1" pin label so downstream diffing is keyed correctly.
    pin = "spi1"
    flow_txs = _operation_flow_transactions(
        device_ir,
        addr_or_pin=pin,
        eval_class=classify_result.eval_class,
    )
    if flow_txs:
        return _normalise_spi_register_flow_transactions(
            flow_txs,
            device_ir,
            task_package=task_package,
        )

    init_text = _phase_text_bag(device_ir, "init_sequence")
    read_text = _phase_text_bag(device_ir, "read_sequence")

    # Treat each known opcode/register address as a possible first byte.
    out: List[ExpectedTransaction] = []
    seen_ops: set = set()
    for idx, entry in enumerate(device_ir.get("registers_or_commands") or []):
        if not isinstance(entry, Mapping):
            continue
        op = _extract_opcode(entry)
        if op is None or op in seen_ops:
            continue
        seen_ops.add(op)
        phase = _classify_opcode_phase(entry, init_text, read_text)
        out.append(ExpectedTransaction(
            phase=phase, addr_or_pin=pin,
            write_prefix_any_of=((op,),),
            read_any=(phase == "read_cycle"),
            note=f"{phase}: {entry.get('name', '<unnamed>')}",
            source=f"registers_or_commands[{idx}]",
        ))

    # Stream-mode SPI still requires at least one data read.
    if not out and device_ir.get("read_sequence"):
        out.append(ExpectedTransaction(
            phase="read_cycle", addr_or_pin=pin,
            write_prefix_any_of=tuple(), read_any=True,
            note="spi stream: clock out N bytes on CS",
            source="read_sequence",
        ))

    return out


def _derive_uart(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> List[ExpectedTransaction]:
    flow_txs = _operation_flow_transactions(
        device_ir,
        addr_or_pin="uart1",
        eval_class=classify_result.eval_class,
    )
    if flow_txs:
        return flow_txs

    out: List[ExpectedTransaction] = []
    for idx, entry in enumerate(device_ir.get("registers_or_commands") or []):
        if not isinstance(entry, Mapping):
            continue
        # UART commands often have cmd16 (2-byte) form.
        op = _extract_opcode(entry)
        if op is None:
            continue
        out.append(ExpectedTransaction(
            phase="write_cycle", addr_or_pin="uart1",
            write_prefix_any_of=((op,),),
            read_any=False,
            note=f"uart command: {entry.get('name', '<unnamed>')}",
            source=f"registers_or_commands[{idx}]",
        ))
    if not out and device_ir.get("read_sequence"):
        out.append(ExpectedTransaction(
            phase="read_cycle", addr_or_pin="uart1",
            write_prefix_any_of=tuple(), read_any=True,
            note="uart: driver exchanges packets with sensor",
            source="read_sequence",
        ))
    return out


def _derive_gpio(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
) -> List[ExpectedTransaction]:
    # GPIO probes use pin-level markers rather than byte prefixes.
    pin = "gpio1"
    flow_txs = _operation_flow_transactions(
        device_ir,
        addr_or_pin=pin,
        eval_class=classify_result.eval_class,
    )
    if flow_txs:
        return flow_txs

    out: List[ExpectedTransaction] = [
        ExpectedTransaction(
            phase="init", addr_or_pin=pin,
            write_prefix_any_of=tuple(),
            read_any=False,
            note="init: configure pin mode (output then input)",
            source="eval_class=single_channel" if classify_result.eval_class
                   == EVAL_CLASS_SINGLE_CHANNEL else "eval_class=multi_channel",
        ),
        ExpectedTransaction(
            phase="read_cycle", addr_or_pin=pin,
            write_prefix_any_of=tuple(),
            read_any=True,
            note="read_cycle: pulse / 1-wire handshake then sample bits",
            source="read_sequence",
        ),
    ]
    return out


# Public entry

def derive_expected_transactions(
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
    task_package: Optional[Mapping[str, Any]] = None,
) -> List[ExpectedTransaction]:
    """Return derived expected transactions for a device."""
    if not isinstance(device_ir, Mapping):
        return []

    bus = classify_result.bus_type
    if bus in {"i2c", "smbus"}:
        return _derive_i2c(device_ir, classify_result)
    if bus == "spi":
        return _derive_spi(device_ir, classify_result, task_package=task_package)
    if bus == "uart":
        return _derive_uart(device_ir, classify_result)
    if bus == "gpio":
        return _derive_gpio(device_ir, classify_result)
    return []


__all__ = [
    "ExpectedTransaction",
    "derive_expected_transactions",
]
