"""evaluation.ladder.l3_protocol._i2c_compare - private helpers for I2C L3."""
from __future__ import annotations

from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple, Union

from evaluation.models import I2CTrace, I2CTransaction, LevelVerdict

# A required-write entry. Two accepted shapes:
#
#   (addr, prefix)                           — single prefix tuple form
#   {"addr": addr, "any_of": [prefix, ...],  — dict form with alternatives
#    "description": "..."}                     (description optional)
RequiredWriteLegacy = Union[
    Tuple[int, Sequence[int]],
    Dict[str, Any],
]


ProtocolPolicy = str  # "strict" | "semantic" | "relaxed"


# ---------- normalisation helpers ----------

def _normalize_required(entry: RequiredWriteLegacy) -> Tuple[int, List[List[int]], str]:
    """Return ``(addr, list_of_prefixes, description)``."""
    if isinstance(entry, dict):
        addr = int(entry["addr"], 0) if isinstance(entry["addr"], str) else int(entry["addr"])
        alts = entry.get("any_of")
        if not alts:
            # allow a single-prefix dict form
            if "prefix" in entry:
                alts = [entry["prefix"]]
            else:
                raise ValueError(f"required_writes entry missing any_of/prefix: {entry!r}")
        norm_alts = [
            [(int(b, 0) if isinstance(b, str) else int(b)) & 0xFF for b in alt]
            for alt in alts
        ]
        return addr, norm_alts, str(entry.get("description", ""))
    addr, prefix = entry
    return int(addr) & 0xFF, [[int(b) & 0xFF for b in prefix]], ""


def _txn_signature(t: I2CTransaction) -> Tuple[
    Optional[int], Optional[bool], Optional[int], int, int,
]:
    """Reduce a transaction to ``(addr, is_read, reg_ptr_or_None, tx_len, rx_len)``."""
    reg = t.tx_bytes[0] if t.tx_bytes else None
    return (t.addr, t.is_read, reg, len(t.tx_bytes), len(t.rx_bytes))


def _normalize_combined_txns(trace: I2CTrace) -> list:
    """Split combined write+read transactions (repeated-START pattern) into separate write and read transactions for semantic comparison."""
    normalized: list = []
    seq_counter = 0
    for t in trace.transactions:
        if t.is_read and t.tx_bytes and t.rx_bytes:
            # Combined write+read: split into WRITE(reg_ptr) + READ(data)
            normalized.append(I2CTransaction(
                seq=seq_counter, addr=t.addr, is_read=False,
                tx_bytes=list(t.tx_bytes), rx_bytes=[]))
            seq_counter += 1
            normalized.append(I2CTransaction(
                seq=seq_counter, addr=t.addr, is_read=True,
                tx_bytes=[], rx_bytes=list(t.rx_bytes)))
            seq_counter += 1
        else:
            normalized.append(t)
            seq_counter += 1
    return normalized


def _iter_required_present(
    gen: I2CTrace,
    required: Iterable[RequiredWriteLegacy],
) -> List[Tuple[Dict[str, Any], bool]]:
    """For each required entry, report whether any generated transaction
    satisfies it (addr match AND tx_bytes starts with one of the alternates)."""
    results: List[Tuple[Dict[str, Any], bool]] = []
    for entry in required:
        addr, alts, desc = _normalize_required(entry)
        ok = False
        for t in gen.transactions:
            if t.addr != addr:
                continue
            for prefix in alts:
                if len(t.tx_bytes) >= len(prefix) and t.tx_bytes[: len(prefix)] == prefix:
                    ok = True
                    break
            if ok:
                break
        results.append(({"addr": addr, "any_of": alts, "description": desc}, ok))
    return results


def _parse_num(v: Any) -> int:
    """Accept int or string integer forms used by oracle JSON files."""
    if isinstance(v, bool):
        raise TypeError(f"expected integer, got bool: {v!r}")
    if isinstance(v, str):
        return int(v, 0)
    return int(v)


def _normalize_byte_alts(raw: Any, *, where: str) -> List[List[int]]:
    """Normalize a list of byte-prefix alternatives."""
    if not isinstance(raw, list) or not raw:
        raise ValueError(f"{where}: expected non-empty list of byte prefixes")
    out: List[List[int]] = []
    for i, alt in enumerate(raw):
        if not isinstance(alt, list):
            raise ValueError(f"{where}[{i}]: expected list of bytes")
        out.append([_parse_num(b) & 0xFF for b in alt])
    return out


