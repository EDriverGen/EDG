"""Deterministic post-processing for ``device_ir`` payloads."""
from __future__ import annotations

import ast
import re
from decimal import Decimal, InvalidOperation
from typing import Any, Mapping, MutableMapping, Sequence

__all__ = [
    "canonicalize_address_rule",
    "canonicalize_conversion_formulae",
    "canonicalize_gpio_byte_frame_sources",
    "canonicalize_operation_flows",
    "canonicalize_primary_interface_and_variant",
    "EXPLICIT_DEFAULT_PATTERNS",
    "STRAP_PIN_FLOAT_PATTERNS",
    "STRAP_PIN_TIE_PATTERNS",
    "ADDRESSING_FORM_VALUES",
]


EXPLICIT_DEFAULT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bdefault\s+address\b", re.IGNORECASE),
    re.compile(r"\bfactory\s+default\b", re.IGNORECASE),
    re.compile(r"\bdefault\b", re.IGNORECASE),
    re.compile(r"\bpreset\b", re.IGNORECASE),
    re.compile(r"\bprimary\b", re.IGNORECASE),
)
"""High-confidence patterns for descriptions that name the silicon default."""

STRAP_PIN_TIE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bA[012]\s*=\s*(?:GND|0|low)\b", re.IGNORECASE),
    re.compile(r"\bSDO\s*=\s*(?:GND|0|low)\b", re.IGNORECASE),
    re.compile(r"\bAD0\s*=\s*(?:GND|0|low)\b", re.IGNORECASE),
    re.compile(r"\bCS\s*=\s*(?:GND|0|low)\b", re.IGNORECASE),
)
"""Fallback patterns for descriptions that tie address strap pins low."""

STRAP_PIN_FLOAT_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"\bA[012]\s*=\s*(?:float|floating|open|nc|n/c)\b", re.IGNORECASE),
    re.compile(r"\bADDR\s*=\s*(?:float|floating|open|nc|n/c)\b", re.IGNORECASE),
    re.compile(r"\bAD0\s*=\s*(?:float|floating|open|nc|n/c)\b", re.IGNORECASE),
)
"""Fallback patterns for tri-state address tables with floating pins."""

_FLOAT_PIN_ASSIGNMENT_RE = re.compile(
    r"\b(?:A[012]|ADDR|AD0|SDO)\s*=\s*(?:float|floating|open|nc|n/c)\b",
    re.IGNORECASE,
)
_LOW_PIN_ASSIGNMENT_RE = re.compile(
    r"\b(?:A[012]|ADDR|AD0|SDO|CS)\s*=\s*(?:GND|0|low)\b",
    re.IGNORECASE,
)

ADDRESSING_FORM_VALUES: frozenset[str] = frozenset({"7-bit", "8-bit"})
BUS_TRANSACTION_KINDS: frozenset[str] = frozenset({"write", "read", "write_then_read"})
SIGNAL_STEP_OPS: frozenset[str] = frozenset({
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
})
NO_REGISTER_TIMING_ACCESS_KINDS: frozenset[str] = frozenset({
    "gpio_timing",
    "analog",
})
NO_REGISTER_TIMING_BUS_TYPES: frozenset[str] = frozenset({
    "gpio",
    "gpio_timing",
    "single_wire",
    "onewire",
    "one_wire",
    "1wire",
    "1-wire",
    "analog",
})

_NO_BUS_INIT_EVIDENCE_RE = re.compile(
    r"\b(power[-\s]*on|power[-\s]*up|startup|start[-\s]*up|warm[-\s]*up|"
    r"stabili[sz]e|settle|settling|reset\s+state|defaults?|"
    r"no\s+bus\s+(?:init|initiali[sz]ation|write)|"
    r"no\s+explicit\s+(?:config|configuration|setup|write)|"
    r"config(?:uration)?\s+writes?\s+(?:is|are)\s+not\s+required)\b",
    re.IGNORECASE,
)

_PART_TOKEN_RE = re.compile(r"\b[A-Za-z][A-Za-z0-9-]*\d[A-Za-z0-9-]*\b")

_BUS_MARKER_PATTERNS: dict[str, tuple[re.Pattern[str], ...]] = {
    "i2c": (
        re.compile(r"\bi2c\b", re.IGNORECASE),
        re.compile(r"\bsmbus\b", re.IGNORECASE),
        re.compile(r"\bsda\b", re.IGNORECASE),
        re.compile(r"\bscl\b", re.IGNORECASE),
        re.compile(r"\bslave\s+address\b", re.IGNORECASE),
        re.compile(r"\bregister\s+pointer\b", re.IGNORECASE),
    ),
    "spi": (
        re.compile(r"\bspi\b", re.IGNORECASE),
        re.compile(r"\bsck\b", re.IGNORECASE),
        re.compile(r"\bmiso\b", re.IGNORECASE),
        re.compile(r"\bmosi\b", re.IGNORECASE),
        re.compile(r"\bchip\s+select\b", re.IGNORECASE),
        re.compile(r"\bcs\b", re.IGNORECASE),
    ),
    "uart": (
        re.compile(r"\buart\b", re.IGNORECASE),
        re.compile(r"\bserial\b", re.IGNORECASE),
        re.compile(r"\bbaud\b", re.IGNORECASE),
        re.compile(r"\bpacket\b", re.IGNORECASE),
        re.compile(r"\brx\b", re.IGNORECASE),
        re.compile(r"\btx\b", re.IGNORECASE),
    ),
    "gpio": (
        re.compile(r"\bgpio\b", re.IGNORECASE),
        re.compile(r"\b(?:one|single)[-\s]?wire\b", re.IGNORECASE),
        re.compile(r"\b1[-\s]?wire\b", re.IGNORECASE),
        re.compile(r"\bdata\s+(?:line|bus|pin)\b", re.IGNORECASE),
        re.compile(r"\bpulse\b", re.IGNORECASE),
        re.compile(r"\bpulse[-\s]?width\b", re.IGNORECASE),
        re.compile(r"\btiming\b", re.IGNORECASE),
        re.compile(r"\bstart\s+pulse\b", re.IGNORECASE),
        re.compile(r"\bpwm\b", re.IGNORECASE),
        re.compile(r"\becho\b", re.IGNORECASE),
    ),
    "analog": (
        re.compile(r"\banalog\b", re.IGNORECASE),
        re.compile(r"\badc\b", re.IGNORECASE),
        re.compile(r"\bvoltage\s+output\b", re.IGNORECASE),
        re.compile(r"\bvout\b", re.IGNORECASE),
        re.compile(r"\bvo\b", re.IGNORECASE),
    ),
}


def canonicalize_address_rule(device_ir: Any) -> Any:
    """Annotate ``device_ir.address_rule`` with explicit default + form fields."""

    if not isinstance(device_ir, MutableMapping):
        return device_ir
    rule = device_ir.get("address_rule")
    if not isinstance(rule, MutableMapping):
        return device_ir

    addresses = rule.get("addresses")
    if isinstance(addresses, list) and addresses:
        if not _has_any_explicit_default(addresses):
            _annotate_with_explicit_keywords(addresses, rule)
        if not _any_default_marked(addresses):
            _annotate_with_float_strap_pins(addresses, rule)
        if not _any_default_marked(addresses):
            _annotate_with_strap_pin_tie(addresses, rule)
        _prefer_float_strap_default_over_weak_low_default(addresses, rule)
        _ensure_one_default(addresses, rule)

    _ensure_addressing_form(rule, addresses)
    return device_ir


def canonicalize_operation_flows(device_ir: Any) -> Any:
    """Normalise operation-flow bus-step ``op``/``transaction.kind`` mismatches."""

    if not isinstance(device_ir, MutableMapping):
        return device_ir
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return device_ir
    _promote_flow_outputs_to_read_channels(device_ir, flows)
    _promote_register_groups_to_read_channels_and_flow(device_ir, flows)
    _promote_gpio_byte_frame_read_channels_to_flow(device_ir, flows)
    valid_channels = _read_channel_ids(device_ir)

    for flow in flows:
        if not isinstance(flow, MutableMapping):
            continue
        _drop_non_channel_outputs(flow, valid_channels)
        steps = flow.get("steps")
        _canonicalize_no_bus_timing_init_flow(device_ir, flow)
        if not isinstance(steps, list):
            continue
        for step_index, step in enumerate(steps):
            if not isinstance(step, MutableMapping):
                continue
            op = step.get("op")
            transaction = step.get("transaction")
            if _canonicalize_i2c_ack_poll_address_probe(device_ir, flow, steps, step_index):
                continue
            if (
                isinstance(op, str)
                and op in BUS_TRANSACTION_KINDS
                and transaction is None
                and _looks_like_non_bus_abstract_step(step)
            ):
                step["op"] = "postprocess"
                notes = str(step.get("notes") or "").strip()
                suffix = "Canonicalized from a bus op because the step has no bus transaction and is described as API/abstract behavior."
                step["notes"] = f"{notes} {suffix}".strip()
                continue
            if not isinstance(op, str) or not isinstance(transaction, MutableMapping):
                continue
            _canonicalize_i2c_leading_address_payload(device_ir, flow, step, transaction)
            kind = transaction.get("kind")
            if not isinstance(kind, str):
                continue
            if _canonicalize_i2c_memory_device_address_step(device_ir, step, transaction):
                continue
            _canonicalize_i2c_memory_payload_placeholders(device_ir, step, transaction)
            _canonicalize_i2c_noncontiguous_multibyte_read(
                device_ir,
                flow,
                steps,
                step_index,
                step,
                transaction,
            )
            if (
                op == "write_then_read"
                and kind == "write_then_read"
                and transaction.get("bytes") == []
            ):
                step["op"] = "read"
                transaction["kind"] = "read"
                transaction["bytes"] = None
                notes = str(step.get("notes") or "").strip()
                suffix = (
                    "Canonicalized from write_then_read with no pointer/payload "
                    "bytes; the slave address byte is handled by the bus layer."
                )
                step["notes"] = f"{notes} {suffix}".strip()
                continue
            if (
                op in BUS_TRANSACTION_KINDS
                and kind in BUS_TRANSACTION_KINDS
                and op != kind
            ):
                step["op"] = kind

    return device_ir


def canonicalize_conversion_formulae(device_ir: Any) -> Any:
    """Normalise formula rows that cannot be represented as one safe expression."""

    if not isinstance(device_ir, MutableMapping):
        return device_ir
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, list):
        return device_ir
    formula_names = {
        str(row.get("name") or "").strip()
        for row in formulae
        if isinstance(row, Mapping) and str(row.get("name") or "").strip()
    }
    channels_by_formula = _read_channels_by_formula_id(device_ir)
    for row in formulae:
        if not isinstance(row, MutableMapping):
            continue
        _canonicalize_formula_inputs(row, formula_names)
        _canonicalize_scaled_linear_fraction_formula(row)
        _canonicalize_tenths_actual_formula(row)
        _canonicalize_signed_tenths_formula_from_channel(row, channels_by_formula)
        expr_obj = row.get("integer_approximation_expression")
        if not isinstance(expr_obj, MutableMapping):
            continue
        expr = str(expr_obj.get("expression") or "")
        if _formula_expression_is_non_executable(expr, row):
            row["integer_approximation_expression"] = None
            row["executable_expression_status"] = "complex_compensation_algorithm"
            notes = str(row.get("notes") or "").strip()
            suffix = (
                "Canonicalized: exact compensation is preserved in formula/"
                "inputs, but the single-line integer expression was marked "
                "non-executable because it is too complex or not accepted by "
                "the safe expression evaluator."
            )
            row["notes"] = f"{notes} {suffix}".strip()
    _align_read_channel_units_to_formula_outputs(device_ir)
    return device_ir


