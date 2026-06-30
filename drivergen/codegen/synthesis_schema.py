"""JSON schemas for driver synthesis responses."""
from __future__ import annotations

from typing import Any, Dict, List, Mapping, Tuple

from .classify_device import (
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
    EVAL_CLASSES,
)


# Per-class api_contract branch schemas

_C_CALL_SCHEMA: Dict[str, Any] = {
    "type": "string",
    "description": (
        "C call expression. May reference (a) `&g_eval_dev` - the "
        "auto-declared device object whose type is `dev_struct_type`; "
        "(b) any identifier you declared in `init_extra_setup_c` that "
        "is still in scope at the call site; (c) template-supplied "
        "locals such as `&raw`, `&ax`, `addr`, `buf`, `len`, `data`, "
        "`&t`. Trailing semicolons tolerated.\n\n"
        "OUTPUT-LOCAL RULE: the adapter generator declares the "
        "result-receiving local automatically; you MUST pass that "
        "exact name as the output pointer or the value the harness "
        "sees will be `0` even when the call succeeds:\n"
        "  - `read_call` (single_channel): adapter declares "
        "`<primary_raw_type> raw = 0;` and copies `raw` into `*out` "
        "after the call. So `read_call` MUST take the form "
        "`<func>(&g_eval_dev, &raw)` (or `<func>(g_eval_dev, &raw)` "
        "for pointer-type device objects). Picking any other local "
        "name either fails to compile or writes to the wrong storage.\n"
        "  - `channels[i].call` (multi_channel): adapter declares "
        "`<out_type> <out_arg> = 0;` for each `channels[i]`, where "
        "`<out_arg>` is taken VERBATIM from the JSON field. So "
        "`channels[i].call` MUST contain `&<out_arg>` somewhere in "
        "its argument list, using the SAME spelling as the "
        "`out_arg` field.\n"
        "    When several multi_channel entries come from the same "
        "Device IR operation flow / read transaction, use one aggregate "
        "driver function and repeat that same `channels[i].call` string "
        "for every involved channel. The adapter groups consecutive "
        "identical calls and invokes the aggregate function once.\n"
        "  - `mem_read_call` / `mem_write_call` (memory): buffer "
        "local is `buf`; pass `buf` (no `&`, it is already a "
        "pointer) and `len`/`addr` as the size/offset arguments.\n\n"
        "BUS-HANDLE RULE for `init_call`: when the driver's init "
        "function signature is `xxx_init(<dev>, <BusType> *handle, ...)`, "
        "the handle argument MUST be a local declared in "
        "`init_extra_setup_c` or a function-call expression that returns "
        "the correct handle type. Never pass the raw `bus_name` parameter "
        "where a typed handle is expected, and do not reference undeclared "
        "board globals or external handles that the stub build cannot link."
    ),
    "minLength": 1,
}

# Allowed C scalar type names for raw-sample buffers.
_PREAMBLE_C_SCHEMA: Dict[str, Any] = {
    "type": "string",
    "description": (
        "Optional C-source preamble pasted into the generated "
        "`<device>_eval_adapter.c` immediately AFTER the stock "
        "`#include \"drivergen_eval_adapter.h\"` / `#include \"<driver>.h\"` "
        "lines and BEFORE the global metadata block.  Use it to pull in "
        "extra platform headers, define "
        "helper macros, or declare local state.\n\n"
        "CRITICAL: DO NOT re-declare the canonical device object "
        "`g_eval_dev`.  The adapter generator auto-declares a single "
        "`static <dev_struct_type> g_eval_dev;` (or `static T * "
        "g_eval_dev = NULL;` when `dev_struct_type` ends in `*`) based "
        "on the `dev_struct_type` field of this contract.  A second "
        "declaration of the same name in `preamble_c` will trigger a "
        "C redefinition error or silently shadow the adapter's copy."
    ),
}