def _prefix_matches(buf: Sequence[int], prefix: Sequence[int]) -> bool:
    return len(buf) >= len(prefix) and list(buf[: len(prefix)]) == list(prefix)


def _match_write_then_read_rule(
    txns: Sequence[I2CTransaction],
    rule: Dict[str, Any],
    *,
    default_addr: Optional[int],
    rule_index: int,
) -> Dict[str, Any]:
    """Match a generic I2C flow: accepted write prefix followed by a read."""
    addr_raw = rule.get("addr", default_addr)
    if addr_raw is None:
        return {
            "id": str(rule.get("id", f"rule_{rule_index}")),
            "passed": False,
            "detail": "missing addr",
        }
    addr = _parse_num(addr_raw) & 0x7F
    write_any_of = _normalize_byte_alts(
        rule.get("write_any_of") or rule.get("command_any_of"),
        where=f"rules[{rule_index}].write_any_of",
    )
    read_min_len = int(_parse_num(rule.get("read_min_len", 1)))

    read_tx_any_of_raw = rule.get("read_tx_any_of")
    read_tx_any_of = None
    if read_tx_any_of_raw is not None:
        read_tx_any_of = _normalize_byte_alts(
            read_tx_any_of_raw,
            where=f"rules[{rule_index}].read_tx_any_of",
        )

    rule_id = str(rule.get("id", f"rule_{rule_index}"))
    for wi, wt in enumerate(txns):
        if wt.addr != addr or wt.is_read is not False:
            continue
        matched_prefix = next(
            (prefix for prefix in write_any_of if _prefix_matches(wt.tx_bytes, prefix)),
            None,
        )
        if matched_prefix is None:
            continue

        for ri in range(wi + 1, len(txns)):
            rt = txns[ri]
            if rt.addr != addr or rt.is_read is not True:
                continue
            if len(rt.rx_bytes) < read_min_len:
                continue
            if read_tx_any_of is not None and not any(
                _prefix_matches(rt.tx_bytes, prefix) for prefix in read_tx_any_of
            ):
                continue
            return {
                "id": rule_id,
                "passed": True,
                "addr": addr,
                "write_index": wi,
                "write_seq": wt.seq,
                "write_prefix": list(matched_prefix),
                "read_index": ri,
                "read_seq": rt.seq,
                "read_rx_len": len(rt.rx_bytes),
            }

    return {
        "id": rule_id,
        "passed": False,
        "addr": addr,
        "write_any_of": write_any_of,
        "read_min_len": read_min_len,
        "detail": "no accepted write prefix followed by a data read",
    }


# ---------- public comparators ----------

def compare_strict(generated: I2CTrace, golden: I2CTrace) -> LevelVerdict:
    """Byte-exact equality of the full transaction sequence."""
    g, x = generated.transactions, golden.transactions
    passed = len(g) == len(x) and all(
        (a.addr, a.is_read, a.tx_bytes, a.rx_bytes)
        == (b.addr, b.is_read, b.tx_bytes, b.rx_bytes)
        for a, b in zip(g, x)
    )
    first_diff = None
    if not passed:
        for i, (a, b) in enumerate(zip(g, x)):
            if (a.addr, a.is_read, a.tx_bytes, a.rx_bytes) != (
                b.addr, b.is_read, b.tx_bytes, b.rx_bytes,
            ):
                first_diff = {
                    "index": i,
                    "generated": a.to_dict(),
                    "golden": b.to_dict(),
                }
                break
        if first_diff is None and len(g) != len(x):
            first_diff = {"length_generated": len(g), "length_golden": len(x)}
    return LevelVerdict(
        device=generated.device,
        level="L3",
        passed=passed,
        claim="protocol-valid-strict",
        detail=("byte-exact match" if passed else "diverged from golden trace"),
        evidence={
            "policy": "strict",
            "generated_len": len(g),
            "golden_len": len(x),
            "first_diff": first_diff,
        },
    )