def canonicalize_gpio_byte_frame_sources(device_ir: Any) -> Any:
    """Restore byte bindings for GPIO/timing protocols that carry byte frames."""

    if not isinstance(device_ir, MutableMapping):
        return device_ir
    if not _looks_like_gpio_byte_frame_device(device_ir):
        return device_ir

    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return device_ir

    source_bytes_by_channel: dict[str, list[str]] = {}
    for channel in channels:
        if not isinstance(channel, MutableMapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        if not channel_id:
            continue
        source_bytes = _source_bytes_list(channel.get("source_bytes"))
        if not source_bytes:
            source_bytes = _infer_gpio_byte_frame_source_bytes(device_ir, channel)
            if source_bytes:
                channel["source_bytes"] = source_bytes
                channel["source_signal"] = None
                _append_canonicalize_note(
                    channel,
                    "Canonicalized GPIO byte-frame binding from extracted "
                    "high/low byte frame evidence.",
                )
        if source_bytes:
            source_bytes_by_channel[channel_id] = source_bytes

    if not source_bytes_by_channel:
        return device_ir

    _bind_gpio_byte_frame_flow_outputs(device_ir, source_bytes_by_channel)
    _bind_gpio_byte_frame_formula_inputs(device_ir, source_bytes_by_channel)
    _align_byte_frame_channel_units_to_formula_outputs(device_ir, source_bytes_by_channel)
    return device_ir


def _canonicalize_formula_inputs(
    row: MutableMapping[str, Any],
    formula_names: set[str],
) -> None:
    expr_obj = row.get("integer_approximation_expression")
    if not isinstance(expr_obj, MutableMapping):
        return
    inputs = expr_obj.get("inputs")
    if not isinstance(inputs, list):
        return
    for item in inputs:
        if not isinstance(item, MutableMapping):
            continue
        if any(item.get(key) for key in ("byte_source", "source_signal", "default_value", "config_source")):
            continue
        name = str(item.get("name") or "").strip()
        if not name:
            continue
        producer = _formula_name_for_intermediate(name, formula_names)
        if producer:
            item["config_source"] = f"output of formula {producer}"


def _read_channels_by_formula_id(device_ir: Mapping[str, Any]) -> dict[str, Mapping[str, Any]]:
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return {}
    out: dict[str, Mapping[str, Any]] = {}
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        formula_id = str(channel.get("formula_id") or "").strip()
        if formula_id and formula_id not in out:
            out[formula_id] = channel
    return out


def _canonicalize_signed_tenths_formula_from_channel(
    row: MutableMapping[str, Any],
    channels_by_formula: Mapping[str, Mapping[str, Any]],
) -> None:
    expr_obj = row.get("integer_approximation_expression")
    if isinstance(expr_obj, MutableMapping) and str(expr_obj.get("expression") or "").strip():
        return
    formula_id = str(row.get("name") or "").strip()
    if not formula_id:
        return
    channel = channels_by_formula.get(formula_id)
    if not isinstance(channel, Mapping):
        return
    source_bytes = _source_bytes_list(channel.get("source_bytes"))
    if len(source_bytes) < 2:
        return
    high, low = source_bytes[0], source_bytes[1]
    if not _is_identifier(high) or not _is_identifier(low):
        return
    text = " ".join(_flow_strings_for_canonicalize(row) + _flow_strings_for_canonicalize(channel)).lower()
    if not _looks_like_signed_tenths_temperature_formula(text):
        return
    unit = str(channel.get("physical_unit") or "").strip()
    output_unit = unit if _unit_has_integer_scale(unit) else _tenths_scaled_unit(unit)
    if not output_unit:
        output_unit = "milli_degC"
    row["integer_approximation_expression"] = {
        "expression": f"(({high} & 127) * 256 + {low}) * 100 * (1 - 2 * (({high} >> 7) & 1))",
        "inputs": [
            {
                "name": high,
                "byte_source": f"{high}:8",
                "source_signal": None,
                "default_value": None,
                "config_source": None,
                "description": "High byte; bit 7 is the sign bit for signed-magnitude x10 value.",
            },
            {
                "name": low,
                "byte_source": f"{low}:8",
                "source_signal": None,
                "default_value": None,
                "config_source": None,
                "description": "Low byte of signed-magnitude x10 value.",
            },
        ],
        "output": {
            "name": str(channel.get("id") or formula_id).strip() or formula_id,
            "unit": output_unit,
        },
    }
    row.pop("executable_expression_status", None)
    _append_canonicalize_note(
        row,
        (
            "Canonicalized signed-magnitude x10 formula from read_channel "
            "high/low source bytes into an executable milli-unit expression."
        ),
    )


def _is_identifier(value: str) -> bool:
    return bool(re.match(r"^[A-Za-z_]\w*$", value or ""))


def _looks_like_signed_tenths_temperature_formula(text: str) -> bool:
    has_temperature = bool(re.search(r"\b(temp(?:erature)?|degc|degree|celsius|°\s*c)\b", text))
    has_sign_bit = bool(re.search(r"\b(bit\s*15|sign\s*bit|0x8000|0x7fff|0x80)\b", text))
    has_tenths = bool(
        re.search(r"\b10\s*times\s+(?:the\s+)?actual\b", text)
        or re.search(r"\btimes\s*10\b", text)
        or re.search(r"\bx\s*10\b", text)
        or re.search(r"/\s*10(?:\.0)?\b", text)
        or re.search(r"\btenths?\b", text)
    )
    return has_temperature and has_sign_bit and has_tenths


_LINEAR_FORMULA_TERM_RE = re.compile(
    r"(?P<coef>[+-]?\d+(?:\.\d+)?)\s*\*\s*(?P<var>[A-Za-z_]\w*)\s*/",
    re.IGNORECASE,
)
_POWER_DENOM_RE = re.compile(r"2\s*(?:\^|\*\*)\s*(?P<bits>\d+)\s*-\s*1", re.IGNORECASE)
_NUMBER_RE = re.compile(r"[+-]?\d+(?:\.\d+)?")


def _canonicalize_scaled_linear_fraction_formula(row: MutableMapping[str, Any]) -> None:
    """_canonicalize_scaled_linear_fraction_formula helper."""

    expr_obj = row.get("integer_approximation_expression")
    if not isinstance(expr_obj, MutableMapping):
        return
    output = expr_obj.get("output")
    unit = ""
    if isinstance(output, Mapping):
        unit = str(output.get("unit") or "")
    scale = _scaled_formula_unit_multiplier(unit)
    if scale == 1:
        return
    parsed = _parse_linear_fraction_formula(str(row.get("formula") or ""))
    if parsed is None:
        return
    var, coef, offset, denom = parsed
    if not _formula_declares_input(expr_obj, var):
        return
    try:
        coef_scaled = coef * Decimal(scale)
        offset_scaled = offset * Decimal(scale)
    except InvalidOperation:
        return
    if coef_scaled != coef_scaled.to_integral_value():
        return
    if offset_scaled != offset_scaled.to_integral_value():
        return

    coef_i = int(coef_scaled)
    offset_i = int(offset_scaled)
    new_expr = f"(({var} * {coef_i}) // {denom})"
    if offset_i < 0:
        new_expr += f" - {abs(offset_i)}"
    elif offset_i > 0:
        new_expr += f" + {offset_i}"

    if str(expr_obj.get("expression") or "").replace(" ", "") == new_expr.replace(" ", ""):
        return
    expr_obj["expression"] = new_expr
    row.pop("executable_expression_status", None)
    notes = str(row.get("notes") or "").strip()
    suffix = (
        "Canonicalized executable expression from the datasheet linear "
        f"fraction formula and scaled output unit {unit!r}."
    )
    row["notes"] = f"{notes} {suffix}".strip()


def _canonicalize_tenths_actual_formula(row: MutableMapping[str, Any]) -> None:
    """_canonicalize_tenths_actual_formula helper."""

    expr_obj = row.get("integer_approximation_expression")
    if not isinstance(expr_obj, MutableMapping):
        return
    output = expr_obj.get("output")
    if not isinstance(output, MutableMapping):
        return
    unit = str(output.get("unit") or "").strip()
    scaled_unit = _tenths_scaled_unit(unit)
    if not scaled_unit:
        return
    text = " ".join(_flow_strings_for_canonicalize(row)).lower()
    expr = str(expr_obj.get("expression") or "").strip()
    if not expr:
        return
    if not _looks_like_tenths_actual_formula(text, expr):
        return
    rewritten = _rewrite_divide_by_ten_expression_to_milli(expr)
    if not rewritten or rewritten == expr:
        return
    expr_obj["expression"] = rewritten
    output["unit"] = scaled_unit
    row.pop("executable_expression_status", None)
    notes = str(row.get("notes") or "").strip()
    suffix = (
        "Canonicalized x10 raw-value formula to a milli-unit executable "
        f"expression ({unit!r} -> {scaled_unit!r}) without truncating the "
        "tenths digit."
    )
    row["notes"] = f"{notes} {suffix}".strip()


def _scaled_formula_unit_multiplier(unit: str) -> int:
    lowered = str(unit or "").lower()
    if "micro" in lowered:
        return 1_000_000
    if "milli" in lowered:
        return 1_000
    return 1


def _parse_linear_fraction_formula(formula: str) -> tuple[str, Decimal, Decimal, int] | None:
    if "=" not in formula:
        return None
    rhs = formula.split("=", 1)[1]
    term = _LINEAR_FORMULA_TERM_RE.search(rhs)
    if term is None:
        return None
    try:
        coef = Decimal(term.group("coef").replace(" ", ""))
    except InvalidOperation:
        return None
    var = term.group("var")
    before = rhs[:term.start()].strip()
    offset = Decimal(0)
    if before:
        offset_match = re.search(
            r"(?P<offset>[+-]?\s*\d+(?:\.\d+)?)\s*\+\s*$",
            before,
        )
        if offset_match is None:
            return None
        try:
            offset = Decimal(offset_match.group("offset").replace(" ", ""))
        except InvalidOperation:
            return None

    after_slash = rhs[term.end():]
    denom = _parse_linear_fraction_denominator(after_slash)
    if denom is None or denom <= 0:
        return None
    return var, coef, offset, denom


def _parse_linear_fraction_denominator(text: str) -> int | None:
    power = _POWER_DENOM_RE.search(text)
    if power is not None:
        bits = int(power.group("bits"))
        if 1 <= bits <= 63:
            return (1 << bits) - 1
    number = _NUMBER_RE.search(text)
    if number is None:
        return None
    try:
        dec = Decimal(number.group(0))
    except InvalidOperation:
        return None
    if dec != dec.to_integral_value():
        return None
    return int(dec)


def _formula_declares_input(expr_obj: Mapping[str, Any], var: str) -> bool:
    inputs = expr_obj.get("inputs")
    if not isinstance(inputs, list):
        return False
    return any(
        isinstance(item, Mapping)
        and str(item.get("name") or "").strip() == var
        for item in inputs
    )


def _formula_name_for_intermediate(name: str, formula_names: set[str]) -> str:
    norm = re.sub(r"[^a-z0-9]", "", name.lower())
    if norm == "tfine":
        for candidate in ("compensate_temperature", "temperature_compensation"):
            if candidate in formula_names:
                return candidate
        for candidate in formula_names:
            if "temp" in candidate.lower():
                return candidate
    return ""


def _formula_expression_is_non_executable(
    expr: str,
    row: Mapping[str, Any],
) -> bool:
    if not expr.strip():
        return False
    text = " ".join(_flow_strings_for_canonicalize(row)).lower()
    try:
        parsed_expr = ast.parse(expr, mode="eval")
    except SyntaxError:
        return True
    if _formula_expression_has_undeclared_names(parsed_expr, row):
        return True
    if len(expr) > 700:
        return True
    if re.search(r"\b(?:var\d+|v_x\d+|v_x1_u32r)\b", expr):
        return True
    if re.search(r"\b\d+\.\d+\b", expr):
        return True
    if re.search(r"\((?:u?int\d+_t|signed|unsigned|long|short|float|double)\b", expr):
        return True
    if "?" in expr or ":" in expr:
        return True
    if (
        "too complex for single-line safe evaluator" in text
        or "complex compensation" in text
        or "vendor compensation" in text
    ):
        return True
    if _formula_expression_likely_bad_milli_temperature_byte_pack(expr, row):
        return True
    return False


def _formula_expression_has_undeclared_names(
    parsed_expr: ast.Expression,
    row: Mapping[str, Any],
) -> bool:
    expr_obj = row.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return False
    inputs = expr_obj.get("inputs")
    declared: set[str] = set()
    if isinstance(inputs, list):
        for item in inputs:
            if isinstance(item, Mapping) and isinstance(item.get("name"), str):
                name = item["name"].strip()
                if name:
                    declared.add(name)
    if not declared:
        return False
    allowed_names = declared | {"abs", "min", "max", "round"}
    used = {
        node.id
        for node in ast.walk(parsed_expr)
        if isinstance(node, ast.Name)
    }
    return bool(used - allowed_names)


def _formula_expression_likely_bad_milli_temperature_byte_pack(
    expr: str,
    row: Mapping[str, Any],
) -> bool:
    expr_norm = re.sub(r"\s+", "", expr.lower())
    row_text = " ".join(_flow_strings_for_canonicalize(row)).lower()
    expr_obj = row.get("integer_approximation_expression")
    unit = ""
    if isinstance(expr_obj, Mapping):
        output = expr_obj.get("output")
        if isinstance(output, Mapping) and isinstance(output.get("unit"), str):
            unit = output["unit"].lower()
    if "milli" not in unit and "mdeg" not in unit:
        return False
    if "degc" not in unit and "temperature" not in row_text and "temp" not in row_text:
        return False
    if "0.0625" in row_text and "high_byte<<8" in expr_norm and (
        "*625//10" in expr_norm or "*625/10" in expr_norm
    ):
        return True
    if (
        ("two's complement" in row_text or "twos complement" in row_text or "two s complement" in row_text)
        and re.search(r"&0x[0-9a-f]+", expr_norm)
        and re.search(r"\*625//10|\*625/10|\*62(?:5)?", expr_norm)
        and not re.search(r"-4096|0x800|sign", expr_norm)
    ):
        return True
    if (
        ("two's complement" in row_text or "twos complement" in row_text or "two s complement" in row_text)
        and re.search(r"&0x[0-9a-f]+", expr_norm)
        and re.search(r"\*2(?:50|000|0000)", expr_norm)
    ):
        return True
    return False


def _tenths_scaled_unit(unit: str) -> str:
    norm = _normalize_unit_name(unit)
    if not norm or _unit_has_integer_scale(unit):
        return ""
    if "percent" in norm or norm.endswith("rh") or "humidity" in norm:
        return "milli_percent_rh" if "rh" in norm or "humidity" in norm else "milli_percent"
    if "degc" in norm or "celsius" in norm or norm in {"c", "degreec"}:
        return "milli_degC"
    return ""


def _looks_like_tenths_actual_formula(text: str, expr: str) -> bool:
    expr_norm = re.sub(r"\s+", "", expr.lower())
    if not any(token in expr_norm for token in ("//10", "/10", "*10", "10*")):
        return False
    return bool(
        re.search(r"\b10\s*times\s+(?:the\s+)?actual\b", text)
        or re.search(r"\btimes\s*10\b", text)
        or re.search(r"\bx\s*10\b", text)
        or re.search(r"\btenths?\b", text)
        or re.search(r"\b0\.1\s*(?:deg|degree|%|percent)", text)
        or re.search(r"/\s*10(?:\.0)?\b", text)
    )


def _rewrite_divide_by_ten_expression_to_milli(expr: str) -> str:
    try:
        parsed = ast.parse(expr, mode="eval")
    except SyntaxError:
        return ""

    class _TenthsToMilli(ast.NodeTransformer):
        changed = False

        def visit_BinOp(self, node: ast.BinOp):  # type: ignore[override]
            node = self.generic_visit(node)
            if isinstance(node.op, (ast.Div, ast.FloorDiv)) and _ast_constant_int(node.right) == 10:
                self.changed = True
                return ast.copy_location(
                    ast.BinOp(left=node.left, op=ast.Mult(), right=ast.Constant(value=100)),
                    node,
                )
            if isinstance(node.op, ast.Mult) and _ast_constant_int(node.right) == 10:
                self.changed = True
                return ast.copy_location(
                    ast.BinOp(left=node.left, op=ast.Mult(), right=ast.Constant(value=100)),
                    node,
                )
            if isinstance(node.op, ast.Mult) and _ast_constant_int(node.left) == 10:
                self.changed = True
                return ast.copy_location(
                    ast.BinOp(left=ast.Constant(value=100), op=ast.Mult(), right=node.right),
                    node,
                )
            return node

    transformer = _TenthsToMilli()
    rewritten = transformer.visit(parsed)
    if not transformer.changed:
        return ""
    ast.fix_missing_locations(rewritten)
    try:
        return ast.unparse(rewritten.body)  # type: ignore[attr-defined]
    except Exception:
        return ""


def _ast_constant_int(node: ast.AST) -> int | None:
    if isinstance(node, ast.Constant) and isinstance(node.value, int):
        return node.value
    return None


_GPIO_BYTE_FRAME_MARKER_RE = re.compile(
    r"\b(?:byte|bytes|octet|frame|payload|scratchpad|checksum|crc|msb|lsb)\b|"
    r"\b\d+\s*[- ]?\s*bit\s+(?:data|frame|payload|response|scratchpad)\b",
    re.IGNORECASE,
)
_HIGH_LOW_BYTE_MARKER_RE = re.compile(
    r"\b(?:high|hi|msb|most\s+significant|low|lo|lsb|least\s+significant)\b"
    r".{0,24}\bbyte\b|\bbyte\b.{0,24}"
    r"\b(?:high|hi|msb|most\s+significant|low|lo|lsb|least\s+significant)\b",
    re.IGNORECASE,
)
_BYTE_FRAME_ROLE_TOKENS: dict[str, tuple[str, ...]] = {
    "high": ("high", "hi", "msb", "mostsignificant"),
    "low": ("low", "lo", "lsb", "leastsignificant"),
}
_BYTE_FRAME_CHANNEL_ALIASES: dict[str, tuple[str, ...]] = {
    "temperature": ("temperature", "temp", "t"),
    "humidity": ("humidity", "relative humidity", "rh", "hum"),
    "pressure": ("pressure", "press", "pres", "p"),
    "distance": ("distance", "range", "dist"),
    "light": ("light", "lux", "illumination", "illuminance"),
    "co2": ("co2", "carbon dioxide", "ppm"),
}


def _looks_like_gpio_byte_frame_device(device_ir: Mapping[str, Any]) -> bool:
    bus_family = _canonical_bus_family(device_ir.get("bus_type"))
    access_model = device_ir.get("access_model")
    access_kind = ""
    if isinstance(access_model, Mapping):
        access_kind = str(access_model.get("kind") or "").lower()
    if bus_family != "gpio" and "gpio" not in access_kind and "one" not in access_kind:
        return False

    text = " ".join(_flow_strings_for_canonicalize(device_ir))
    if not _GPIO_BYTE_FRAME_MARKER_RE.search(text):
        return False
    return (
        bool(_HIGH_LOW_BYTE_MARKER_RE.search(text))
        or "checksum" in text.lower()
        or "crc" in text.lower()
        or "scratchpad" in text.lower()
        or bool(re.search(r"\b\d+\s*[- ]?\s*bit\s+(?:data|frame|payload|response)\b", text, re.IGNORECASE))
    )


def _source_bytes_list(value: Any) -> list[str]:
    if not isinstance(value, list):
        return []
    out: list[str] = []
    for item in value:
        if isinstance(item, str) and item.strip():
            out.append(item.strip())
    return out


def _infer_gpio_byte_frame_source_bytes(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
) -> list[str]:
    channel_id = str(channel.get("id") or "").strip()
    if not channel_id:
        return []
    aliases = _byte_frame_aliases_for_channel(channel_id)
    formula_id = str(channel.get("formula_id") or "").strip()

    from_existing = _source_bytes_from_existing_byte_sources(device_ir, channel_id, formula_id, aliases)
    if from_existing:
        return from_existing

    from_named_inputs = _source_bytes_from_named_inputs(device_ir, channel, aliases)
    if from_named_inputs:
        return from_named_inputs

    from_text = _source_bytes_from_byte_frame_text(device_ir, channel, aliases)
    if from_text:
        return from_text

    return []


def _source_bytes_from_existing_byte_sources(
    device_ir: Mapping[str, Any],
    channel_id: str,
    formula_id: str,
    aliases: Sequence[str],
) -> list[str]:
    flows = device_ir.get("operation_flows")
    if isinstance(flows, list):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            outputs = flow.get("outputs")
            if not isinstance(outputs, list):
                continue
            for output in outputs:
                if not isinstance(output, Mapping):
                    continue
                if str(output.get("channel") or "").strip() != channel_id:
                    continue
                tokens = _byte_source_register_tokens(output.get("byte_source"))
                if _tokens_are_channel_byte_pair(tokens, aliases):
                    return tokens[:2]

    formulae = device_ir.get("conversion_formulae")
    if isinstance(formulae, list):
        for row in formulae:
            if not isinstance(row, Mapping):
                continue
            if formula_id and str(row.get("name") or "").strip() != formula_id:
                continue
            for item in _formula_expression_inputs(row):
                tokens = _byte_source_register_tokens(item.get("byte_source"))
                if _tokens_are_channel_byte_pair(tokens, aliases):
                    return tokens[:2]
    return []


def _source_bytes_from_named_inputs(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
    aliases: Sequence[str],
) -> list[str]:
    high_name = ""
    low_name = ""
    for row in _formula_rows(device_ir):
        for item in _formula_expression_inputs(row):
            name = str(item.get("name") or "").strip()
            if not name or not _token_matches_any_alias(name, aliases):
                continue
            role = _byte_frame_role(name)
            if role == "high" and not high_name:
                high_name = name
            elif role == "low" and not low_name:
                low_name = name
    if high_name and low_name:
        if _raw_byte_order_for_channel(device_ir, channel) == "little":
            return [low_name, high_name]
        return [high_name, low_name]
    return []


def _source_bytes_from_byte_frame_text(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
    aliases: Sequence[str],
) -> list[str]:
    text = _byte_frame_context_for_channel(device_ir, channel)
    if not text:
        return []
    high_pos = _find_alias_byte_role_position(text, aliases, role="high")
    low_pos = _find_alias_byte_role_position(text, aliases, role="low")
    if high_pos is None or low_pos is None:
        return []

    base = _byte_frame_source_base(str(channel.get("id") or ""))
    high_name = f"{base}_high"
    low_name = f"{base}_low"
    if high_pos <= low_pos:
        return [high_name, low_name]
    if _raw_byte_order_for_channel(device_ir, channel) == "big":
        return [high_name, low_name]
    return [low_name, high_name]


def _byte_frame_context_for_channel(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
) -> str:
    pieces: list[str] = []
    pieces.extend(_flow_strings_for_canonicalize(channel))
    channel_id = str(channel.get("id") or "").strip()
    formula_id = str(channel.get("formula_id") or "").strip()
    for row in _formula_rows(device_ir):
        row_name = str(row.get("name") or "").strip()
        row_text = " ".join(_flow_strings_for_canonicalize(row))
        if formula_id and row_name == formula_id:
            pieces.append(row_text)
        elif _token_matches_any_alias(row_text, _byte_frame_aliases_for_channel(channel_id)):
            pieces.append(row_text)
    flows = device_ir.get("operation_flows")
    if isinstance(flows, list):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            outputs = flow.get("outputs")
            if not isinstance(outputs, list):
                continue
            for output in outputs:
                if isinstance(output, Mapping) and str(output.get("channel") or "").strip() == channel_id:
                    pieces.extend(_flow_strings_for_canonicalize(output))
    return " ".join(pieces)


def _find_alias_byte_role_position(
    text: str,
    aliases: Sequence[str],
    *,
    role: str,
) -> int | None:
    role_words = _BYTE_FRAME_ROLE_TOKENS[role]
    lowered = text.lower()
    best: int | None = None
    for alias in aliases:
        alias_pattern = re.escape(alias.lower()).replace(r"\ ", r"\s+")
        for role_word in role_words:
            role_pattern = re.escape(role_word).replace(r"\ ", r"\s+")
            patterns = (
                rf"\b{alias_pattern}\b.{{0,64}}\b{role_pattern}\b.{{0,32}}\bbyte\b",
                rf"\b{role_pattern}\b.{{0,32}}\b{alias_pattern}\b.{{0,32}}\bbyte\b",
                rf"\b{role_pattern}\b.{{0,32}}\bbyte\b.{{0,64}}\b{alias_pattern}\b",
            )
            for pattern in patterns:
                match = re.search(pattern, lowered, flags=re.IGNORECASE)
                if match is not None:
                    best = match.start() if best is None else min(best, match.start())
    return best


def _byte_frame_aliases_for_channel(channel_id: str) -> tuple[str, ...]:
    lowered = channel_id.lower().strip()
    norm = re.sub(r"[^a-z0-9]+", "", lowered)
    aliases: list[str] = [lowered.replace("_", " ")]
    for key, values in _BYTE_FRAME_CHANNEL_ALIASES.items():
        if key in norm or any(re.sub(r"[^a-z0-9]+", "", value) in norm for value in values):
            aliases.extend(values)
    aliases.append(channel_id)

    out: list[str] = []
    seen: set[str] = set()
    for alias in aliases:
        alias = str(alias or "").strip().lower()
        if alias and alias not in seen:
            seen.add(alias)
            out.append(alias)
    return tuple(out)


def _byte_frame_source_base(channel_id: str) -> str:
    norm = re.sub(r"[^a-z0-9]+", "_", channel_id.lower()).strip("_")
    collapsed = re.sub(r"[^a-z0-9]+", "", channel_id.lower())
    if "humid" in collapsed or collapsed in {"rh", "relativehumidity"}:
        return "humidity"
    if "temp" in collapsed:
        return "temperature"
    if "press" in collapsed:
        return "pressure"
    if "dist" in collapsed or "range" in collapsed:
        return "distance"
    if "lux" in collapsed or "light" in collapsed or "illum" in collapsed:
        return "light"
    return norm or "value"


def _tokens_are_channel_byte_pair(tokens: Sequence[str], aliases: Sequence[str]) -> bool:
    if len(tokens) < 2:
        return False
    first_two = list(tokens[:2])
    roles = {_byte_frame_role(token) for token in first_two}
    if not {"high", "low"}.issubset(roles):
        return False
    return any(_token_matches_any_alias(token, aliases) for token in first_two)


def _byte_frame_role(value: str) -> str:
    norm = re.sub(r"[^a-z0-9]+", "", str(value or "").lower())
    for role, tokens in _BYTE_FRAME_ROLE_TOKENS.items():
        if any(token in norm for token in tokens):
            return role
    return ""


def _token_matches_any_alias(value: str, aliases: Sequence[str]) -> bool:
    norm_value = re.sub(r"[^a-z0-9]+", "", str(value or "").lower())
    if not norm_value:
        return False
    for alias in aliases:
        norm_alias = re.sub(r"[^a-z0-9]+", "", alias.lower())
        if not norm_alias:
            continue
        if norm_alias == "t":
            if re.search(r"(?:^|[^a-z])t(?:[^a-z]|$)", str(value or "").lower()):
                return True
            continue
        if norm_alias == "p":
            if re.search(r"(?:^|[^a-z])p(?:[^a-z]|$)", str(value or "").lower()):
                return True
            continue
        if norm_alias in norm_value or norm_value in norm_alias:
            return True
    return False


def _raw_byte_order_for_channel(device_ir: Mapping[str, Any], channel: Mapping[str, Any]) -> str:
    text_parts: list[str] = []
    text_parts.extend(_flow_strings_for_canonicalize(channel))
    raw_encoding = device_ir.get("raw_encoding")
    if isinstance(raw_encoding, Mapping):
        text_parts.extend(_flow_strings_for_canonicalize(raw_encoding))
    elif isinstance(raw_encoding, list):
        for item in raw_encoding:
            if isinstance(item, Mapping):
                text_parts.extend(_flow_strings_for_canonicalize(item))
            elif isinstance(item, str):
                text_parts.append(item)
    text = " ".join(text_parts).lower()
    if "little" in text or "lsb first" in text or "least significant byte first" in text:
        return "little"
    if "big" in text or "msb first" in text or "most significant byte first" in text:
        return "big"
    return ""


def _bind_gpio_byte_frame_flow_outputs(
    device_ir: MutableMapping[str, Any],
    source_bytes_by_channel: Mapping[str, Sequence[str]],
) -> None:
    channels = _read_channel_map(device_ir)
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list):
        return
    for flow in flows:
        if not isinstance(flow, MutableMapping):
            continue
        outputs = flow.get("outputs")
        if not isinstance(outputs, list):
            continue
        for output in outputs:
            if not isinstance(output, MutableMapping):
                continue
            channel_id = str(output.get("channel") or "").strip()
            source_bytes = source_bytes_by_channel.get(channel_id)
            if not source_bytes:
                continue
            if not isinstance(output.get("byte_source"), str) or not str(output.get("byte_source")).strip():
                output["byte_source"] = _byte_source_expression(source_bytes)
                _append_canonicalize_note(
                    output,
                    "Canonicalized byte_source from read_channel.source_bytes.",
                )
            output["source_signal"] = None
            channel = channels.get(channel_id)
            if channel is not None and not output.get("formula_id") and channel.get("formula_id"):
                output["formula_id"] = channel.get("formula_id")


