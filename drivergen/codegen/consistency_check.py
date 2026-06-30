"""Stimulus consistency self-check."""
from __future__ import annotations

import ast
import dataclasses
import logging
import operator as _op
import re
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
)

logger = logging.getLogger(__name__)


# Verdicts

VERDICT_L1 = "consistent_l1"
VERDICT_L2 = "consistent_l2"
VERDICT_LLM_ONLY = "llm_only"
VERDICT_INCONSISTENT = "inconsistent"

_CONSISTENT_VERDICTS = (VERDICT_L1, VERDICT_L2)


@dataclasses.dataclass(frozen=True)
class StimulusConsistency:
    """Self-check verdict for one test stimulus."""
    name: str
    verdict: str
    layer: str
    evidence: str
    warnings: Tuple[str, ...] = ()

    def to_dict(self) -> Dict[str, Any]:
        return {
            "name":     self.name,
            "verdict":  self.verdict,
            "layer":    self.layer,
            "evidence": self.evidence,
            "warnings": list(self.warnings),
        }


@dataclasses.dataclass(frozen=True)
class ConsistencyReport:
    """Aggregate of per-stimulus verdicts for a test_plan."""
    stimuli: Tuple[StimulusConsistency, ...]
    inconsistent_count: int
    llm_only_count: int
    consistent_count: int
    summary: str

    @property
    def ok(self) -> bool:
        return self.inconsistent_count == 0

    @property
    def overall(self) -> str:
        if self.inconsistent_count:
            return "has_inconsistent"
        if not self.stimuli:
            return "empty"
        if self.llm_only_count and not self.consistent_count:
            return "all_llm_only"
        if self.consistent_count and not self.llm_only_count:
            return "all_consistent"
        return "mixed"

    def inconsistent(self) -> Tuple[StimulusConsistency, ...]:
        """Subset needing model feedback."""
        return tuple(s for s in self.stimuli
                     if s.verdict == VERDICT_INCONSISTENT)

    def to_dict(self) -> Dict[str, Any]:
        return {
            "overall":            self.overall,
            "ok":                 self.ok,
            "inconsistent_count": self.inconsistent_count,
            "llm_only_count":     self.llm_only_count,
            "consistent_count":   self.consistent_count,
            "summary":            self.summary,
            "stimuli":            [s.to_dict() for s in self.stimuli],
        }


# Byte extraction from mock_preload values

_HEX_TOKEN_RE = re.compile(r"0[xX][0-9A-Fa-f]+|[0-9A-Fa-f]+")


def _coerce_byte(value: Any) -> Optional[int]:
    """Return an 8-bit integer for int / hex-string byte tokens."""
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value if 0 <= value <= 0xFF else None
    if isinstance(value, str):
        try:
            n = int(value.strip(), 0)
        except ValueError:
            return None
        return n if 0 <= n <= 0xFF else None
    return None


def _parse_bytes_from_str(s: str) -> Optional[Tuple[int, ...]]:
    """Parse a string into a byte tuple."""
    stripped = s.strip()
    if not stripped:
        return None

    if stripped.startswith("[") and stripped.endswith("]"):
        try:
            parsed = ast.literal_eval(stripped)
        except (ValueError, SyntaxError):
            parsed = None
        if isinstance(parsed, (list, tuple)):
            out: List[int] = []
            for item in parsed:
                n = _coerce_byte(item)
                if n is None:
                    return None
                out.append(n)
            return tuple(out)

    tokens = [t for t in re.split(r"[\s,]+", stripped) if t]
    # Treat as a token sequence when either there are multiple tokens
    # or the single token is explicitly hex-prefixed.
    if len(tokens) > 1 or (len(tokens) == 1 and tokens[0].lower().startswith("0x")):
        out = []
        ok = True
        for t in tokens:
            try:
                if t.lower().startswith("0x"):
                    n = int(t, 16)
                elif re.fullmatch(r"[0-9]+", t):
                    n = int(t, 10)
                elif re.fullmatch(r"[0-9A-Fa-f]+", t):
                    n = int(t, 16)
                else:
                    ok = False
                    break
            except ValueError:
                ok = False
                break
            if not (0 <= n <= 0xFF):
                ok = False
                break
            out.append(n)
        if ok and out:
            return tuple(out)

    # Last resort: continuous hex pair-split. Drop any ``0x`` prefix
    # first so ``"0x04B0"`` == ``"04B0"``.
    body = stripped
    if body.lower().startswith("0x"):
        body = body[2:]
    if re.fullmatch(r"[0-9A-Fa-f]+", body) and len(body) % 2 == 0:
        out = []
        for i in range(0, len(body), 2):
            try:
                out.append(int(body[i:i + 2], 16))
            except ValueError:
                return None
        if out:
            return tuple(out)

    return None