def compare_semantic(generated: I2CTrace, golden: I2CTrace) -> LevelVerdict:
    """Subset-containment on register-level signatures - order-insensitive."""
    from collections import Counter
    gen_norm = _normalize_combined_txns(generated)
    gol_norm = _normalize_combined_txns(golden)
    gen_counter = Counter(_txn_signature(t) for t in gen_norm)
    gol_counter = Counter(_txn_signature(t) for t in gol_norm)

    missing_counter = gol_counter - gen_counter
    extra_counter = gen_counter - gol_counter
    missing = sorted(missing_counter.elements())
    extra = sorted(extra_counter.elements())

    passed = not missing  # extras OK, but every golden sig must be present

    if passed:
        if extra:
            detail = (
                f"register-level signatures contain golden (+{sum(extra_counter.values())} "
                f"extra idempotent txn(s) tolerated)"
            )
        else:
            detail = "register-level signatures match golden (exact)"
    else:
        detail = (
            f"register-level signatures miss {sum(missing_counter.values())} "
            f"required txn(s) from golden"
        )
    return LevelVerdict(
        device=generated.device,
        level="L3",
        passed=passed,
        claim="protocol-valid-semantic",
        detail=detail,
        evidence={
            "policy": "semantic",
            "missing_vs_golden": missing[:16],
            "extra_vs_golden": extra[:16],
            "missing_count": sum(missing_counter.values()),
            "extra_count": sum(extra_counter.values()),
            "generated_len": sum(gen_counter.values()),
            "golden_len": sum(gol_counter.values()),
        },
    )


def compare_relaxed(
    generated: I2CTrace,
    required: Iterable[RequiredWriteLegacy],
) -> LevelVerdict:
    """Every required ``(addr, prefix)`` must appear as a TX prefix somewhere."""
    results = _iter_required_present(generated, required)
    passed = all(ok for _, ok in results)
    missing = [entry for entry, ok in results if not ok]
    return LevelVerdict(
        device=generated.device,
        level="L3",
        passed=passed,
        claim="protocol-valid-relaxed",
        detail=(
            "all required writes observed"
            if passed
            else f"{len(missing)} required write(s) missing"
        ),
        evidence={
            "policy": "relaxed",
            "required_total": len(results),
            "missing": missing,
        },
    )


def compare_protocol_equivalence(
    generated: I2CTrace,
    protocol_equivalence: Dict[str, Any],
) -> LevelVerdict:
    """Evaluate configured protocol-semantic alternatives."""
    if not isinstance(protocol_equivalence, dict):
        raise ValueError("protocol_equivalence must be a dict")

    rules = protocol_equivalence.get("rules")
    if rules is None:
        rules = [protocol_equivalence]
    if not isinstance(rules, list) or not rules:
        return LevelVerdict(
            device=generated.device,
            level="L3",
            passed=False,
            claim="protocol-valid-semantic",
            detail="protocol_equivalence has no rules",
            evidence={"policy": "semantic_equivalence", "generated_len": 0},
        )

    default_addr = protocol_equivalence.get("addr")
    if default_addr is not None:
        default_addr = _parse_num(default_addr) & 0x7F

    txns = _normalize_combined_txns(generated)
    matches: List[Dict[str, Any]] = []
    failures: List[Dict[str, Any]] = []
    for i, raw_rule in enumerate(rules):
        if not isinstance(raw_rule, dict):
            failures.append({
                "id": f"rule_{i}",
                "passed": False,
                "detail": "rule is not an object",
            })
            continue
        kind = str(raw_rule.get("kind", "write_then_read"))
        if kind != "write_then_read":
            failures.append({
                "id": str(raw_rule.get("id", f"rule_{i}")),
                "passed": False,
                "detail": f"unsupported rule kind {kind!r}",
            })
            continue
        try:
            res = _match_write_then_read_rule(
                txns, raw_rule, default_addr=default_addr, rule_index=i
            )
        except (KeyError, TypeError, ValueError) as e:
            res = {
                "id": str(raw_rule.get("id", f"rule_{i}")),
                "passed": False,
                "detail": f"invalid rule: {e}",
            }
        if res.get("passed"):
            matches.append(res)
        else:
            failures.append(res)

    passed = not failures
    return LevelVerdict(
        device=generated.device,
        level="L3",
        passed=passed,
        claim="protocol-valid-semantic",
        detail=(
            f"{len(matches)}/{len(rules)} protocol equivalence rule(s) matched"
            if passed
            else f"{len(failures)}/{len(rules)} protocol equivalence rule(s) failed"
        ),
        evidence={
            "policy": "semantic_equivalence",
            "generated_len": len(txns),
            "rules_total": len(rules),
            "passed_rules": len(matches),
            "matches": matches[:16],
            "failed_rules": failures[:16],
        },
    )


__all__ = [
    "RequiredWriteLegacy",
    "ProtocolPolicy",
    "compare_strict",
    "compare_semantic",
    "compare_relaxed",
    "compare_protocol_equivalence",
]