_DEV_STRUCT_TYPE_SCHEMA: Dict[str, Any] = {
    "type": "string",
    "description": (
        "C type spelling used to declare the canonical device object "
        "`static <type> g_eval_dev;` in the generated adapter. "
        "Must be a full C type, including `struct`/`union`/`enum` tags "
        "when required. Pointer types are supported by ending the spelling "
        "with `*`; the adapter then initializes `g_eval_dev` to NULL.\n"
        "When `init_call` must pass a pointer to this object to the "
        "driver, use `&g_eval_dev` ONLY if the type does NOT end in "
        "`*`. For pointer types, pass `g_eval_dev` directly."
    ),
}

_C_SCALAR_TYPE_SCHEMA: Dict[str, Any] = {
    "type": "string",
    "enum": [
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int8_t",  "int16_t",  "int32_t",  "int64_t",
    ],
    "description": (
        "C scalar type for the raw sample. Must be a stdint.h type; "
        "project-specific shorthand like `u8`/`u16`/`u32` is rejected "
        "because the stub build does not guarantee those aliases."
    ),
}

_INIT_EXTRA_SETUP_SCHEMA: Dict[str, Any] = {
    "type": "string",
    "description": (
        "Optional C statements inserted INSIDE "
        "`int drivergen_eval_init(const char *bus_name)`, AFTER the "
        "`if (bus_name == NULL)` guard but BEFORE the call to the "
        "driver's init function. Use it to resolve the bus handle from "
        "`bus_name`, populate `g_eval_dev`, or prepare local variables "
        "referenced by `init_call`.\n\n"
        "Rules:\n"
        "  - Do not redeclare `bus_name`; it is the incoming adapter "
        "parameter.\n"
        "  - Do not redeclare `g_eval_dev`; assign into it instead.\n"
        "  - Do not pass raw `bus_name` where the driver expects a typed "
        "bus handle. Resolve or cast a local handle first.\n"
        "  - Do not take the address of a function return value. Assign it "
        "to a local pointer first, then pass that pointer.\n"
        "  - Do not reference undeclared board globals or external handles "
        "that are not provided by the contract.\n"
        "  - Do not declare read/output locals here; the adapter declares "
        "`raw` or each channel `out_arg` at the read call site.\n"
        "  - Adapter ABI error constants may be used only inside this "
        "adapter-side block. Driver source/header code should return plain "
        "`int` values and must not depend on adapter-only constants."
    ),
}

_CHANNEL_ENTRY_SCHEMA: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["id", "call", "out_arg", "out_type"],
    "properties": {
        "id":            {"type": "string", "minLength": 1},
        "call":          _C_CALL_SCHEMA,
        "out_arg":       {
            "type": "string",
            "pattern": r"^[A-Za-z_][A-Za-z0-9_]*$",
            "description": (
                "C identifier the adapter will declare as the "
                "result-receiving local for this channel. The adapter "
                "emits `<out_type> <out_arg> = 0;` BEFORE the call and "
                "`g_cached[i] = (int32_t)<out_arg>;` AFTER it, so the "
                "name you put here MUST also appear inside the `call` "
                "expression as the address-of output argument "
                "(`&<out_arg>`). If `out_arg=\"ax\"` then `call` must "
                "contain `&ax`; if `out_arg=\"temperature_mdegc\"` "
                "then `call` must contain `&temperature_mdegc`. "
                "Mismatched spelling silently publishes zero into the "
                "channel cache even when the call returns OK. If this "
                "channel shares one Device IR operation flow / bus read "
                "transaction with other channels, reuse the same "
                "aggregate call string on all of those channel entries; "
                "only change each entry's `id`, `out_arg`, `out_type`, "
                "and unit metadata."
            ),
        },
        "out_type":      _C_SCALAR_TYPE_SCHEMA,
        "physical_unit": {"type": "string", "default": "raw"},
        "scale":         {"type": "integer"},
    },
}