def _extract_bytes(value: Any) -> Optional[Tuple[int, ...]]:
    """Extract a byte tuple from an arbitrary mock_preload value."""
    if value is None:
        return None
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        n = _coerce_byte(value)
        if n is not None:
            return (n,)
        return None
    if isinstance(value, (list, tuple)):
        out: List[int] = []
        for item in value:
            n = _coerce_byte(item)
            if n is None:
                return None
            out.append(n)
        return tuple(out)
    if isinstance(value, str):
        return _parse_bytes_from_str(value)
    return None


def _first_byte_sequence(preload: Mapping[str, Any]) -> Optional[Tuple[int, ...]]:
    """Pick the longest byte sequence from a mock_preload dict."""
    best: Optional[Tuple[int, ...]] = None
    for k, v in preload.items():
        # Skip request descriptors; they are not data payloads.
        if isinstance(k, str) and k.startswith("req_"):
            continue
        bs = _extract_bytes(v)
        if bs is None:
            continue
        if best is None or len(bs) > len(best):
            best = bs
    return best


def _format_bytes(bs: Sequence[int]) -> str:
    return "[" + ", ".join(f"0x{b:02X}" for b in bs) + "]"


# L1: exhaustive byte-order interpretations

@dataclasses.dataclass(frozen=True)
class ByteInterpretation:
    label: str
    value: int