def _bind_gpio_byte_frame_formula_inputs(
    device_ir: MutableMapping[str, Any],
    source_bytes_by_channel: Mapping[str, Sequence[str]],
) -> None:
    channels = _read_channel_map(device_ir)
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, list):
        return

    for row in formulae:
        if not isinstance(row, MutableMapping):
            continue
        row_name = str(row.get("name") or "").strip()
        for channel_id, channel in channels.items():
            if row_name != str(channel.get("formula_id") or "").strip():
                continue
            source_bytes = source_bytes_by_channel.get(channel_id)
            if not source_bytes:
                continue
            aliases = _byte_frame_aliases_for_channel(channel_id)
            for item in _formula_expression_inputs(row):
                if not isinstance(item, MutableMapping):
                    continue
                if _formula_input_belongs_to_byte_frame_channel(item, row, aliases):
                    item["byte_source"] = _byte_source_expression_for_formula_input(
                        item,
                        source_bytes,
                        aliases,
                    )
                    item["source_signal"] = None
                    _append_canonicalize_note(
                        item,
                        "Canonicalized byte_source from GPIO byte-frame channel binding.",
                    )

        if _row_looks_like_integrity_formula(row):
            for item in _formula_expression_inputs(row):
                if not isinstance(item, MutableMapping):
                    continue
                name = str(item.get("name") or "").strip()
                if not name or not _byte_frame_role(name):
                    continue
                if not isinstance(item.get("byte_source"), str) or not str(item.get("byte_source")).strip():
                    item["byte_source"] = f"{name}:8"
                    item["source_signal"] = None