API_CONTRACT_SINGLE_CHANNEL: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["eval_class", "init_call", "read_call",
                  "primary_raw_type", "primary_raw_unit"],
    "properties": {
        "eval_class":        {"const": EVAL_CLASS_SINGLE_CHANNEL},
        "init_call":         _C_CALL_SCHEMA,
        "read_call":         _C_CALL_SCHEMA,
        "primary_raw_type":  _C_SCALAR_TYPE_SCHEMA,
        "primary_raw_unit":  {"type": "string", "minLength": 1},
        "dev_struct_type":   _DEV_STRUCT_TYPE_SCHEMA,
        "init_extra_setup_c": _INIT_EXTRA_SETUP_SCHEMA,
        "preamble_c":        _PREAMBLE_C_SCHEMA,
    },
}

API_CONTRACT_MULTI_CHANNEL: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["eval_class", "init_call", "channels"],
    "properties": {
        "eval_class":      {"const": EVAL_CLASS_MULTI_CHANNEL},
        "init_call":       _C_CALL_SCHEMA,
        "channels":        {
            "type": "array",
            "minItems": 2,
            "items": _CHANNEL_ENTRY_SCHEMA,
        },
        "dev_struct_type": _DEV_STRUCT_TYPE_SCHEMA,
        "init_extra_setup_c": _INIT_EXTRA_SETUP_SCHEMA,
        "preamble_c":      _PREAMBLE_C_SCHEMA,
    },
}

API_CONTRACT_MEMORY: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["eval_class", "init_call", "mem_read_call",
                  "mem_write_call", "memory_size_bytes"],
    "properties": {
        "eval_class":        {"const": EVAL_CLASS_MEMORY},
        "init_call":         _C_CALL_SCHEMA,
        "mem_read_call":     _C_CALL_SCHEMA,
        "mem_write_call":    _C_CALL_SCHEMA,
        "memory_size_bytes": {"type": "integer", "minimum": 1},
        "memory_page_bytes": {"type": "integer", "minimum": 0},
        "dev_struct_type":   _DEV_STRUCT_TYPE_SCHEMA,
        "init_extra_setup_c": _INIT_EXTRA_SETUP_SCHEMA,
        "preamble_c":        _PREAMBLE_C_SCHEMA,
    },
}

API_CONTRACT_DISPLAY: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["eval_class", "init_call", "output_frame_call"],
    "properties": {
        "eval_class":        {"const": EVAL_CLASS_DISPLAY},
        "init_call":         _C_CALL_SCHEMA,
        "output_frame_call": _C_CALL_SCHEMA,
        "read_status_call":  {
            "anyOf": [_C_CALL_SCHEMA, {"type": "null"}],
            "description": (
                "Optional. Set to null (or omit) if the device does "
                "not support a status read; the adapter will return "
                "DRIVERGEN_EVAL_ERR_UNSUPPORTED."
            ),
        },
        "dev_struct_type":   _DEV_STRUCT_TYPE_SCHEMA,
        "init_extra_setup_c": _INIT_EXTRA_SETUP_SCHEMA,
        "preamble_c":        _PREAMBLE_C_SCHEMA,
    },
}

_RTC_TIME_FIELDS: Tuple[str, ...] = (
    "year", "month", "day", "hour", "minute", "second", "weekday",
)