def _signed(val: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    if val & sign:
        return val - (1 << bits)
    return val


def _enumerate_interpretations(bs: Sequence[int]) -> List[ByteInterpretation]:
    """Enumerate every plausible numeric interpretation of ``bs``."""
    out: List[ByteInterpretation] = []
    n = len(bs)
    if n == 0:
        return out

    if n >= 4:
        be = (bs[0] << 24) | (bs[1] << 16) | (bs[2] << 8) | bs[3]
        le = (bs[3] << 24) | (bs[2] << 16) | (bs[1] << 8) | bs[0]
        out.append(ByteInterpretation("big_endian_u32", be))
        out.append(ByteInterpretation("big_endian_i32", _signed(be, 32)))
        out.append(ByteInterpretation("little_endian_u32", le))
        out.append(ByteInterpretation("little_endian_i32", _signed(le, 32)))

    if n >= 3:
        be = (bs[0] << 16) | (bs[1] << 8) | bs[2]
        le = (bs[2] << 16) | (bs[1] << 8) | bs[0]
        out.append(ByteInterpretation("big_endian_u24", be))
        out.append(ByteInterpretation("big_endian_i24", _signed(be, 24)))
        out.append(ByteInterpretation("little_endian_u24", le))
        out.append(ByteInterpretation("little_endian_i24", _signed(le, 24)))

    if n >= 2:
        be = (bs[0] << 8) | bs[1]
        le = (bs[1] << 8) | bs[0]
        out.append(ByteInterpretation("big_endian_u16", be))
        out.append(ByteInterpretation("big_endian_i16", _signed(be, 16)))
        out.append(ByteInterpretation("little_endian_u16", le))
        out.append(ByteInterpretation("little_endian_i16", _signed(le, 16)))
        last = bs[-1] & 0xFF
        out.append(ByteInterpretation("last_u8", last))
        out.append(ByteInterpretation("last_i8", _signed(last, 8)))

    if n >= 1:
        v = bs[0] & 0xFF
        out.append(ByteInterpretation("single_u8", v))
        out.append(ByteInterpretation("single_i8", _signed(v, 8)))

    return out


def _l1_match(
    bs: Sequence[int],
    expected: float,
    tolerance: float,
) -> Optional[ByteInterpretation]:
    """Return the first interpretation that matches ``expected``."""
    for interp in _enumerate_interpretations(bs):
        try:
            if abs(interp.value - expected) <= tolerance:
                return interp
        except TypeError:
            continue
    return None


# L2: AST safe evaluation of derivation strings

_L2_BINOPS: Dict[type, Any] = {
    ast.Add:       _op.add,
    ast.Sub:       _op.sub,
    ast.Mult:      _op.mul,
    ast.Div:       _op.truediv,
    ast.Mod:       _op.mod,
    ast.LShift:    _op.lshift,
    ast.RShift:    _op.rshift,
    ast.BitAnd:    _op.and_,
    ast.BitOr:     _op.or_,
    ast.BitXor:    _op.xor,
    ast.Pow:       _op.pow,
}

_L2_UNARYOPS: Dict[type, Any] = {
    ast.USub:   _op.neg,
    ast.UAdd:   _op.pos,
    ast.Invert: _op.invert,
}

_L2_EXPR_RE = re.compile(
    r"(?<![A-Za-z_])"
    r"(?:0x[0-9A-Fa-f]+|\d+(?:\.\d+)?(?:[eE][+\-]?\d+)?|"
    r"<<|>>|[()+\-*/%&|^~]|\s){2,}"
    r"(?![A-Za-z_])"
)

_INT_LITERAL_RE = re.compile(
    r"(?<![\w.])[-+]?(?:0[xX][0-9A-Fa-f]+|\d+)(?![\w.])"
)


def _c_trunc_div(left: Any, right: Any) -> int:
    """Integer division with C99 trunc-toward-zero semantics."""
    lhs = int(left)
    rhs = int(right)
    if rhs == 0:
        raise ZeroDivisionError("division by zero")
    magnitude = abs(lhs) // abs(rhs)
    return -magnitude if (lhs < 0) ^ (rhs < 0) else magnitude


def _safe_eval(node: ast.AST) -> Any:
    """Evaluate an AST node with a strict whitelist. Raises on violation."""
    if isinstance(node, ast.Expression):
        return _safe_eval(node.body)
    if isinstance(node, ast.Constant):
        if isinstance(node.value, bool):
            raise ValueError("bool literal not allowed")
        if isinstance(node.value, (int, float)):
            return node.value
        raise ValueError(
            f"constant type {type(node.value).__name__} not allowed"
        )
    if isinstance(node, ast.UnaryOp):
        fn = _L2_UNARYOPS.get(type(node.op))
        if fn is None:
            raise ValueError(f"unary op {type(node.op).__name__} not allowed")
        return fn(_safe_eval(node.operand))
    if isinstance(node, ast.BinOp):
        if isinstance(node.op, ast.FloorDiv):
            return _c_trunc_div(
                _safe_eval(node.left),
                _safe_eval(node.right),
            )
        fn = _L2_BINOPS.get(type(node.op))
        if fn is None:
            raise ValueError(f"binary op {type(node.op).__name__} not allowed")
        return fn(_safe_eval(node.left), _safe_eval(node.right))
    raise ValueError(f"ast node {type(node).__name__} not allowed")


def _is_trivial_numeric_expr(node: ast.AST) -> bool:
    """True for expressions that merely restate a numeric literal."""
    if isinstance(node, ast.Expression):
        return _is_trivial_numeric_expr(node.body)
    if isinstance(node, ast.Constant):
        return isinstance(node.value, (int, float)) and not isinstance(node.value, bool)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
        return _is_trivial_numeric_expr(node.operand)
    return False


def _derivation_expression_candidates(derivation: str) -> Tuple[str, ...]:
    """Extract AST-checkable numeric expressions from free-form derivations."""
    seen = set()
    candidates: List[str] = []
    raw_candidates = list(derivation.split("="))
    raw_candidates.extend(_L2_EXPR_RE.findall(derivation))
    for raw in raw_candidates:
        seg = raw.strip().strip(",;:")
        if not seg or seg in seen:
            continue
        if not any(ch.isdigit() for ch in seg):
            continue
        seen.add(seg)
        candidates.append(seg)
    return tuple(candidates)


def _integer_literals(expr: str) -> Tuple[int, ...]:
    """Return integer literals in an AST-checkable derivation segment."""
    out: List[int] = []
    for match in _INT_LITERAL_RE.finditer(expr):
        try:
            out.append(int(match.group(0), 0))
        except ValueError:
            continue
    return tuple(out)


def _preload_l2_anchor_values(
    bs: Sequence[int],
) -> Tuple[Tuple[int, ...], Tuple[int, ...]]:
    """Return strong and weak numeric anchors derived from preload bytes."""
    strong = set()
    weak = set()
    for b in bs:
        byte = int(b) & 0xFF
        weak.add(byte)
        weak.add(_signed(byte, 8))
    n = len(bs)
    if n == 1:
        byte = int(bs[0]) & 0xFF
        strong.add(byte)
        strong.add(_signed(byte, 8))
    for start in range(n):
        max_width = min(4, n - start)
        for width in range(2, max_width + 1):
            chunk = tuple(int(b) & 0xFF for b in bs[start:start + width])
            for interp in _enumerate_interpretations(chunk):
                strong.add(interp.value)
    return tuple(sorted(strong)), tuple(sorted(weak))


def _l2_segment_anchored_to_preload(
    seg: str,
    preload_bytes: Sequence[int],
    derived_values: Sequence[int] = (),
) -> bool:
    """True if an L2 expression uses numeric literals from preload bytes."""
    literals = _integer_literals(seg)
    if not literals:
        return False
    strong_values, weak_values = _preload_l2_anchor_values(preload_bytes)
    strong = set(strong_values)
    strong.update(int(v) for v in derived_values)
    if any(v in strong for v in literals):
        return True
    weak = set(weak_values)
    weak_hits = {v for v in literals if v in weak}
    return len(weak_hits) >= 2


def _anchorable_int(value: Any) -> Optional[int]:
    """Return an exact integer anchor for an evaluated numeric expression."""
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        rounded = round(value)
        if abs(value - rounded) <= 1e-9:
            return int(rounded)
    return None


def _try_l2_derivation(
    derivation: str,
    expected: float,
    tolerance: float,
    *,
    preload_bytes: Optional[Sequence[int]] = None,
) -> Optional[str]:
    """Try evaluating ``derivation`` for equality with ``expected``."""
    if not derivation:
        return None
    derived_anchors: List[int] = []
    for seg in _derivation_expression_candidates(derivation):
        if not seg:
            continue
        try:
            tree = ast.parse(seg, mode="eval")
        except SyntaxError:
            continue
        if _is_trivial_numeric_expr(tree):
            continue
        try:
            val = _safe_eval(tree)
        except (ValueError, ZeroDivisionError, TypeError):
            continue
        anchored = preload_bytes is None or _l2_segment_anchored_to_preload(
            seg,
            preload_bytes,
            derived_anchors,
        )
        if anchored:
            anchor = _anchorable_int(val)
            if anchor is not None and anchor not in derived_anchors:
                derived_anchors.append(anchor)
        try:
            if abs(val - expected) <= tolerance:
                if not anchored:
                    continue
                return f"derivation `{seg}` -> {val} == expected {expected}"
        except TypeError:
            continue
    return None


def _unit_scales_from_derivation(derivation: str) -> Tuple[float, ...]:
    lowered = derivation.lower()
    scales: List[float] = []
    if any(tok in lowered for tok in ("milli", "mdeg", "mdegc", "mc")):
        scales.append(1000.0)
    if re.search(r"\bmm\b", lowered) or "millimeter" in lowered:
        scales.append(1000.0)
    if any(tok in lowered for tok in ("micro", "usec", "uhz", "upa")):
        scales.append(1000000.0)
    return tuple(scales)


def _try_l2_unit_scaled_derivation(
    derivation: str,
    expected: float,
    tolerance: float,
    *,
    preload_bytes: Optional[Sequence[int]] = None,
) -> Optional[str]:
    """Try L2 derivations that explicitly convert base units to scaled units."""
    scales = _unit_scales_from_derivation(derivation)
    if not derivation or not scales:
        return None
    derived_anchors: List[int] = []
    evaluated: List[Tuple[str, Any, bool]] = []
    for seg in _derivation_expression_candidates(derivation):
        if not seg:
            continue
        try:
            tree = ast.parse(seg, mode="eval")
        except SyntaxError:
            continue
        if _is_trivial_numeric_expr(tree):
            continue
        try:
            val = _safe_eval(tree)
        except (ValueError, ZeroDivisionError, TypeError):
            continue
        anchored = preload_bytes is None or _l2_segment_anchored_to_preload(
            seg,
            preload_bytes,
            derived_anchors,
        )
        if anchored:
            anchor = _anchorable_int(val)
            if anchor is not None and anchor not in derived_anchors:
                derived_anchors.append(anchor)
        evaluated.append((seg, val, anchored))

    for seg, val, anchored_initially in evaluated:
        anchored = anchored_initially or preload_bytes is None or _l2_segment_anchored_to_preload(
            seg,
            preload_bytes,
            derived_anchors,
        )
        for scale in scales:
            try:
                scaled = val * scale
                tol = max(tolerance, 1e-9)
                if abs(scaled - expected) <= tol:
                    if not anchored:
                        continue
                    return (
                        f"derivation `{seg}` -> {val} * {scale:g} "
                        f"== expected {expected}"
                    )
            except TypeError:
                continue
    return None


def _derivation_looks_complex(derivation: str) -> bool:
    """Heuristic: does this derivation need calibration / compensation?."""
    if not derivation:
        return False
    lowered = derivation.lower()
    tokens = (
        "compensat", "calibrat", "cal_", "dig_",
        "t_fine", "adc_t", "adc_p", "adc_h",
        "polynomial", "2nd order", "second order",
        "look_up", "lookup",
        "coef", "coeff", "coefficient",
    )
    if any(t in lowered for t in tokens):
        return True
    return re.search(r"\blut\b", lowered) is not None


def _preload_has_signal_schedule(preload: Mapping[str, Any]) -> bool:
    """True when mock_preload contains a GPIO/timing style schedule."""
    for key, value in preload.items():
        if str(key).lower() != "schedule":
            continue
        if not isinstance(value, (list, tuple)) or not value:
            return False
        return all(isinstance(item, (list, tuple)) and len(item) >= 2 for item in value)
    return False


def _try_l2_numeric_derivation(
    derivation: str,
    expected: float,
    tolerance: float,
    *,
    allow_rounding_tolerance: bool = False,
    preload_bytes: Optional[Sequence[int]] = None,
) -> Optional[str]:
    """Try all L2 numeric derivation paths with one shared policy."""
    tol = float(tolerance)
    if allow_rounding_tolerance:
        tol = max(tol, 1.0)
    evidence = _try_l2_derivation(
        derivation,
        expected,
        tol,
        preload_bytes=preload_bytes,
    )
    if evidence is not None:
        return evidence
    return _try_l2_unit_scaled_derivation(
        derivation,
        expected,
        tol,
        preload_bytes=preload_bytes,
    )


# Per-eval_class stimulus checkers

def _check_single_channel(stim: Mapping[str, Any]) -> StimulusConsistency:
    name = str(stim.get("name") or "<unnamed>")
    preload = stim.get("mock_preload") or {}
    if not isinstance(preload, Mapping) or not preload:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="mock_preload empty - runtime probe will verify",
        )
    if "expected_read_raw" not in stim:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="no expected_read_raw - runtime probe will verify",
        )

    expected = float(stim["expected_read_raw"])
    tolerance = float(stim.get("raw_tolerance") or 0)
    derivation = str(stim.get("derivation") or "")

    bs = _first_byte_sequence(preload)
    if bs is None:
        l2_evidence = _try_l2_numeric_derivation(
            derivation,
            expected,
            tolerance,
            allow_rounding_tolerance=_preload_has_signal_schedule(preload),
        )
        if l2_evidence is not None:
            return StimulusConsistency(
                name=name, verdict=VERDICT_L2, layer="L2",
                evidence=l2_evidence,
                warnings=("preload is non-byte signal/schedule data",),
            )
        if _derivation_looks_complex(derivation):
            return StimulusConsistency(
                name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
                evidence=(f"complex derivation `{derivation}` - "
                          "runtime probe will verify"),
            )
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="mock_preload has no parseable byte sequence",
            warnings=("preload not L1-checkable",),
        )

    interp = _l1_match(bs, expected, tolerance)
    if interp is not None:
        return StimulusConsistency(
            name=name, verdict=VERDICT_L1, layer="L1",
            evidence=f"bytes={_format_bytes(bs)} as {interp.label} = {interp.value}",
        )

    l2_evidence = _try_l2_numeric_derivation(
        derivation,
        expected,
        tolerance,
        preload_bytes=bs,
    )
    if l2_evidence is not None:
        return StimulusConsistency(
            name=name, verdict=VERDICT_L2, layer="L2",
            evidence=l2_evidence,
        )

    if _derivation_looks_complex(derivation):
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence=(f"complex derivation `{derivation}` - "
                      "runtime probe will verify"),
        )

    return StimulusConsistency(
        name=name, verdict=VERDICT_INCONSISTENT, layer="L1",
        evidence=(
            f"bytes={_format_bytes(bs)} cannot yield expected_read_raw="
            f"{expected} under any standard interpretation; "
            f"derivation=`{derivation}`"
            if derivation else
            f"bytes={_format_bytes(bs)} cannot yield expected_read_raw="
            f"{expected} under any standard interpretation; "
            "no derivation supplied"
        ),
        warnings=("model-derived expected value does not match preload",),
    )