def _formula_input_belongs_to_byte_frame_channel(
    item: Mapping[str, Any],
    row: Mapping[str, Any],
    aliases: Sequence[str],
) -> bool:
    name = str(item.get("name") or "")
    if _token_matches_any_alias(name, aliases):
        return True
    row_text = " ".join(_flow_strings_for_canonicalize(row))
    if not _token_matches_any_alias(row_text, aliases):
        return False
    name_norm = re.sub(r"[^a-z0-9]+", "", name.lower())
    if name_norm.startswith("raw") or "count" in name_norm or "value" in name_norm:
        return True
    source_signal = str(item.get("source_signal") or "")
    return bool(source_signal and ("bit" in source_signal.lower() or "data" in source_signal.lower()))


def _byte_source_expression_for_formula_input(
    item: Mapping[str, Any],
    source_bytes: Sequence[str],
    aliases: Sequence[str],
) -> str:
    name = str(item.get("name") or "").strip()
    role = _byte_frame_role(name)
    if role and len(source_bytes) >= 2:
        role_index = 0 if role == "high" else 1
        source = str(source_bytes[role_index] or "").strip()
        if source and (
            _byte_frame_role(source) == role
            or _token_matches_any_alias(name, aliases)
            or _token_matches_any_alias(source, aliases)
        ):
            return f"{source}:8"
    return _byte_source_expression(source_bytes)


def _align_byte_frame_channel_units_to_formula_outputs(
    device_ir: MutableMapping[str, Any],
    source_bytes_by_channel: Mapping[str, Sequence[str]],
) -> None:
    _align_read_channel_units_to_formula_outputs(
        device_ir,
        channel_ids=set(source_bytes_by_channel),
    )