API_CONTRACT_RTC: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["eval_class", "init_call", "get_time_call"],
    "properties": {
        "eval_class":     {"const": EVAL_CLASS_RTC},
        "init_call":      _C_CALL_SCHEMA,
        "get_time_call":  _C_CALL_SCHEMA,
        "set_time_call":  {
            "anyOf": [_C_CALL_SCHEMA, {"type": "null"}],
            "description": (
                "Optional. Set to null (or omit) if the device does "
                "not support set_time; adapter returns "
                "DRIVERGEN_EVAL_ERR_UNSUPPORTED."
            ),
        },
        "time_struct_decl": {
            "type": "string",
            "description": (
                "C declaration of the native time struct the driver "
                "fills."
            ),
        },
        "time_struct_from_in": {
            "type": "string",
            "description": (
                "C declaration + assignment that converts the ABI "
                "`drivergen_eval_time_t *in` into the driver's native "
                "struct."
            ),
        },
        "time_fields":   {
            "type": "object",
            "additionalProperties": False,
            "properties": {
                f: {"type": ["string", "null"]} for f in _RTC_TIME_FIELDS
            },
            "description": (
                "Optional map of ABI time fields to C expressions "
                "referencing the native struct. "
                "Omitted fields default to 0; if the whole object is "
                "omitted the template assumes the driver speaks the "
                "ABI struct directly."
            ),
        },
        "dev_struct_type": _DEV_STRUCT_TYPE_SCHEMA,
        "init_extra_setup_c": _INIT_EXTRA_SETUP_SCHEMA,
        "preamble_c":      _PREAMBLE_C_SCHEMA,
    },
}

API_CONTRACT_BRANCHES: Mapping[str, Dict[str, Any]] = {
    EVAL_CLASS_SINGLE_CHANNEL: API_CONTRACT_SINGLE_CHANNEL,
    EVAL_CLASS_MULTI_CHANNEL:  API_CONTRACT_MULTI_CHANNEL,
    EVAL_CLASS_MEMORY:         API_CONTRACT_MEMORY,
    EVAL_CLASS_DISPLAY:        API_CONTRACT_DISPLAY,
    EVAL_CLASS_RTC:            API_CONTRACT_RTC,
}


# test_plan schema: expected_transactions + test_stimuli

TEST_PLAN_SCHEMA: Dict[str, Any] = {
    "type": "object",
    "additionalProperties": True,
    "required": ["test_stimuli"],
    "properties": {
        "expected_transactions": {
            "type": "array",
            "description": (
                "Optional declaration of the bus transactions "
                "its driver is expected to emit. Merged with the "
                "List derived from expected transactions."
            ),
            "items": {
                "type": "object",
                "additionalProperties": True,
                "required": ["phase"],
                "properties": {
                    "phase":        {"type": "string", "minLength": 1},
                    "addr_or_pin":  {"type": "string"},
                    "write_prefix_any_of": {
                        "type": "array",
                        "items": {
                            "type": "array",
                            "items": {"type": "string"},
                        },
                    },
                    "note":         {"type": "string"},
                },
            },
        },
        "test_stimuli": {
            "type": "array",
            "minItems": 2,
            "description": (
                "Required: at least two stimuli for "
                "runtime_probe to run against its driver. "
                "Used to derive generation-side cross-check and failure "
                "diagnosis."
            ),
            "items": {
                "type": "object",
                "additionalProperties": True,
                "required": ["name", "mock_preload"],
                "properties": {
                    "name":           {"type": "string", "minLength": 1},
                    "mock_preload": {
                        "type": "object",
                        "description": (
                            "Bytes the Renode-side slave must serve when "
                            "the driver reads a register / memory address. "
                            "Keys MUST use ONE of these shapes; any other "
                            "descriptive key is SILENTLY "
                            "DROPPED and the slave will return 0xFF for "
                            "that register. Accepted key shapes: hex register "
                            "literal, decimal register literal, prefixed "
                            "forms, addr:reg pair, or named sentinels "
                            "(`read_bytes`, `stream`, `schedule`, `payload`, "
                            "`frame_ok`, `frame_err`, `status_err`). "
                            "Values MUST be either a list of integers / "
                            "hex-string bytes "
                            "or a JSON-encoded list literal string "
                            "; for GPIO schedule a "
                            "list-of-pairs [[1, 100], [0, 200]]. "
                            "Booleans / nested mappings are ignored; do "
                            "NOT use them as state flags here."
                        ),
                        "patternProperties": {
                            r"^(0x[0-9A-Fa-f]+|\d+|(reg|req|resp)_0x[0-9A-Fa-f]+|"
                            r"0x[0-9A-Fa-f]+:0x[0-9A-Fa-f]+|"
                            r"read_bytes|stream|schedule|payload|"
                            r"frame_ok|frame_err|status|status_err|status_ok|status_zeros)$": {}
                        },
                    },
                    "expected_read_raw":  {
                        "type": "number",
                        "description": (
                            "Exact value the runtime probe should observe from "
                            "drivergen_eval_read_raw_i32 for this stimulus. "
                            "Use the public driver output unit declared by "
                            "the generated adapter/API contract, not an "
                            "intermediate register code. Example: for a "
                            "temperature API returning milli-degrees C, "
                            "25 C must be 25000, not 25 and not the raw "
                            "two-byte register word."
                        ),
                    },
                    "raw_tolerance":      {"type": "number"},
                    "expected_channels":  {"type": "object"},
                    "expected_mem_bytes": {"type": "string"},
                    "expected_time":      {"type": "object"},
                    "expected_frame_err": {"type": "integer"},
                    "expected_err": {
                        "type": "integer",
                        "description": (
                            "Expected nonzero public driver error return for "
                            "runtime-detectable fault/status stimuli. Use 0 "
                            "only when the stimulus is explicitly expected to "
                            "succeed; omit it for normal value-read stimuli."
                        ),
                    },
                    "derivation":         {"type": "string"},
                },
            },
        },
    },
}