def _channel_preload_bytes(
    preload: Mapping[str, Any],
    ch_id: str,
) -> Optional[Tuple[int, ...]]:
    """Find the byte sequence most likely tied to ``ch_id``."""
    needle = ch_id.lower()
    for k, v in preload.items():
        if needle in str(k).lower():
            bs = _extract_bytes(v)
            if bs is not None:
                return bs
    combined: List[int] = []
    for _k, v in preload.items():
        bs = _extract_bytes(v)
        if bs is not None:
            combined.extend(bs)
    if combined:
        return tuple(combined)
    return _first_byte_sequence(preload)


def _normalise_channel_key(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.lower())


def _hinted_channel_preload_bytes(
    stim: Mapping[str, Any],
    ch_id: str,
) -> Optional[Tuple[int, ...]]:
    """Return channel-local bytes when the plan carries an explicit hint."""
    hints = stim.get("channel_preload_bytes")
    if not isinstance(hints, Mapping):
        return None

    target = _normalise_channel_key(str(ch_id))
    selected: Any = None
    for key, value in hints.items():
        if _normalise_channel_key(str(key)) == target:
            selected = value
            break
    if selected is None:
        return None

    if isinstance(selected, Mapping):
        combined: List[int] = []
        for _key, value in selected.items():
            bs = _extract_bytes(value)
            if bs is None:
                return None
            combined.extend(bs)
        return tuple(combined) if combined else None

    return _extract_bytes(selected)