def _align_read_channel_units_to_formula_outputs(
    device_ir: MutableMapping[str, Any],
    *,
    channel_ids: set[str] | None = None,
) -> None:
    formula_units = _formula_output_units(device_ir)
    flows = device_ir.get("operation_flows")
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return
    for channel in channels:
        if not isinstance(channel, MutableMapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        if channel_ids is not None and channel_id not in channel_ids:
            continue
        formula_id = str(channel.get("formula_id") or "").strip()
        formula_unit = formula_units.get(formula_id)
        if not formula_unit:
            continue
        current_unit = str(channel.get("physical_unit") or "").strip()
        if current_unit == formula_unit:
            continue
        if not _units_are_scaled_equivalent(current_unit, formula_unit):
            continue
        channel["physical_unit"] = formula_unit
        _append_canonicalize_note(
            channel,
            f"Canonicalized physical_unit to formula output unit {formula_unit!r}.",
        )
        if isinstance(flows, list):
            for flow in flows:
                if not isinstance(flow, MutableMapping):
                    continue
                outputs = flow.get("outputs")
                if not isinstance(outputs, list):
                    continue
                for output in outputs:
                    if not isinstance(output, MutableMapping):
                        continue
                    if str(output.get("channel") or "").strip() != channel_id:
                        continue
                    output_unit = str(output.get("unit") or "").strip()
                    if not output_unit or output_unit == current_unit or _units_are_scaled_equivalent(output_unit, formula_unit):
                        output["unit"] = formula_unit


def _formula_output_units(device_ir: Mapping[str, Any]) -> dict[str, str]:
    out: dict[str, str] = {}
    for row in _formula_rows(device_ir):
        name = str(row.get("name") or "").strip()
        expr_obj = row.get("integer_approximation_expression")
        if not name or not isinstance(expr_obj, Mapping):
            continue
        output = expr_obj.get("output")
        if isinstance(output, Mapping):
            unit = str(output.get("unit") or "").strip()
            if unit:
                out[name] = unit
    return out


def _units_are_scaled_equivalent(left: str, right: str) -> bool:
    if not left or not right:
        return False
    if _normalize_unit_name(left) == _normalize_unit_name(right):
        return True
    if not _unit_has_integer_scale(right):
        return False
    return _unit_dimension(left) != "" and _unit_dimension(left) == _unit_dimension(right)


def _normalize_unit_name(unit: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", unit.lower())


def _unit_has_integer_scale(unit: str) -> bool:
    norm = _normalize_unit_name(unit)
    return norm.startswith("milli") or norm.startswith("micro") or norm.startswith("mdeg")


def _unit_dimension(unit: str) -> str:
    norm = _normalize_unit_name(unit)
    if not norm:
        return ""
    if "percent" in norm or norm.endswith("rh") or "humidity" in norm:
        return "relative_humidity"
    if "degc" in norm or "celsius" in norm or "temperature" in norm:
        return "temperature"
    if "pascal" in norm or norm == "pa" or norm.endswith("pa"):
        return "pressure"
    if "lux" in norm or "light" in norm or "illum" in norm:
        return "light"
    if "ppm" in norm or "co2" in norm:
        return "concentration"
    if "meter" in norm or "metre" in norm or norm.endswith("mm") or norm.endswith("cm"):
        return "distance"
    return norm


def _read_channel_map(device_ir: Mapping[str, Any]) -> dict[str, MutableMapping[str, Any]]:
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return {}
    out: dict[str, MutableMapping[str, Any]] = {}
    for channel in channels:
        if not isinstance(channel, MutableMapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        if channel_id:
            out[channel_id] = channel
    return out


def _formula_rows(device_ir: Mapping[str, Any]) -> list[Mapping[str, Any]]:
    rows = device_ir.get("conversion_formulae")
    if not isinstance(rows, list):
        return []
    return [row for row in rows if isinstance(row, Mapping)]


def _formula_expression_inputs(row: Mapping[str, Any]) -> list[Any]:
    expr_obj = row.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return []
    inputs = expr_obj.get("inputs")
    if not isinstance(inputs, list):
        return []
    return inputs


def _row_looks_like_integrity_formula(row: Mapping[str, Any]) -> bool:
    text = " ".join(_flow_strings_for_canonicalize(row)).lower()
    return "checksum" in text or "crc" in text or "pec" in text or "parity" in text


def _byte_source_expression(source_bytes: Sequence[str]) -> str:
    return " || ".join(f"{name}:8" for name in source_bytes if str(name).strip())


def _append_canonicalize_note(target: MutableMapping[str, Any], suffix: str) -> None:
    notes = str(target.get("notes") or "").strip()
    if suffix in notes:
        return
    target["notes"] = f"{notes} {suffix}".strip()


def _promote_gpio_byte_frame_read_channels_to_flow(
    device_ir: MutableMapping[str, Any],
    flows: list[Any],
) -> None:
    if not _looks_like_gpio_byte_frame_device(device_ir):
        return
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return
    existing_flow_ids = {
        str(flow.get("flow_id") or "").strip()
        for flow in flows
        if isinstance(flow, Mapping) and str(flow.get("flow_id") or "").strip()
    }
    grouped: dict[str, list[MutableMapping[str, Any]]] = {}
    for channel in channels:
        if not isinstance(channel, MutableMapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        if not channel_id:
            continue
        source_bytes = _source_bytes_list(channel.get("source_bytes"))
        if not source_bytes:
            continue
        flow_id = str(channel.get("flow_id") or "").strip()
        if not flow_id:
            flow_id = "read_sensor"
            channel["flow_id"] = flow_id
            _append_canonicalize_note(
                channel,
                "Canonicalized missing flow_id to synthetic GPIO byte-frame read flow.",
            )
        if flow_id in existing_flow_ids:
            continue
        grouped.setdefault(flow_id, []).append(channel)

    for flow_id, grouped_channels in grouped.items():
        if not grouped_channels:
            continue
        outputs: list[dict[str, Any]] = []
        channel_ids: list[str] = []
        for channel in grouped_channels:
            channel_id = str(channel.get("id") or "").strip()
            if not channel_id:
                continue
            channel_ids.append(channel_id)
            source_bytes = _source_bytes_list(channel.get("source_bytes"))
            output: dict[str, Any] = {
                "channel": channel_id,
                "byte_source": _byte_source_expression(source_bytes),
                "source_signal": None,
                "notes": (
                    "Canonicalized from read_channel.source_bytes because the "
                    "extractor omitted the GPIO byte-frame read flow."
                ),
            }
            formula_id = str(channel.get("formula_id") or "").strip()
            if formula_id:
                output["formula_id"] = formula_id
            unit = str(channel.get("physical_unit") or "").strip()
            if unit:
                output["unit"] = unit
            outputs.append(output)
        if not channel_ids or not outputs:
            continue
        flows.append(
            {
                "flow_id": flow_id,
                "kind": "read",
                "channels": channel_ids,
                "steps": [
                    {
                        "op": "set_signal",
                        "signal": "data",
                        "condition": "host start request",
                        "notes": (
                            "Host drives the data line for the device-specific "
                            "start pulse described by timing_constraints."
                        ),
                    },
                    {
                        "op": "wait_signal",
                        "signal": "data",
                        "condition": "sensor response and bit preamble",
                        "notes": (
                            "Wait for the sensor response edges before sampling "
                            "the timed byte frame."
                        ),
                    },
                    {
                        "op": "sample_signal",
                        "source_signal": "data",
                        "notes": (
                            "Sample the timed GPIO byte frame; source_bytes and "
                            "checksum/error_conditions define byte order and validity."
                        ),
                    },
                ],
                "outputs": outputs,
                "requires_human": False,
                "notes": (
                    "Canonicalized GPIO byte-frame read flow from read_channels "
                    "because the extractor omitted operation_flows coverage."
                ),
            }
        )
        existing_flow_ids.add(flow_id)


def _canonicalize_no_bus_timing_init_flow(
    device_ir: Mapping[str, Any],
    flow: MutableMapping[str, Any],
) -> None:
    if str(flow.get("kind") or "").strip().lower() != "init":
        return
    if flow.get("requires_human") is True:
        return
    if not _device_uses_no_register_timing_access(device_ir):
        return
    if _flow_has_nonempty_list(flow.get("channels")) or _flow_has_nonempty_list(flow.get("outputs")):
        return
    if _flow_has_producer_step(flow):
        return
    if not _flow_has_non_bus_init_evidence(flow):
        return
    _append_canonicalize_note(
        flow,
        (
            "Canonicalized as no bus initialization: only power-up/default-state "
            "settling or host-side setup is required; configuration writes are "
            "not required."
        ),
    )


def _device_uses_no_register_timing_access(device_ir: Mapping[str, Any]) -> bool:
    bus_type = _normalize_access_token(str(device_ir.get("bus_type") or ""))
    bus_family = _canonical_bus_family(device_ir.get("bus_type"))
    if bus_type in NO_REGISTER_TIMING_BUS_TYPES or bus_family in NO_REGISTER_TIMING_BUS_TYPES:
        return True
    access_model = device_ir.get("access_model")
    if isinstance(access_model, Mapping):
        kind = _normalize_access_token(str(access_model.get("kind") or ""))
        if kind in NO_REGISTER_TIMING_ACCESS_KINDS:
            return True
    return False


def _normalize_access_token(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", value.strip().lower()).strip("_")


def _flow_has_nonempty_list(value: Any) -> bool:
    return isinstance(value, list) and bool(value)


def _flow_has_producer_step(flow: Mapping[str, Any]) -> bool:
    steps = flow.get("steps")
    if not isinstance(steps, list):
        return False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        op = str(step.get("op") or "").strip()
        transaction = step.get("transaction")
        if op in BUS_TRANSACTION_KINDS and isinstance(transaction, Mapping):
            return True
        if op in {"poll_until", "clear", "select_page"} and isinstance(transaction, Mapping):
            return True
        if op in SIGNAL_STEP_OPS and _step_has_signal_reference_for_canonicalize(step):
            return True
    return False


def _step_has_signal_reference_for_canonicalize(step: Mapping[str, Any]) -> bool:
    for key in ("signal", "source_signal", "output_ref", "condition"):
        value = step.get(key)
        if isinstance(value, str) and value.strip():
            return True
    return False


def _flow_has_non_bus_init_evidence(flow: Mapping[str, Any]) -> bool:
    text_parts: list[str] = []
    for key in ("flow_id", "notes"):
        text_parts.append(str(flow.get(key) or ""))
    preconditions = flow.get("preconditions")
    if isinstance(preconditions, list):
        text_parts.extend(str(item or "") for item in preconditions)
    steps = flow.get("steps")
    has_delay_step = False
    if isinstance(steps, list):
        for step in steps:
            if not isinstance(step, Mapping):
                continue
            op = str(step.get("op") or "").strip()
            if op == "delay":
                has_delay_step = True
            for key in ("notes", "role", "condition", "description"):
                text_parts.append(str(step.get(key) or ""))
    text = " ".join(text_parts)
    if not _NO_BUS_INIT_EVIDENCE_RE.search(text):
        return False
    return has_delay_step or bool(
        re.search(
            r"\b(no\s+bus\s+(?:init|initiali[sz]ation|write)|"
            r"no\s+explicit\s+(?:config|configuration|setup|write)|"
            r"config(?:uration)?\s+writes?\s+(?:is|are)\s+not\s+required|defaults?)\b",
            text,
            re.IGNORECASE,
        )
    )


def _canonicalize_i2c_leading_address_payload(
    device_ir: MutableMapping[str, Any],
    flow: Mapping[str, Any],
    step: MutableMapping[str, Any],
    transaction: Mapping[str, Any],
) -> None:
    if str(device_ir.get("bus_type") or "").lower() != "i2c":
        return
    if str(transaction.get("kind") or "") not in {"write", "write_then_read"}:
        return
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list) or len(bytes_value) <= 1:
        return
    first = _coerce_int(bytes_value[0])
    if first is None:
        return
    if first not in _i2c_logical_addresses(device_ir) and first not in _i2c_wire_address_bytes(device_ir):
        return
    if not _looks_like_i2c_address_payload_context(flow, step, transaction):
        return

    # ``transaction`` is typed as Mapping because the surrounding validator only
    # needs read access, but at runtime this is the mutable dict inside ``step``.
    if isinstance(transaction, MutableMapping):
        del bytes_value[0]
        notes = str(step.get("notes") or "").strip()
        suffix = (
            "Canonicalized: removed a leading I2C slave-address byte from "
            "transaction.bytes; address selection is handled by address_rule "
            "and the bus API, while transaction.bytes contains only pointer, "
            "command, memory-address, or payload bytes."
        )
        step["notes"] = f"{notes} {suffix}".strip()


def _looks_like_i2c_address_payload_context(
    flow: Mapping[str, Any],
    step: Mapping[str, Any],
    transaction: Mapping[str, Any],
) -> bool:
    text = _normalise_flow_text_for_memory_step(step, transaction)
    if "deviceaddress" in text or "slaveaddress" in text:
        return True
    kind = str(transaction.get("kind") or "").lower()
    flow_kind = str(flow.get("kind") or "").lower()
    if kind == "write_then_read" or flow_kind == "read":
        return True
    return False


def _canonicalize_i2c_noncontiguous_multibyte_read(
    device_ir: MutableMapping[str, Any],
    flow: MutableMapping[str, Any],
    steps: list[Any],
    step_index: int,
    step: MutableMapping[str, Any],
    transaction: MutableMapping[str, Any],
) -> None:
    if str(device_ir.get("bus_type") or "").lower() != "i2c":
        return
    if transaction.get("kind") != "write_then_read":
        return
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list) or len(bytes_value) != 1:
        return
    start = _coerce_int(bytes_value[0])
    length = transaction.get("length")
    if start is None or isinstance(length, bool) or not isinstance(length, int) or length <= 1:
        return
    output_registers = _flow_output_register_sequence(device_ir, flow)
    if len(output_registers) < 2:
        return
    addresses = [addr for _token, _name, addr in output_registers]
    if addresses[0] != start:
        return
    if addresses == list(range(start, start + len(addresses))):
        return

    first_token, first_name, first_addr = output_registers[0]
    _rewrite_step_as_single_i2c_register_read(
        step,
        transaction,
        first_addr,
        first_name,
        first_token,
    )
    new_steps = [
        _single_i2c_register_read_step(addr, name, token)
        for token, name, addr in output_registers[1:]
    ]
    steps[step_index + 1:step_index + 1] = new_steps


def _canonicalize_i2c_memory_device_address_step(
    device_ir: Mapping[str, Any],
    step: MutableMapping[str, Any],
    transaction: MutableMapping[str, Any],
) -> bool:
    if not _is_i2c_memory_access(device_ir):
        return False
    if transaction.get("kind") != "write":
        return False
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list) or len(bytes_value) != 1:
        return False
    text = _normalise_flow_text_for_memory_step(step, transaction)
    if "deviceaddress" not in text and "slaveaddress" not in text:
        return False
    if any(token in text for token in ("wordaddress", "memoryaddress", "addressbytes", "addrbytes")):
        return False
    byte_value = _coerce_int(bytes_value[0])
    if byte_value not in {0, None}:
        return False
    step["op"] = "postprocess"
    step["transaction"] = None
    notes = str(step.get("notes") or "").strip()
    suffix = (
        "Canonicalized: removed an I2C device-address-only pseudo step; "
        "the bus API handles slave address and R/W phase selection."
    )
    step["notes"] = f"{notes} {suffix}".strip()
    return True


def _canonicalize_i2c_memory_payload_placeholders(
    device_ir: Mapping[str, Any],
    step: MutableMapping[str, Any],
    transaction: MutableMapping[str, Any],
) -> None:
    if not _is_i2c_memory_access(device_ir):
        return
    if transaction.get("kind") not in {"write", "write_then_read"}:
        return
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list):
        return
    address_bytes = _memory_address_byte_count(device_ir)
    if address_bytes <= 0:
        return
    text = _normalise_flow_text_for_memory_step(step, transaction)
    if "deviceaddress" in text or "slaveaddress" in text:
        if len(bytes_value) > address_bytes and _coerce_int(bytes_value[0]) in {0, None}:
            del bytes_value[0]
            notes = str(step.get("notes") or "").strip()
            suffix = (
                "Canonicalized: removed leading I2C device-address placeholder; "
                "transaction.bytes now starts with runtime memory word-address bytes."
            )
            step["notes"] = f"{notes} {suffix}".strip()
    if not any(token in text for token in ("wordaddress", "memoryaddress", "addressbytes", "addrbytes")):
        return
    for index in range(min(address_bytes, len(bytes_value))):
        if _coerce_int(bytes_value[index]) == 0:
            bytes_value[index] = None


def _is_i2c_memory_access(device_ir: Mapping[str, Any]) -> bool:
    if str(device_ir.get("bus_type") or "").lower() != "i2c":
        return False
    access_model = device_ir.get("access_model")
    return isinstance(access_model, Mapping) and str(access_model.get("kind") or "").lower() == "memory"


def _memory_address_byte_count(device_ir: Mapping[str, Any]) -> int:
    access_model = device_ir.get("access_model")
    if not isinstance(access_model, Mapping):
        return 0
    raw = access_model.get("address_bytes")
    if isinstance(raw, bool) or not isinstance(raw, int):
        return 0
    return max(raw, 0)


def _normalise_flow_text_for_memory_step(
    step: Mapping[str, Any],
    transaction: Mapping[str, Any],
) -> str:
    text = " ".join(_flow_strings_for_canonicalize(step, transaction)).lower()
    return re.sub(r"[^a-z0-9]+", "", text)


def _flow_output_register_sequence(
    device_ir: Mapping[str, Any],
    flow: Mapping[str, Any],
) -> list[tuple[str, str, int]]:
    lookup = _register_lookup_by_normalized_name(device_ir)
    if not lookup:
        return []
    outputs = flow.get("outputs")
    if not isinstance(outputs, list):
        return []
    out: list[tuple[str, str, int]] = []
    seen: set[int] = set()
    for output in outputs:
        if not isinstance(output, Mapping):
            continue
        for token in _byte_source_register_tokens(output.get("byte_source")):
            match = _lookup_register_token(token, lookup)
            if match is None:
                continue
            name, addr = match
            if addr in seen:
                continue
            seen.add(addr)
            out.append((token, name, addr))
    return out


def _byte_source_register_tokens(value: Any) -> list[str]:
    if not isinstance(value, str) or not value.strip():
        return []
    tokens: list[str] = []
    for part in value.split("||"):
        token = part.split(":", 1)[0].strip()
        if token:
            tokens.append(token)
    return tokens


def _register_lookup_by_normalized_name(
    device_ir: Mapping[str, Any],
) -> dict[str, tuple[str, int]]:
    registers = device_ir.get("registers_or_commands")
    if not isinstance(registers, list):
        return {}
    out: dict[str, tuple[str, int]] = {}
    for reg in registers:
        if not isinstance(reg, Mapping):
            continue
        name = str(reg.get("name") or "").strip()
        addr = _coerce_int(reg.get("value"))
        if not name or addr is None:
            continue
        out[_normalize_register_token(name)] = (name, addr)
    return out


def _lookup_register_token(
    token: str,
    lookup: Mapping[str, tuple[str, int]],
) -> tuple[str, int] | None:
    key = _normalize_register_token(token)
    direct = lookup.get(key)
    if direct is not None:
        return direct
    for reg_key, match in lookup.items():
        if key and (key in reg_key or reg_key in key):
            return match
    return None


def _normalize_register_token(value: str) -> str:
    text = value.strip().lower()
    text = re.sub(r"\bremote\s+temperature\s+(\d+)\b", r"remote\1 temp", text)
    text = re.sub(r"\bremote\s+temp\s+(\d+)\b", r"remote\1 temp", text)
    text = re.sub(r"\bremote\s+(\d+)\s+temperature\b", r"remote\1 temp", text)
    text = re.sub(r"\bremote\s+(\d+)\s+temp\b", r"remote\1 temp", text)
    text = text.replace("temperature", "temp")
    text = re.sub(r"\b(?:register|reg)\b", "", text)
    return re.sub(r"[^a-z0-9]+", "", text)


def _rewrite_step_as_single_i2c_register_read(
    step: MutableMapping[str, Any],
    transaction: MutableMapping[str, Any],
    addr: int,
    register_name: str,
    token: str,
) -> None:
    transaction["bytes"] = [f"0x{addr:02X}"]
    transaction["length"] = 1
    transaction["pointer_target"] = register_name
    notes = str(step.get("notes") or "").strip()
    suffix = (
        "Canonicalized from a multi-byte pointer read because output byte "
        f"{token!r} maps to non-contiguous register {register_name}."
    )
    step["notes"] = f"{notes} {suffix}".strip()


def _single_i2c_register_read_step(
    addr: int,
    register_name: str,
    token: str,
) -> dict[str, Any]:
    return {
        "op": "write_then_read",
        "transaction": {
            "kind": "write_then_read",
            "bytes": [f"0x{addr:02X}"],
            "length": 1,
            "pointer_target": register_name,
            "notes": (
                "Canonicalized separate pointer read for non-contiguous "
                f"output byte {token!r}."
            ),
        },
        "notes": (
            f"Read non-contiguous byte {token!r} from {register_name} "
            f"(0x{addr:02X})."
        ),
    }


def canonicalize_primary_interface_and_variant(
    device_ir: Any,
    *,
    target_bus_type: str | None = None,
    target_device_id: str | None = None,
) -> Any:
    """Keep the Device IR focused on the task's primary interface/variant."""
    if not isinstance(device_ir, MutableMapping):
        return device_ir

    bus_family = _canonical_bus_family(target_bus_type or device_ir.get("bus_type"))
    target_norm = _normalise_part_token(target_device_id or device_ir.get("device_id"))

    kept_channels = _filter_list_in_place(
        device_ir,
        "read_channels",
        lambda item: (
            not _mentions_only_other_family_variant(item, target_norm)
            and not _is_non_target_interface_item(item, bus_family)
        ),
    )
    kept_channel_ids = _read_channel_ids_from_items(kept_channels)

    # Drop formula rows that are explicitly variant-only for a sibling.  Do not
    # drop merely because no surviving channel references them; some flows use
    # formulae indirectly through outputs.
    for key in ("registers_or_commands", "conversion_formulae"):
        _filter_list_in_place(
            device_ir,
            key,
            lambda item: not _mentions_only_other_family_variant(item, target_norm),
        )
    for key in ("timing_constraints", "evidence_spans"):
        _filter_list_in_place(
            device_ir,
            key,
            lambda item: (
                not _mentions_only_other_family_variant(item, target_norm)
                and not _is_non_target_interface_item(item, bus_family)
            ),
        )

    flows = device_ir.get("operation_flows")
    if isinstance(flows, list):
        kept_flows: list[Any] = []
        for flow in flows:
            if not isinstance(flow, MutableMapping):
                kept_flows.append(flow)
                continue
            if _mentions_only_other_family_variant(flow, target_norm):
                continue
            if _is_non_target_interface_item(flow, bus_family):
                continue
            _drop_non_channel_outputs(flow, kept_channel_ids)
            _filter_flow_channel_list(flow, kept_channel_ids)
            outputs = flow.get("outputs")
            channels = flow.get("channels")
            if (
                str(flow.get("kind") or "").lower() == "read"
                and isinstance(outputs, list)
                and not outputs
                and isinstance(channels, list)
                and not channels
            ):
                continue
            kept_flows.append(flow)
        if len(kept_flows) != len(flows):
            device_ir["operation_flows"] = kept_flows

    return device_ir


def _canonicalize_i2c_ack_poll_address_probe(
    device_ir: MutableMapping[str, Any],
    flow: MutableMapping[str, Any],
    steps: list[Any],
    step_index: int,
) -> bool:
    if str(device_ir.get("bus_type") or "").lower() != "i2c":
        return False
    step = steps[step_index]
    if not isinstance(step, MutableMapping):
        return False
    transaction = step.get("transaction")
    if not isinstance(transaction, MutableMapping):
        return False
    bytes_value = transaction.get("bytes")
    if not isinstance(bytes_value, list) or len(bytes_value) != 1:
        return False
    byte_value = _coerce_int(bytes_value[0])
    if byte_value is None:
        return False
    address_bytes = _i2c_wire_address_bytes(device_ir)
    if byte_value not in address_bytes:
        return False
    if not _looks_like_ack_poll_probe(flow, step, steps[step_index + 1:]):
        return False

    step["op"] = "poll_until"
    step["transaction"] = None
    if not step.get("register"):
        step["register"] = "I2C address ACK"
    if not step.get("condition"):
        step["condition"] = "device acknowledges configured 7-bit I2C address"
    notes = str(step.get("notes") or "").strip()
    suffix = (
        "Canonicalized ACK/ready polling address probe: the 8-bit I2C "
        "control byte is handled by address_rule and the bus API, not "
        "transaction.bytes payload."
    )
    step["notes"] = f"{notes} {suffix}".strip()
    transaction["bytes"] = None
    transaction["kind"] = "read" if byte_value & 1 else "write"
    return True


def _promote_flow_outputs_to_read_channels(
    device_ir: MutableMapping[str, Any],
    flows: list[Any],
) -> None:
    channels = device_ir.get("read_channels")
    if isinstance(channels, list) and channels:
        return
    if channels is not None and not isinstance(channels, list):
        return

    promoted: list[dict[str, Any]] = []
    seen: set[str] = set()
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        if str(flow.get("kind") or "") != "read":
            continue
        flow_id = str(flow.get("flow_id") or "").strip() or None
        outputs = flow.get("outputs")
        if not isinstance(outputs, list):
            continue
        for output in outputs:
            if not isinstance(output, Mapping):
                continue
            channel_id = str(output.get("channel") or "").strip()
            if not channel_id or channel_id in seen:
                continue
            seen.add(channel_id)
            formula_id = output.get("formula_id")
            promoted.append({
                "id": channel_id,
                "raw_type": _infer_channel_raw_type(output),
                "physical_unit": _infer_promoted_channel_unit(device_ir, output),
                "read_call_hint": None,
                "flow_id": flow_id,
                "source_bytes": _source_bytes_from_output(output),
                "source_signal": output.get("source_signal"),
                "formula_id": formula_id,
                "notes": (
                    "Canonicalized from operation_flows output because the "
                    "extractor omitted read_channels."
                ),
            })
    if promoted:
        device_ir["read_channels"] = promoted


def _infer_promoted_channel_unit(
    device_ir: Mapping[str, Any],
    output: Mapping[str, Any],
) -> str:
    unit = str(output.get("unit") or "").strip()
    if unit:
        return unit

    formula_id = str(output.get("formula_id") or "").strip()
    formulas = device_ir.get("conversion_formulae")
    if formula_id and isinstance(formulas, list):
        for formula in formulas:
            if not isinstance(formula, Mapping):
                continue
            if str(formula.get("name") or "").strip() != formula_id:
                continue
            expr = formula.get("integer_approximation_expression")
            if isinstance(expr, Mapping):
                output_spec = expr.get("output")
                if isinstance(output_spec, Mapping):
                    formula_unit = str(output_spec.get("unit") or "").strip()
                    if formula_unit:
                        return formula_unit
            break

    text = " ".join(
        str(output.get(key) or "")
        for key in ("channel", "byte_source", "source_signal", "notes")
    ).lower()
    if any(token in text for token in ("memory", "payload", "packet", "data_bytes")):
        return "bytes"
    return "raw"


def _infer_channel_raw_type(output: Mapping[str, Any]) -> str:
    text = " ".join(
        str(output.get(key) or "")
        for key in ("channel", "byte_source", "source_signal", "notes")
    ).lower()
    byte_source = str(output.get("byte_source") or "").lower()
    if re.search(r"\b(?:20|24|32)[-\s]*bit\b", text) or "xlsb" in byte_source:
        return "uint32"
    if re.search(r"\b16[-\s]*bit\b", text):
        return "uint16"
    if byte_source.count("||") >= 2:
        return "uint32"
    if byte_source.count("||") == 1:
        return "uint16"
    if any(token in text for token in ("memory", "payload", "packet", "stream", "data_bytes")):
        return "bytes"
    return "uint8"


def _promote_register_groups_to_read_channels_and_flow(
    device_ir: MutableMapping[str, Any],
    flows: list[Any],
) -> None:
    channels = device_ir.get("read_channels")
    if isinstance(channels, list) and channels:
        return
    if channels is not None and not isinstance(channels, list):
        return
    if _canonical_bus_family(device_ir.get("bus_type")) != "i2c":
        return
    groups = _measurement_register_groups(device_ir)
    if not groups:
        return

    promoted: list[dict[str, Any]] = []
    outputs: list[dict[str, Any]] = []
    for group in groups:
        channel_id = group["channel_id"]
        formula_id = _formula_id_for_channel(device_ir, channel_id)
        unit = _unit_for_channel(device_ir, channel_id, formula_id)
        source = group["byte_source"]
        promoted.append({
            "id": channel_id,
            "raw_type": "uint32" if group["width_bits"] > 16 else "uint16",
            "physical_unit": unit,
            "read_call_hint": None,
            "flow_id": "read_measurements",
            "source_bytes": [source],
            "source_signal": None,
            "formula_id": formula_id,
            "notes": (
                "Canonicalized from contiguous measurement data registers "
                "because the extractor omitted read_channels."
            ),
        })
        outputs.append({
            "channel": channel_id,
            "byte_source": source,
            "source_signal": None,
            "formula_id": formula_id,
            "unit": unit,
            "notes": "Canonicalized measurement output from register group.",
        })
    if not promoted:
        return

    start = min(group["start_addr"] for group in groups)
    end = max(group["end_addr"] for group in groups)
    has_read_measurements_flow = any(
        isinstance(flow, Mapping)
        and str(flow.get("flow_id") or "") == "read_measurements"
        for flow in flows
    )
    if not has_read_measurements_flow and not _groups_cover_contiguous_span(groups, start, end):
        return

    device_ir["read_channels"] = promoted
    if any(
        isinstance(flow, Mapping)
        and str(flow.get("flow_id") or "") == "read_measurements"
        for flow in flows
    ):
        return
    flows.append({
        "flow_id": "read_measurements",
        "kind": "read",
        "channels": [channel["id"] for channel in promoted],
        "preconditions": [
            "Device is configured for measurement and conversion is complete.",
            "Calibration data has been read when compensation formulae require it.",
        ],
        "steps": [
            {
                "op": "write_then_read",
                "transaction": {
                    "kind": "write_then_read",
                    "bytes": [f"0x{start:02X}"],
                    "length": end - start + 1,
                    "pointer_target": _register_name_for_address(device_ir, start),
                    "notes": (
                        f"Canonicalized burst read over contiguous measurement "
                        f"registers 0x{start:02X}..0x{end:02X}."
                    ),
                },
                "notes": "Read contiguous measurement register block.",
            }
        ],
        "outputs": outputs,
        "requires_human": False,
        "notes": (
            "Canonicalized read flow reconstructed from contiguous public "
            "measurement registers because the extractor omitted an executable "
            "data-read flow."
        ),
    })


def _measurement_register_groups(device_ir: Mapping[str, Any]) -> list[dict[str, Any]]:
    registers = device_ir.get("registers_or_commands")
    if not isinstance(registers, list):
        return []
    by_group: dict[str, dict[str, tuple[str, int]]] = {}
    for reg in registers:
        if not isinstance(reg, Mapping):
            continue
        access = str(reg.get("access") or "").lower()
        if access and access not in {"ro", "r", "read", "read-only"}:
            continue
        name = str(reg.get("name") or "").strip()
        addr = _coerce_int(reg.get("value"))
        if not name or addr is None:
            continue
        parsed = _parse_measurement_register_name(name)
        if parsed is None:
            continue
        group, part = parsed
        by_group.setdefault(group, {})[part] = (name, addr)
    out: list[dict[str, Any]] = []
    for group, parts in by_group.items():
        if "msb" not in parts or "lsb" not in parts:
            continue
        ordered_parts = ["msb", "lsb"] + (["xlsb"] if "xlsb" in parts else [])
        addrs = [parts[part][1] for part in ordered_parts]
        if sorted(addrs) != list(range(min(addrs), max(addrs) + 1)):
            continue
        channel_id = _channel_id_from_register_group(group)
        if not channel_id:
            continue
        width_bits = 20 if "xlsb" in parts else 16
        source = " || ".join(
            f"{parts[part][0]}:{4 if part == 'xlsb' else 8}"
            for part in ordered_parts
        )
        out.append({
            "group": group,
            "channel_id": channel_id,
            "byte_source": source,
            "width_bits": width_bits,
            "start_addr": min(addrs),
            "end_addr": max(addrs),
        })
    return sorted(out, key=lambda item: item["start_addr"])


def _parse_measurement_register_name(name: str) -> tuple[str, str] | None:
    text = re.sub(r"[^a-z0-9]+", "_", name.strip().lower()).strip("_")
    for suffix in ("msb", "lsb", "xlsb", "high", "low"):
        marker = f"_{suffix}"
        if text.endswith(marker):
            group = text[: -len(marker)]
            part = {"high": "msb", "low": "lsb"}.get(suffix, suffix)
            break
    else:
        return None
    if not group or any(token in group for token in ("calib", "coeff", "trim")):
        return None
    return group, part


def _channel_id_from_register_group(group: str) -> str:
    aliases = {
        "temp": "temperature",
        "temperature": "temperature",
        "press": "pressure",
        "pressure": "pressure",
        "hum": "humidity",
        "humidity": "humidity",
    }
    return aliases.get(group, group)


def _formula_id_for_channel(device_ir: Mapping[str, Any], channel_id: str) -> str | None:
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, list):
        return None
    aliases = {channel_id}
    if channel_id == "temperature":
        aliases.add("temp")
    elif channel_id == "pressure":
        aliases.add("press")
    elif channel_id == "humidity":
        aliases.add("hum")
    for row in formulae:
        if not isinstance(row, Mapping):
            continue
        name = str(row.get("name") or "")
        norm = re.sub(r"[^a-z0-9]+", "_", name.lower())
        if any(alias in norm for alias in aliases):
            return name.strip() or None
    return None