# Top-level SYNTHESIS_SCHEMA factory

def synthesis_schema_for(eval_class: str) -> Dict[str, Any]:
    """Return the SYNTHESIS_SCHEMA bound to an eval_class."""
    if eval_class not in API_CONTRACT_BRANCHES:
        raise ValueError(
            f"unknown eval_class {eval_class!r}; "
            f"must be one of {list(EVAL_CLASSES)}"
        )
    return {
        "type": "object",
        "additionalProperties": False,
        "required": ["driver_header", "driver_source",
                      "api_contract", "test_plan"],
        "properties": {
            "driver_header": {
                "type": "string",
                "description": (
                    "Full <device>.h source text (C code). The "
                    "driver is its own translation unit; the header "
                    "may only `#include` items from Section C's "
                    "allow-list plus C stdlib. "
                    "DO NOT reference adapter ABI symbols here "
                    "(`DRIVERGEN_EVAL_OK`, `DRIVERGEN_EVAL_ERR_*`, "
                    "`drivergen_eval_meta_t`, `DRIVERGEN_EVAL_CLASS_*`) "
                    "; those live in `drivergen_eval_adapter.h`, "
                    "which the driver does NOT include. Driver "
                    "functions return plain `int` "
                    "(0 = OK, non-zero = failure); the adapter "
                    "wraps the result for the harness."
                ),
                "minLength": 16,
            },
            "driver_source": {
                "type": "string",
                "description": (
                    "Full <device>.c source text (C code). Same "
                    "scope rules as driver_header: only Section C "
                    "allow-list / C stdlib includes, and adapter "
                    "ABI symbols (`DRIVERGEN_EVAL_OK`, "
                    "`DRIVERGEN_EVAL_ERR_INVALID`, "
                    "`DRIVERGEN_EVAL_ERR_IO`, `DRIVERGEN_EVAL_ERR_NACK`, "
                    "`DRIVERGEN_EVAL_ERR_TIMEOUT`, `DRIVERGEN_EVAL_ERR_CRC`, "
                    "`DRIVERGEN_EVAL_ERR_UNSUPPORTED`, ...) are NOT "
                    "in scope here. If you write `return "
                    "DRIVERGEN_EVAL_ERR_IO;` inside a driver "
                    "function the L1 stub-compile fails with "
                    "`'DRIVERGEN_EVAL_ERR_IO' undeclared`. Driver "
                    "functions MUST return plain `int` (0 = OK, "
                    "non-zero = failure) and the adapter ABI mapping "
                    "is handled by the generated adapter."
                ),
                "minLength": 32,
            },
            "api_contract": API_CONTRACT_BRANCHES[eval_class],
            "test_plan":    TEST_PLAN_SCHEMA,
        },
    }