def _channel_preload_bytes_for_stim(
    stim: Mapping[str, Any],
    preload: Mapping[str, Any],
    ch_id: str,
) -> Optional[Tuple[int, ...]]:
    hinted = _hinted_channel_preload_bytes(stim, ch_id)
    if hinted is not None:
        return hinted
    return _channel_preload_bytes(preload, ch_id)


def _preload_contains_zero_byte(preload: Mapping[str, Any]) -> bool:
    for value in preload.values():
        bs = _extract_bytes(value)
        if bs is not None and any(b == 0 for b in bs):
            return True
    return False


def _try_l2_channel_zero_derivation(
    *,
    derivation: str,
    ch_id: str,
    expected: float,
    tolerance: float,
    preload: Mapping[str, Any],
) -> Optional[str]:
    """Accept explicit channel-local zero derivations backed by zero bytes."""
    if abs(expected) > max(float(tolerance), 0.0):
        return None
    if not derivation or not _preload_contains_zero_byte(preload):
        return None
    needle = str(ch_id).lower()
    for frag in re.split(r"[;\n]+", derivation):
        lowered = frag.lower()
        if needle not in lowered:
            continue
        if "not read" in lowered or "unused" in lowered or "placeholder" in lowered:
            continue
        if "0x00" in lowered or re.search(r"(?:=>|=)\s*0(?:\.0+)?\b", lowered):
            return (
                f"channel `{ch_id}` derivation explicitly yields zero "
                "from zero preload bytes"
            )
    return None