def _unit_for_channel(device_ir: Mapping[str, Any], channel_id: str, formula_id: str | None) -> str:
    if formula_id:
        for row in device_ir.get("conversion_formulae") or []:
            if not isinstance(row, Mapping) or str(row.get("name") or "") != formula_id:
                continue
            expr = row.get("integer_approximation_expression")
            if isinstance(expr, Mapping):
                output = expr.get("output")
                if isinstance(output, Mapping) and isinstance(output.get("unit"), str):
                    return str(output["unit"])
    if channel_id == "temperature":
        return "milli_degC"
    if channel_id == "pressure":
        return "Pa"
    if channel_id == "humidity":
        return "milli_percent_rH"
    return "raw"


def _groups_cover_contiguous_span(groups: list[dict[str, Any]], start: int, end: int) -> bool:
    covered: set[int] = set()
    for group in groups:
        covered.update(range(group["start_addr"], group["end_addr"] + 1))
    return covered == set(range(start, end + 1))


def _register_name_for_address(device_ir: Mapping[str, Any], address: int) -> str | None:
    for reg in device_ir.get("registers_or_commands") or []:
        if isinstance(reg, Mapping) and _coerce_int(reg.get("value")) == address:
            name = reg.get("name")
            return str(name) if name is not None else None
    return None