def contract_test_plan_schema_for(eval_class: str) -> Dict[str, Any]:
    """Return the split planner schema for ``api_contract + test_plan``."""
    if eval_class not in API_CONTRACT_BRANCHES:
        raise ValueError(
            f"unknown eval_class {eval_class!r}; "
            f"must be one of {list(EVAL_CLASSES)}"
        )
    return {
        "type": "object",
        "additionalProperties": False,
        "required": ["api_contract", "test_plan"],
        "properties": {
            "api_contract": API_CONTRACT_BRANCHES[eval_class],
            "test_plan": TEST_PLAN_SCHEMA,
        },
    }


def driver_code_schema() -> Dict[str, Any]:
    """Return the split code-generation schema for driver files only."""
    return {
        "type": "object",
        "additionalProperties": False,
        "required": ["driver_header", "driver_source"],
        "properties": {
            "driver_header": {
                "type": "string",
                "minLength": 16,
                "description": "Full <device>.h source text (C code).",
            },
            "driver_source": {
                "type": "string",
                "minLength": 32,
                "description": "Full <device>.c source text (C code).",
            },
        },
    }


def build_prompt_schema_hint(eval_class: str) -> str:
    """Return a compact schema hint string for prompting."""
    import json
    schema = synthesis_schema_for(eval_class)
    return json.dumps(schema, ensure_ascii=False, indent=2)


def build_plan_schema_hint(eval_class: str) -> str:
    """Return a compact schema hint for the split planner call."""
    import json
    return json.dumps(
        contract_test_plan_schema_for(eval_class), ensure_ascii=False, indent=2
    )


def build_driver_code_schema_hint() -> str:
    """Return a compact schema hint for the split driver-code call."""
    import json
    return json.dumps(driver_code_schema(), ensure_ascii=False, indent=2)


# Validators

def _validate_with_jsonschema(
    data: Mapping[str, Any],
    schema: Mapping[str, Any],
) -> Tuple[bool, List[str], bool]:
    """Validate ``data`` against ``schema`` when jsonschema is available."""
    Validator, _ = _try_import_jsonschema()
    if Validator is None:
        return False, [], False
    errors: List[str] = []
    v = Validator(schema)
    for err in v.iter_errors(data):
        path = ".".join(str(p) for p in err.absolute_path) or "<root>"
        errors.append(f"{path}: {err.message}")
    return (not errors), errors, True


def _try_import_jsonschema():
    """Import jsonschema lazily; return (Draft202012Validator, None) or (None, reason)."""
    try:
        from jsonschema import Draft202012Validator  # type: ignore
        return Draft202012Validator, None
    except Exception as e:   # pragma: no cover - defensive
        return None, f"jsonschema unavailable ({type(e).__name__}: {e})"


def validate_synthesis_response(
    data: Mapping[str, Any],
    eval_class: str,
) -> Tuple[bool, List[str]]:
    """Validate a parsed JSON synthesis response."""
    if not isinstance(data, Mapping):
        return False, [f"response is not a mapping (got {type(data).__name__})"]

    schema = synthesis_schema_for(eval_class)

    ok, errors, used = _validate_with_jsonschema(data, schema)
    if used:
        return ok, errors

    return _fallback_validate(data, eval_class)