def _channel_derivation_says_not_read(derivation: str, ch_id: str) -> bool:
    """True when the derivation explicitly marks a channel as not covered."""
    if not derivation:
        return False
    needle = str(ch_id).lower()
    for frag in re.split(r"[;\n]+", derivation):
        lowered = frag.lower()
        if needle not in lowered:
            continue
        if (
            "not read" in lowered
            or "unused" in lowered
            or "placeholder" in lowered
            or "not provided" in lowered
        ):
            return True
    return False


def _check_multi_channel(stim: Mapping[str, Any]) -> StimulusConsistency:
    name = str(stim.get("name") or "<unnamed>")
    preload = stim.get("mock_preload") or {}
    expected_chans = stim.get("expected_channels") or {}

    if not isinstance(preload, Mapping) or not isinstance(expected_chans, Mapping):
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="multi_channel stim missing preload/expected_channels",
        )
    if not preload or not expected_chans:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="multi_channel stim incomplete",
        )

    tolerance = float(stim.get("raw_tolerance") or 0)
    derivation = str(stim.get("derivation") or "")
    matched: List[str] = []
    matched_layers: List[str] = []
    unmatched: List[str] = []

    for ch_id, expected in expected_chans.items():
        try:
            exp_val = float(expected)
        except (TypeError, ValueError):
            unmatched.append(f"ch={ch_id}: non-numeric expected")
            continue
        bs = _channel_preload_bytes_for_stim(stim, preload, str(ch_id))
        if bs is None:
            unmatched.append(f"ch={ch_id}: no bytes")
            continue
        if _channel_derivation_says_not_read(derivation, str(ch_id)):
            unmatched.append(f"ch={ch_id}: derivation says channel is not read")
            continue
        interp = _l1_match(bs, exp_val, tolerance)
        if interp is None:
            l2_evidence = _try_l2_numeric_derivation(
                derivation,
                exp_val,
                tolerance,
                preload_bytes=bs,
            )
            if l2_evidence is not None:
                matched.append(f"ch={ch_id}:{l2_evidence}")
                matched_layers.append(VERDICT_L2)
                continue
            zero_evidence = _try_l2_channel_zero_derivation(
                derivation=derivation,
                ch_id=str(ch_id),
                expected=exp_val,
                tolerance=tolerance,
                preload=preload,
            )
            if zero_evidence is not None:
                matched.append(f"ch={ch_id}:{zero_evidence}")
                matched_layers.append(VERDICT_L2)
                continue
            unmatched.append(
                f"ch={ch_id}: bytes={_format_bytes(bs)} "
                f"no interp = {exp_val}"
            )
        else:
            matched.append(
                f"ch={ch_id}:{interp.label}={interp.value}"
            )
            matched_layers.append(VERDICT_L1)

    if matched and not unmatched:
        verdict = VERDICT_L2 if VERDICT_L2 in matched_layers else VERDICT_L1
        return StimulusConsistency(
            name=name,
            verdict=verdict,
            layer="L2" if verdict == VERDICT_L2 else "L1",
            evidence="; ".join(matched),
        )
    if matched and unmatched:
        # Partial coverage is still useful but carries per-channel warnings.
        verdict = VERDICT_L2 if VERDICT_L2 in matched_layers else VERDICT_L1
        return StimulusConsistency(
            name=name,
            verdict=verdict,
            layer="L2" if verdict == VERDICT_L2 else "L1",
            evidence="partial: " + "; ".join(matched),
            warnings=tuple(unmatched),
        )
    return StimulusConsistency(
        name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
        evidence="no channel could be L1-verified",
        warnings=tuple(unmatched),
    )