def _source_bytes_from_output(output: Mapping[str, Any]) -> list[str] | None:
    value = output.get("byte_source")
    if not isinstance(value, str) or not value.strip():
        return None
    return [value.strip()]


def _read_channel_ids(device_ir: Mapping[str, Any]) -> set[str]:
    channels = device_ir.get("read_channels")
    if not isinstance(channels, list):
        return set()
    ids: set[str] = set()
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        if channel_id:
            ids.add(channel_id)
    return ids


def _drop_non_channel_outputs(flow: MutableMapping[str, Any], valid_channels: set[str]) -> None:
    outputs = flow.get("outputs")
    if not isinstance(outputs, list) or not outputs:
        return
    if not valid_channels:
        return
    kept = []
    dropped = []
    for output in outputs:
        if not isinstance(output, MutableMapping):
            dropped.append(str(output))
            continue
        channel = str(output.get("channel") or "").strip()
        if channel in valid_channels:
            kept.append(output)
        else:
            dropped.append(channel or str(output))
    if len(kept) == len(outputs):
        return
    flow["outputs"] = kept
    if dropped:
        notes = str(flow.get("notes") or "").strip()
        suffix = (
            "Canonicalized: removed non-read_channel outputs "
            + ", ".join(sorted(set(dropped)))
            + "; keep them as probe/status notes rather than public measurement outputs."
        )
        flow["notes"] = f"{notes} {suffix}".strip()


def _filter_flow_channel_list(
    flow: MutableMapping[str, Any],
    valid_channels: set[str],
) -> None:
    channels = flow.get("channels")
    if not isinstance(channels, list):
        return
    kept: list[Any] = []
    for channel in channels:
        cid = str(channel or "").strip()
        if not cid or cid in valid_channels:
            kept.append(channel)
    if len(kept) != len(channels):
        flow["channels"] = kept


def _filter_list_in_place(
    root: MutableMapping[str, Any],
    key: str,
    predicate,
) -> list[Any]:
    values = root.get(key)
    if not isinstance(values, list):
        return []
    kept = [item for item in values if predicate(item)]
    if len(kept) != len(values):
        root[key] = kept
    return kept


def _read_channel_ids_from_items(channels: list[Any]) -> set[str]:
    ids: set[str] = set()
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        cid = str(channel.get("id") or "").strip()
        if cid:
            ids.add(cid)
    return ids


def _canonical_bus_family(value: Any) -> str:
    text = str(value or "").strip().lower()
    if not text:
        return ""
    if text.startswith("smbus") or text.startswith("i2c"):
        return "i2c"
    if text.startswith("spi") or text == "display_spi":
        return "spi"
    if text.startswith("uart") or text.startswith("serial"):
        return "uart"
    if text.startswith("gpio") or text in {"gpio_pulse", "gpio_timing"}:
        return "gpio"
    if text.startswith("display_parallel"):
        return "parallel"
    return text.split("_", 1)[0].split("-", 1)[0]


def _item_text(item: Any) -> str:
    if isinstance(item, Mapping):
        parts: list[str] = []
        for _path, text in _walk_strings(item):
            parts.append(text)
        return " ".join(parts)
    if isinstance(item, str):
        return item
    return ""


def _matches_any_marker(text: str, family: str) -> bool:
    patterns = _BUS_MARKER_PATTERNS.get(family, ())
    return any(pattern.search(text) for pattern in patterns)