def validate_contract_test_plan_response(
    data: Mapping[str, Any],
    eval_class: str,
) -> Tuple[bool, List[str]]:
    """Validate a split planner response."""
    if not isinstance(data, Mapping):
        return False, [f"response is not a mapping (got {type(data).__name__})"]

    schema = contract_test_plan_schema_for(eval_class)
    ok, errors, used = _validate_with_jsonschema(data, schema)
    if used:
        return ok, errors

    extras = sorted(k for k in data.keys() if k not in ("api_contract", "test_plan"))
    if extras:
        return False, [f"<root>: unexpected key(s) {extras!r}"]

    wrapper = {
        "driver_header": "/* placeholder */\n",
        "driver_source": "/* placeholder placeholder placeholder */\n",
        "api_contract": data.get("api_contract"),
        "test_plan": data.get("test_plan"),
    }
    return _fallback_validate(wrapper, eval_class)


def validate_driver_code_response(
    data: Mapping[str, Any],
) -> Tuple[bool, List[str]]:
    """Validate a split driver-code response."""
    if not isinstance(data, Mapping):
        return False, [f"response is not a mapping (got {type(data).__name__})"]

    schema = driver_code_schema()
    ok, errors, used = _validate_with_jsonschema(data, schema)
    if used:
        return ok, errors

    errors = []
    for key, min_len in (("driver_header", 16), ("driver_source", 32)):
        value = data.get(key)
        if not isinstance(value, str):
            errors.append(f"{key}: must be a string")
        elif len(value) < min_len:
            errors.append(f"{key}: must be at least {min_len} characters")
    extras = sorted(k for k in data.keys() if k not in ("driver_header", "driver_source"))
    if extras:
        errors.append(f"<root>: unexpected key(s) {extras!r}")
    return (not errors), errors


def _fallback_validate(
    data: Mapping[str, Any],
    eval_class: str,
) -> Tuple[bool, List[str]]:
    """Minimal Python-only validator for environments without jsonschema."""
    errors: List[str] = []
    top_required = ("driver_header", "driver_source",
                     "api_contract", "test_plan")
    for k in top_required:
        if k not in data:
            errors.append(f"<root>: missing required key {k!r}")

    if "api_contract" in data:
        ac = data["api_contract"]
        if not isinstance(ac, Mapping):
            errors.append("api_contract: must be an object")
        else:
            if ac.get("eval_class") not in (eval_class, None):
                errors.append(
                    f"api_contract.eval_class: expected {eval_class!r}, "
                    f"got {ac.get('eval_class')!r}"
                )
            branch = API_CONTRACT_BRANCHES.get(eval_class)
            if isinstance(branch, Mapping):
                for k in branch.get("required", []):
                    if k not in ac:
                        errors.append(
                            f"api_contract: missing required key {k!r}"
                        )

    tp = data.get("test_plan")
    if tp is not None:
        if not isinstance(tp, Mapping):
            errors.append("test_plan: must be an object")
        else:
            stim = tp.get("test_stimuli")
            if not isinstance(stim, list) or len(stim) < 2:
                errors.append(
                    "test_plan.test_stimuli: must be an array of length >= 2"
                )
            else:
                for i, s in enumerate(stim):
                    if not isinstance(s, Mapping):
                        errors.append(
                            f"test_plan.test_stimuli[{i}]: must be an object"
                        )
                        continue
                    for k in ("name", "mock_preload"):
                        if k not in s:
                            errors.append(
                                f"test_plan.test_stimuli[{i}]: "
                                f"missing required key {k!r}"
                            )

    return (not errors), errors


__all__ = [
    "API_CONTRACT_BRANCHES",
    "API_CONTRACT_DISPLAY",
    "API_CONTRACT_MEMORY",
    "API_CONTRACT_MULTI_CHANNEL",
    "API_CONTRACT_RTC",
    "API_CONTRACT_SINGLE_CHANNEL",
    "TEST_PLAN_SCHEMA",
    "build_driver_code_schema_hint",
    "build_plan_schema_hint",
    "build_prompt_schema_hint",
    "contract_test_plan_schema_for",
    "driver_code_schema",
    "synthesis_schema_for",
    "validate_contract_test_plan_response",
    "validate_driver_code_response",
    "validate_synthesis_response",
]