def _check_memory(stim: Mapping[str, Any]) -> StimulusConsistency:
    name = str(stim.get("name") or "<unnamed>")
    preload = stim.get("mock_preload") or {}
    expected_hex = stim.get("expected_mem_bytes")

    if not isinstance(preload, Mapping) or not preload:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="memory stim missing mock_preload",
        )
    if not expected_hex:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="no expected_mem_bytes - runtime probe will verify",
        )

    preload_bytes = _first_byte_sequence(preload)
    expected_bytes = _extract_bytes(expected_hex)

    if preload_bytes is None or expected_bytes is None:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence=(
                f"memory stim not L1-checkable (preload_bytes="
                f"{preload_bytes!r}, expected_bytes={expected_bytes!r})"
            ),
        )

    if preload_bytes == expected_bytes:
        return StimulusConsistency(
            name=name, verdict=VERDICT_L1, layer="L1",
            evidence=(f"preload matches expected_mem_bytes exactly "
                      f"({_format_bytes(preload_bytes)})"),
        )
    # Allow a short header before the expected payload.
    if len(expected_bytes) < len(preload_bytes):
        for start in range(0, len(preload_bytes) - len(expected_bytes) + 1):
            if preload_bytes[start:start + len(expected_bytes)] == expected_bytes:
                prefix = _format_bytes(preload_bytes[:start])
                return StimulusConsistency(
                    name=name, verdict=VERDICT_L1, layer="L1",
                    evidence=(
                        f"preload contains expected_mem_bytes "
                        f"({_format_bytes(expected_bytes)}) "
                        f"with leading {prefix}"
                    ),
                    warnings=(f"preload has {start} header byte(s) not in expected",),
                )
    return StimulusConsistency(
        name=name, verdict=VERDICT_INCONSISTENT, layer="L1",
        evidence=(
            f"preload {_format_bytes(preload_bytes)} does not match "
            f"expected_mem_bytes {_format_bytes(expected_bytes)}"
        ),
        warnings=("model-derived memory stimulus does not match preload",),
    )


def _check_display(stim: Mapping[str, Any]) -> StimulusConsistency:
    name = str(stim.get("name") or "<unnamed>")
    # Display output is only verifiable by running the driver.
    return StimulusConsistency(
        name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
        evidence="display stims verified at runtime (no L1/L2 path)",
    )