def _is_non_target_interface_item(item: Any, target_family: str) -> bool:
    if not target_family:
        return False
    text = _item_text(item)
    if not text:
        return False
    if _matches_any_marker(text, target_family):
        return False

    # Analog outputs are not a supported generated bus in the current
    # pipeline, so they are alternate outputs for every supported target bus.
    if target_family != "analog" and _matches_any_marker(text, "analog"):
        return True

    for family in ("i2c", "spi", "uart", "gpio"):
        if family == target_family:
            continue
        if _matches_any_marker(text, family):
            return True
    return False


def _normalise_part_token(value: Any) -> str:
    return re.sub(r"[^a-z0-9]", "", str(value or "").lower())


def _part_family_prefix(target_norm: str) -> str:
    match = re.match(r"^([a-z]{2,})(?:\d|$)", target_norm)
    return match.group(1) if match else ""


def _part_tokens(text: str) -> set[str]:
    out: set[str] = set()
    for match in _PART_TOKEN_RE.finditer(text or ""):
        token = _normalise_part_token(match.group(0))
        if token and token not in {"i2c", "spi", "uart"}:
            out.add(token)
    return out


def _mentions_only_other_family_variant(item: Any, target_norm: str) -> bool:
    if not target_norm:
        return False
    text = _item_text(item)
    if not text:
        return False
    tokens = _part_tokens(text)
    if not tokens or target_norm in tokens:
        return False

    prefix = _part_family_prefix(target_norm)
    if not prefix:
        return False
    return any(
        token != target_norm
        and token.startswith(prefix)
        and any(ch.isdigit() for ch in token)
        for token in tokens
    )


def _looks_like_non_bus_abstract_step(step: Mapping[str, Any]) -> bool:
    if not any(step.get(key) for key in ("register", "mask", "value", "length")):
        return True
    text = " ".join(
        str(step.get(key) or "")
        for key in ("role", "register", "condition", "notes", "output_ref")
    ).lower()
    return any(
        token in text
        for token in (
            "api",
            "abstract",
            "no direct register",
            "no register",
            "not specified",
            "not provided",
        )
    )


def _looks_like_ack_poll_probe(
    flow: Mapping[str, Any],
    step: Mapping[str, Any],
    following_steps: Sequence[Any],
) -> bool:
    text = " ".join(_flow_strings_for_canonicalize(flow, step)).lower()
    has_ack_or_ready_context = any(
        token in text
        for token in (
            "ack polling",
            "acknowledge polling",
            "ack received",
            "poll",
            "ready",
            "write cycle complete",
            "write-cycle complete",
        )
    )
    if not has_ack_or_ready_context:
        return False
    if str(flow.get("kind") or "").lower() in {"other", "probe"}:
        return True
    return any(
        isinstance(next_step, Mapping)
        and str(next_step.get("op") or "") in {"poll_until", "wait_until_ready"}
        for next_step in following_steps
    )


def _flow_strings_for_canonicalize(*values: Mapping[str, Any]) -> list[str]:
    out: list[str] = []
    for value in values:
        for _path, text in _walk_strings(value):
            out.append(text)
    return out


def _walk_strings(value: Any, path: str = ""):
    if isinstance(value, str):
        yield path or "$", value
    elif isinstance(value, Mapping):
        for key, item in value.items():
            next_path = f"{path}.{key}" if path else str(key)
            yield from _walk_strings(item, next_path)
    elif isinstance(value, list):
        for index, item in enumerate(value):
            next_path = f"{path}[{index}]" if path else f"[{index}]"
            yield from _walk_strings(item, next_path)


def _i2c_wire_address_bytes(device_ir: Mapping[str, Any]) -> set[int]:
    out: set[int] = set()
    for address in _i2c_logical_addresses(device_ir):
        out.add((address << 1) & 0xFE)
        out.add(((address << 1) | 1) & 0xFF)
    return out


def _i2c_logical_addresses(device_ir: Mapping[str, Any]) -> set[int]:
    out: set[int] = set()
    rule = device_ir.get("address_rule")
    if not isinstance(rule, Mapping):
        return out
    values: list[Any] = []
    raw_addresses = rule.get("addresses")
    if isinstance(raw_addresses, list):
        for item in raw_addresses:
            if isinstance(item, Mapping):
                values.extend((item.get("address"), item.get("value"), item.get("default")))
            else:
                values.append(item)
    values.extend((rule.get("address"), rule.get("value"), rule.get("default")))
    for value in values:
        address = _coerce_int(value)
        if address is None or address < 0 or address > 0x7F:
            continue
        out.add(address)
    return out


def _has_any_explicit_default(addresses: Sequence[Any]) -> bool:
    """Return ``True`` if at least one entry already has ``is_default=True``."""
    return any(
        isinstance(entry, MutableMapping) and entry.get("is_default") is True
        for entry in addresses
    )


def _any_default_marked(addresses: Sequence[Any]) -> bool:
    return _has_any_explicit_default(addresses)


def _annotate_with_explicit_keywords(
    addresses: Sequence[Any],
    rule: MutableMapping[str, Any],
) -> None:
    """Mark entries whose description contains a high-confidence default token."""
    for index, entry in enumerate(addresses):
        if not isinstance(entry, MutableMapping):
            continue
        if "is_default" in entry and entry["is_default"] is not None:
            continue
        desc = _description_text(entry)
        matched = _match_first(desc, EXPLICIT_DEFAULT_PATTERNS)
        if matched is not None:
            entry["is_default"] = True
            rule.setdefault(
                "default_address_resolution",
                {
                    "method": "description-keyword",
                    "confidence": "high",
                    "matched_pattern": matched.pattern,
                    "matched_index": index,
                },
            )
            return  # only mark the first explicit-default match


def _annotate_with_strap_pin_tie(
    addresses: Sequence[Any],
    rule: MutableMapping[str, Any],
) -> None:
    """Mark the row that describes address strap pins tied low."""
    for index, entry in enumerate(addresses):
        if not isinstance(entry, MutableMapping):
            continue
        if "is_default" in entry and entry["is_default"] is not None:
            continue
        desc = _description_text(entry)
        matched = _match_first(desc, STRAP_PIN_TIE_PATTERNS)
        if matched is not None:
            entry["is_default"] = True
            rule.setdefault(
                "default_address_resolution",
                {
                    "method": "strap-pin-tie",
                    "confidence": "medium",
                    "matched_pattern": matched.pattern,
                    "matched_index": index,
                },
            )
            return


def _annotate_with_float_strap_pins(
    addresses: Sequence[Any],
    rule: MutableMapping[str, Any],
) -> None:
    """Prefer an all-floating/open address-pin row when no default is stated."""
    candidates: list[tuple[int, MutableMapping[str, Any], int]] = []
    for index, entry in enumerate(addresses):
        if not isinstance(entry, MutableMapping):
            continue
        if "is_default" in entry and entry["is_default"] is not None:
            continue
        desc = _description_text(entry)
        match_count = _float_pin_assignment_count(desc)
        if match_count:
            candidates.append((match_count, entry, index))
    if not candidates:
        return
    match_count, entry, index = max(candidates, key=lambda item: (item[0], -item[2]))
    entry["is_default"] = True
    rule.setdefault(
        "default_address_resolution",
        {
            "method": "strap-pin-float",
            "confidence": "medium",
            "matched_pin_count": match_count,
            "matched_index": index,
        },
    )


def _prefer_float_strap_default_over_weak_low_default(
    addresses: Sequence[Any],
    rule: MutableMapping[str, Any],
) -> None:
    """Prefer a stronger floating-pin default in tri-state address tables."""
    default_entries = [
        entry for entry in addresses
        if isinstance(entry, MutableMapping) and entry.get("is_default") is True
    ]
    if len(default_entries) != 1:
        return
    current = default_entries[0]
    current_desc = _description_text(current)
    if re.search(r"\b(?:factory\s+default|default\s+address|preset|primary)\b", current_desc, re.IGNORECASE):
        return
    if _low_pin_assignment_count(current_desc) < 2:
        return

    candidates: list[tuple[int, MutableMapping[str, Any], int]] = []
    for index, entry in enumerate(addresses):
        if not isinstance(entry, MutableMapping):
            continue
        count = _float_pin_assignment_count(_description_text(entry))
        if count >= 2:
            candidates.append((count, entry, index))
    if not candidates:
        return
    _count, preferred, index = max(candidates, key=lambda item: (item[0], -item[2]))
    if preferred is current:
        return
    for entry in addresses:
        if isinstance(entry, MutableMapping):
            entry["is_default"] = entry is preferred
    rule["default_address_resolution"] = {
        "method": "strap-pin-float-over-weak-low-default",
        "confidence": "medium",
        "matched_index": index,
        "warning": (
            "A stronger all-floating/open address-pin row was preferred over "
            "a weak all-low default label without factory-default wording."
        ),
    }


def _float_pin_assignment_count(text: str) -> int:
    return len(_FLOAT_PIN_ASSIGNMENT_RE.findall(text or ""))


def _low_pin_assignment_count(text: str) -> int:
    return len(_LOW_PIN_ASSIGNMENT_RE.findall(text or ""))


def _ensure_one_default(
    addresses: Sequence[Any],
    rule: MutableMapping[str, Any],
) -> None:
    """Cardinality fallback: pick a default when the per-entry pass left none."""
    truthy_defaults = [
        entry for entry in addresses
        if isinstance(entry, MutableMapping) and entry.get("is_default") is True
    ]
    if truthy_defaults:
        return

    if len(addresses) == 1:
        only = addresses[0]
        if isinstance(only, MutableMapping):
            only["is_default"] = True
            rule.setdefault(
                "default_address_resolution",
                {"method": "single-entry", "confidence": "high"},
            )
        return

    for entry in addresses:
        if isinstance(entry, MutableMapping):
            entry["is_default"] = True
            rule.setdefault(
                "default_address_resolution",
                {
                    "method": "first-fallback",
                    "confidence": "low",
                    "warning": (
                        "Multiple address entries without an explicit default; "
                        "selecting the first as default. Confirm against the "
                        "datasheet — see DEFAULT_KEYWORD_PATTERNS in "
                        "drivergen.core.ir_canonicalize."
                    ),
                },
            )
            return


def _ensure_addressing_form(
    rule: MutableMapping[str, Any],
    addresses: Any,
) -> None:
    """Fill ``addressing_form`` only when the value is unambiguous."""
    existing = rule.get("addressing_form")
    if isinstance(existing, str) and existing in ADDRESSING_FORM_VALUES:
        return
    if not isinstance(addresses, list) or not addresses:
        return

    fits_seven_bit = True
    for entry in addresses:
        value = _entry_address_value(entry)
        addr_int = _coerce_int(value)
        if addr_int is None:
            fits_seven_bit = False
            break
        if addr_int < 0 or addr_int > 0xFF:
            fits_seven_bit = False
            break
        if addr_int >= 0x80:
            fits_seven_bit = False
            break
    if fits_seven_bit:
        rule["addressing_form"] = "7-bit"


def _description_text(entry: Mapping[str, Any]) -> str:
    parts: list[str] = []
    for key in ("description", "condition", "notes", "name", "label"):
        value = entry.get(key)
        if isinstance(value, str) and value.strip():
            parts.append(value.strip())
    return " | ".join(parts)


def _match_first(
    text: str, patterns: Sequence[re.Pattern[str]],
) -> re.Pattern[str] | None:
    if not text:
        return None
    for pattern in patterns:
        if pattern.search(text):
            return pattern
    return None


def _entry_address_value(entry: Any) -> Any:
    if isinstance(entry, Mapping):
        for key in ("address", "value", "addr", "i2c_address"):
            value = entry.get(key)
            if value is not None:
                return value
        return None
    return entry


def _coerce_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        text = value.strip()
        if not text:
            return None
        try:
            if text.lower().startswith("0x"):
                return int(text, 16)
            if text.lower().startswith("0b"):
                return int(text, 2)
            return int(text, 0)
        except ValueError:
            return None
    return None