def _check_rtc(stim: Mapping[str, Any]) -> StimulusConsistency:
    name = str(stim.get("name") or "<unnamed>")
    preload = stim.get("mock_preload") or {}
    expected_time = stim.get("expected_time") or {}

    if not isinstance(preload, Mapping) or not isinstance(expected_time, Mapping):
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="rtc stim missing preload/expected_time",
        )
    if not preload or not expected_time:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="rtc stim incomplete",
        )

    bs = _first_byte_sequence(preload)
    if bs is None:
        return StimulusConsistency(
            name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence="rtc preload has no parseable byte sequence",
        )

    # Match BCD bytes greedily against expected time fields.
    pending_fields: Dict[str, int] = {}
    for f, v in expected_time.items():
        try:
            pending_fields[str(f)] = int(v)
        except (TypeError, ValueError):
            continue

    matched_fields: List[str] = []
    for i, b in enumerate(bs[:7]):   # RTCs have at most 7 time regs
        high = (b >> 4) & 0xF
        low = b & 0xF
        if high > 9 or low > 9:
            continue
        candidates = {high * 10 + low}
        # Hour registers often set bits 6/7 for AM/PM or 24-hour flag;
        # try the low-6-bits interpretation too.
        candidates.add(((b & 0x3F) >> 4) * 10 + (b & 0xF))
        hit_field: Optional[str] = None
        for field, exp in pending_fields.items():
            if exp in candidates:
                hit_field = field
                break
        if hit_field is not None:
            matched_fields.append(
                f"byte[{i}]=0x{b:02X}(BCD)->{hit_field}={pending_fields[hit_field]}"
            )
            del pending_fields[hit_field]

    total = len(expected_time)
    hits = len(matched_fields)
    required = max(2, (total + 1) // 2)

    if hits >= required:
        return StimulusConsistency(
            name=name, verdict=VERDICT_L1, layer="L1",
            evidence=f"{hits}/{total} BCD fields matched: "
                     + "; ".join(matched_fields),
        )
    if hits:
        return StimulusConsistency(
            name=name, verdict=VERDICT_L1, layer="L1",
            evidence=f"{hits}/{total} BCD fields matched: "
                     + "; ".join(matched_fields),
            warnings=(f"only {hits}/{total} fields matched; "
                      "RTC layout may differ from expected ordering",),
        )
    return StimulusConsistency(
        name=name, verdict=VERDICT_LLM_ONLY, layer="L3",
        evidence="no RTC field BCD-matched; runtime probe will verify",
    )


# Dispatch + public API

_DISPATCH = {
    EVAL_CLASS_SINGLE_CHANNEL: _check_single_channel,
    EVAL_CLASS_MULTI_CHANNEL:  _check_multi_channel,
    EVAL_CLASS_MEMORY:         _check_memory,
    EVAL_CLASS_DISPLAY:        _check_display,
    EVAL_CLASS_RTC:            _check_rtc,
}


def check_stimulus(
    stimulus: Mapping[str, Any],
    eval_class: str,
) -> StimulusConsistency:
    """Self-check a single stimulus under the given ``eval_class``."""
    checker = _DISPATCH.get(eval_class)
    if checker is None:
        return StimulusConsistency(
            name=str(stimulus.get("name") or "<unnamed>"),
            verdict=VERDICT_LLM_ONLY, layer="L3",
            evidence=f"unknown eval_class {eval_class!r}; skipping L1/L2",
            warnings=(f"eval_class {eval_class!r} not in {list(_DISPATCH)}",),
        )
    return checker(stimulus)


def check_consistency(
    test_plan: Mapping[str, Any],
    eval_class: str,
) -> ConsistencyReport:
    """Run the L1/L2/L3 self-check on every stimulus in ``test_plan``."""
    stimuli_raw = test_plan.get("test_stimuli") or []
    results: List[StimulusConsistency] = []
    for stim in stimuli_raw:
        if not isinstance(stim, Mapping):
            continue
        results.append(check_stimulus(stim, eval_class))

    inc = sum(1 for r in results if r.verdict == VERDICT_INCONSISTENT)
    llm_only = sum(1 for r in results if r.verdict == VERDICT_LLM_ONLY)
    cons = sum(1 for r in results if r.verdict in _CONSISTENT_VERDICTS)

    parts: List[str] = [f"{len(results)} stim"]
    if cons:
        parts.append(f"{cons} L1/L2")
    if llm_only:
        parts.append(f"{llm_only} llm_only")
    if inc:
        parts.append(f"{inc} inconsistent")
    summary = "; ".join(parts)

    logger.info("consistency_check: %s", summary)
    return ConsistencyReport(
        stimuli=tuple(results),
        inconsistent_count=inc,
        llm_only_count=llm_only,
        consistent_count=cons,
        summary=summary,
    )


__all__ = [
    "VERDICT_L1",
    "VERDICT_L2",
    "VERDICT_LLM_ONLY",
    "VERDICT_INCONSISTENT",
    "StimulusConsistency",
    "ConsistencyReport",
    "check_stimulus",
    "check_consistency",
]
