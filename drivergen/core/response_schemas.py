def evidence_span_schema() -> dict:
    return {
        "type": "object",
        "additionalProperties": False,
        "required": ["source_id", "page", "snippet"],
        "properties": {
            "source_id": {"type": "string"},
            "page": {"type": "integer"},
            "snippet": {"type": "string"},
        },
    }


# Shared schema fragments for DEVICE_IR_SCHEMA.

# IR-A: flat register-map descriptor for memory-style devices (AT24Cxx / FRAM
# / config-EEPROM) and pointer-style register banks (LM75, DS3231, …).
REGISTER_MAP_SCHEMA = {
    "anyOf": [
        {"type": "null"},
        {
            "type": "object",
            "additionalProperties": False,
            "required": ["total_size_bytes", "addressing", "auto_increment"],
            "properties": {
                "total_size_bytes": {"type": "integer", "minimum": 1},
                "addressing": {
                    "type": "string",
                    "enum": ["1-byte", "2-byte", "pointer", "command_opcode"],
                },
                "auto_increment": {"type": "boolean"},
                "page_size_bytes": {"type": ["integer", "null"], "minimum": 1},
                "write_busy_ms": {"type": ["number", "null"], "minimum": 0},
                "notes": {"type": ["string", "null"]},
            },
        },
    ]
}

# IR-B: machine-readable readable channels.
READ_CHANNELS_SCHEMA = {
    "type": "array",
    "minItems": 1,
    "items": {
        "type": "object",
        "additionalProperties": False,
        "required": ["id", "raw_type", "physical_unit"],
        "properties": {
            "id": {"type": "string", "minLength": 1},
            "raw_type": {
                "type": "string",
                "enum": [
                    "uint8", "int8",
                    "uint16", "int16",
                    "uint32", "int32",
                    "float", "bytes",
                ],
            },
            "physical_unit": {"type": "string"},
            "read_call_hint": {"type": ["string", "null"]},
            "flow_id": {"type": ["string", "null"]},
            "source_bytes": {
                "anyOf": [
                    {"type": "null"},
                    {"type": "array", "items": {"type": "string"}},
                ],
            },
            "source_signal": {"type": ["string", "null"]},
            "formula_id": {"type": ["string", "null"]},
            "notes": {"type": ["string", "null"]},
        },
    },
}

# IR-D: raw byte-level READ layout for rebuild and sign-extension checks.
RAW_ENCODING_SCHEMA = {
    "anyOf": [
        {"type": "null"},
        {
            "type": "object",
            "additionalProperties": False,
            "required": ["byte_order", "bit_width", "signed"],
            "properties": {
                "byte_order": {
                    "type": "string",
                    "enum": ["big_endian", "little_endian", "single_byte"],
                },
                "bit_width": {"type": "integer", "minimum": 1, "maximum": 64},
                "effective_bits": {
                    "type": ["integer", "null"],
                    "minimum": 1,
                    "maximum": 64,
                },
                "signed": {"type": "boolean"},
                "right_shift": {
                    "type": ["integer", "null"],
                    "minimum": 0,
                    "maximum": 63,
                },
                "sign_extend_from_bit": {
                    "type": ["integer", "null"],
                    "minimum": 0,
                    "maximum": 63,
                },
                "notes": {"type": ["string", "null"]},
            },
        },
    ]
}

# IR-E: machine-readable transaction hint for one sequence step.
SEQUENCE_TRANSACTION_KINDS = ("write", "read", "write_then_read")

SEQUENCE_TRANSACTION_SCHEMA = {
    "anyOf": [
        {"type": "null"},
        {
            "type": "object",
            "additionalProperties": False,
            "required": ["kind"],
            "properties": {
                "kind": {
                    "type": "string",
                    "enum": list(SEQUENCE_TRANSACTION_KINDS),
                },
                "bytes": {
                    "anyOf": [
                        {"type": "null"},
                        {
                            "type": "array",
                            "items": {
                                "oneOf": [
                                    {"type": "integer", "minimum": 0, "maximum": 255},
                                    {
                                        "type": "string",
                                        "pattern": (
                                            r"^(DATA|0x[0-9A-Fa-f]{1,2})$"
                                        ),
                                    },
                                    {"type": "null"},
                                ],
                            },
                        },
                    ],
                },
                "length": {
                    "anyOf": [
                        {"type": "null"},
                        {"type": "integer", "minimum": 1, "maximum": 65535},
                        {"type": "string", "pattern": r"^DATA$"},
                    ],
                },
                "pointer_target": {"type": ["string", "null"]},
                "notes": {"type": ["string", "null"]},
            },
        },
    ]
}


# Bus access model and operation flows. These fields are optional so partial
# IRs can still be validated, but should be populated when datasheet evidence
# is available.
ACCESS_MODEL_SCHEMA = {
    "anyOf": [
        {"type": "null"},
        {
            "type": "object",
            "additionalProperties": False,
            "required": ["kind"],
            "properties": {
                "kind": {
                    "type": "string",
                    "enum": [
                        "register_pointer",
                        "register_auto_increment",
                        "command_then_direct_read",
                        "memory",
                        "stream",
                        "packet",
                        "gpio_timing",
                        "unknown",
                    ],
                },
                "address_bytes": {
                    "anyOf": [
                        {"type": "null"},
                        {"type": "integer", "minimum": 0, "maximum": 4},
                    ],
                },
                "read_requires_pointer": {"type": ["boolean", "null"]},
                "direct_read_after_write": {"type": ["boolean", "null"]},
                "notes": {"type": ["string", "null"]},
            },
        },
    ],
}


OPERATION_FLOW_STEP_OPS = (
    "write",
    "read",
    "write_then_read",
    "delay",
    "poll_until",
    "wait_until_ready",
    "select_page",
    "clear",
    "postprocess",
    "set_signal",
    "wait_signal",
    "measure_pulse",
    "sample_signal",
)

OPERATION_FLOW_STEP_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["op"],
    "properties": {
        "op": {"type": "string", "enum": list(OPERATION_FLOW_STEP_OPS)},
        "role": {"type": ["string", "null"]},
        "transaction": SEQUENCE_TRANSACTION_SCHEMA,
        "register": {"type": ["string", "null"]},
        "mask": {"type": ["string", "null"]},
        "value": {"type": ["string", "null"]},
        "condition": {"type": ["string", "null"]},
        "timeout_ms": {"type": ["number", "null"], "minimum": 0},
        "interval_ms": {"type": ["number", "null"], "minimum": 0},
        "delay_ms": {"type": ["number", "null"], "minimum": 0},
        "length": {
            "anyOf": [
                {"type": "null"},
                {"type": "integer", "minimum": 1, "maximum": 65535},
                {"type": "string", "pattern": r"^DATA$"},
            ],
        },
        "signal": {"type": ["string", "null"]},
        "source_signal": {"type": ["string", "null"]},
        "edge": {"type": ["string", "null"]},
        "duration_us": {"type": ["number", "null"], "minimum": 0},
        "notes": {"type": ["string", "null"]},
        "output_ref": {"type": ["string", "null"]},
    },
}

OPERATION_FLOW_OUTPUT_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["channel"],
    "properties": {
        "channel": {"type": "string"},
        "byte_source": {"type": ["string", "null"]},
        "source_signal": {"type": ["string", "null"]},
        "formula_id": {"type": ["string", "null"]},
        "unit": {"type": ["string", "null"]},
        "notes": {"type": ["string", "null"]},
    },
}

OPERATION_FLOWS_SCHEMA = {
    "type": "array",
    "items": {
        "type": "object",
        "additionalProperties": False,
        "required": ["flow_id", "kind", "channels", "steps", "outputs"],
        "properties": {
            "flow_id": {"type": "string", "minLength": 1},
            "kind": {
                "type": "string",
                "enum": [
                    "init",
                    "probe",
                    "read",
                    "calibration",
                    "write",
                    "power",
                    "other",
                ],
            },
            "channels": {"type": "array", "items": {"type": "string"}},
            "preconditions": {"type": "array", "items": {"type": "string"}},
            "steps": {"type": "array", "items": OPERATION_FLOW_STEP_SCHEMA},
            "outputs": {"type": "array", "items": OPERATION_FLOW_OUTPUT_SCHEMA},
            "requires_human": {"type": ["boolean", "null"]},
            "notes": {"type": ["string", "null"]},
        },
    },
}


# Machine-readable companion to the prose integer approximation.
INTEGER_APPROXIMATION_EXPRESSION_SCHEMA = {
    "anyOf": [
        {"type": "null"},
        {
            "type": "object",
            "additionalProperties": False,
            "required": ["expression", "inputs", "output"],
            "properties": {
                "expression": {"type": "string", "minLength": 1, "maxLength": 512},
                "inputs": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "additionalProperties": False,
                        "required": ["name"],
                        "properties": {
                            "name": {"type": "string", "minLength": 1},
                            "byte_source": {"type": ["string", "null"]},
                            "source_signal": {"type": ["string", "null"]},
                            "default_value": {
                                "type": ["string", "number", "boolean", "null"],
                            },
                            "config_source": {"type": ["string", "null"]},
                            "description": {"type": ["string", "null"]},
                        },
                    },
                },
                "output": {
                    "type": "object",
                    "additionalProperties": False,
                    "required": ["name"],
                    "properties": {
                        "name": {"type": "string", "minLength": 1},
                        "unit": {"type": ["string", "null"]},
                    },
                },
            },
        },
    ]
}


# Bump when downstream consumers must detect a Device IR schema change.
IR_SCHEMA_VERSION = "2026-05-05.ire-flow-p2+signals+completion"


DEVICE_IR_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "device_id",
        "bus_type",
        "address_rule",
        "register_map",
        "read_channels",
        "raw_encoding",
        "registers_or_commands",
        "bitfields",
        "init_sequence",
        "read_sequence",
        "timing_constraints",
        "conversion_formulae",
        "error_conditions",
        "power_states",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "device_id": {"type": "string"},
        "bus_type": {"type": "string"},
        "address_rule": {"type": "object"},
        "register_map": REGISTER_MAP_SCHEMA,
        "read_channels": READ_CHANNELS_SCHEMA,
        "access_model": ACCESS_MODEL_SCHEMA,
        "operation_flows": OPERATION_FLOWS_SCHEMA,
        "raw_encoding": RAW_ENCODING_SCHEMA,
        "registers_or_commands": {"type": "array", "items": {"type": "object"}},
        "bitfields": {"type": "array", "items": {"type": "object"}},
        "init_sequence": {
            "type": "array",
            "items": {
                "type": "object",
                # Preserve existing row fields while validating transaction if present.
                "properties": {
                    "transaction": SEQUENCE_TRANSACTION_SCHEMA,
                },
            },
        },
        "read_sequence": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "transaction": SEQUENCE_TRANSACTION_SCHEMA,
                },
            },
        },
        "timing_constraints": {"type": "array", "items": {"type": "object"}},
        "conversion_formulae": {
            "type": "array",
            "items": {
                "type": "object",
                # Preserve existing formula fields and validate the machine form.
                "required": ["integer_approximation_expression"],
                "properties": {
                    "integer_approximation_expression": (
                        INTEGER_APPROXIMATION_EXPRESSION_SCHEMA
                    ),
                },
            },
        },
        "error_conditions": {"type": "array", "items": {"type": "object"}},
        "power_states": {"type": "array", "items": {"type": "object"}},
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
        # Optional companion field; not in ``required`` because many
        # fixtures/tests don't populate it. run_pipeline() unconditionally
        # stamps ``IR_SCHEMA_VERSION`` onto finalised payloads.
        "ir_schema_version": {"type": ["string", "null"]},
    },
}


DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["page", "snippet"],
    "properties": {
        "page": {"type": ["integer", "null"]},
        "snippet": {"type": "string"},
        "source_id": {"type": ["string", "null"]},
    },
}


DEVICE_IR_FACT_CANDIDATE_STEP_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["op", "target", "details", "evidence"],
    "properties": {
        "op": {
            "type": "string",
            "enum": [
                "write",
                "read",
                "write_then_read",
                "delay",
                "poll_until",
                "wait_until_ready",
                "select_page",
                "clear",
                "postprocess",
                "set_signal",
                "wait_signal",
                "measure_pulse",
                "sample_signal",
                "other",
            ],
        },
        "target": {"type": ["string", "null"]},
        "details": {"type": "string"},
        "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
    },
}


DEVICE_IR_FACT_CANDIDATES_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "device_id",
        "bus_type",
        "candidate_addresses",
        "candidate_registers",
        "candidate_bitfields",
        "candidate_operations",
        "candidate_channels",
        "candidate_formulae",
        "candidate_timing_constraints",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "device_id": {"type": "string"},
        "bus_type": {"type": ["string", "null"]},
        "candidate_addresses": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["value", "addressing_form", "description", "evidence"],
                "properties": {
                    "value": {"type": ["string", "null"]},
                    "addressing_form": {"type": ["string", "null"]},
                    "is_default": {"type": ["boolean", "null"]},
                    "description": {"type": "string"},
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_registers": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["name", "value", "access", "size_bits", "description", "semantic_roles", "evidence"],
                "properties": {
                    "name": {"type": "string"},
                    "value": {"type": ["string", "null"]},
                    "access": {"type": ["string", "null"]},
                    "size_bits": {"type": ["integer", "null"], "minimum": 1},
                    "description": {"type": "string"},
                    "semantic_roles": {
                        "type": "array",
                        "items": {
                            "type": "string",
                            "enum": [
                                "identity",
                                "status",
                                "result",
                                "data",
                                "config",
                                "control",
                                "coefficient",
                                "trigger",
                                "clear",
                                "threshold",
                                "other",
                            ],
                        },
                    },
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_bitfields": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["register", "name", "bit_range", "description", "semantic_roles", "evidence"],
                "properties": {
                    "register": {"type": ["string", "null"]},
                    "name": {"type": "string"},
                    "bit_range": {"type": ["string", "null"]},
                    "reset_value": {"type": ["string", "null"]},
                    "description": {"type": "string"},
                    "semantic_roles": {
                        "type": "array",
                        "items": {
                            "type": "string",
                            "enum": [
                                "identity",
                                "status",
                                "result",
                                "data",
                                "config",
                                "control",
                                "coefficient",
                                "trigger",
                                "clear",
                                "threshold",
                                "other",
                            ],
                        },
                    },
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_operations": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["flow_id", "kind", "summary", "steps", "outputs", "evidence"],
                "properties": {
                    "flow_id": {"type": "string"},
                    "kind": {
                        "type": "string",
                        "enum": ["init", "probe", "read", "calibration", "write", "power", "other"],
                    },
                    "channels": {"type": "array", "items": {"type": "string"}},
                    "summary": {"type": "string"},
                    "steps": {
                        "type": "array",
                        "items": DEVICE_IR_FACT_CANDIDATE_STEP_SCHEMA,
                    },
                    "outputs": {"type": "array", "items": {"type": "string"}},
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_channels": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["id", "description", "source", "formula_id", "evidence"],
                "properties": {
                    "id": {"type": "string"},
                    "description": {"type": "string"},
                    "source": {"type": ["string", "null"]},
                    "formula_id": {"type": ["string", "null"]},
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_formulae": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["name", "formula", "inputs", "output", "evidence"],
                "properties": {
                    "name": {"type": "string"},
                    "formula": {"type": "string"},
                    "inputs": {"type": "array", "items": {"type": "string"}},
                    "output": {"type": ["string", "null"]},
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "candidate_timing_constraints": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["name", "value", "unit", "condition", "evidence"],
                "properties": {
                    "name": {"type": "string"},
                    "value": {"type": ["number", "string", "null"]},
                    "unit": {"type": ["string", "null"]},
                    "condition": {"type": "string"},
                    "evidence": DEVICE_IR_FACT_CANDIDATE_EVIDENCE_SCHEMA,
                },
            },
        },
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
    },
}


DEVICE_IR_EVIDENCE_REPAIR_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["evidence_spans"],
    "properties": {
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
    },
}


DEVICE_IR_FLOW_AUDIT_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["audit_findings", "device_ir"],
    "properties": {
        "audit_findings": {
            "type": "array",
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["flow_id", "summary", "checks"],
                "properties": {
                    "flow_id": {"type": "string"},
                    "summary": {"type": "string"},
                    "checks": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "additionalProperties": False,
                            "required": ["name", "status", "evidence", "action"],
                            "properties": {
                                "name": {
                                    "type": "string",
                                    "enum": [
                                        "trigger_or_mode",
                                        "wait_or_poll",
                                        "result_read",
                                        "clear_or_ack",
                                        "config_or_calibration",
                                        "byte_source",
                                    ],
                                },
                                "status": {
                                    "type": "string",
                                    "enum": [
                                        "present",
                                        "not_required",
                                        "missing_required",
                                        "uncertain",
                                    ],
                                },
                                "evidence": {"type": "string"},
                                "action": {"type": "string"},
                            },
                        },
                    },
                },
            },
        },
        "device_ir": DEVICE_IR_SCHEMA,
    },
}

KERNEL_PROFILE_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "rtos",
        "board",
        "integration",
        "runtime_symbols",
        "delay_symbols",
        "error_symbols",
        "header_sources",
        "build_sources",
        "notes",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "rtos": {"type": "string"},
        "board": {"type": "string"},
        "integration": {"type": ["string", "null"]},
        "runtime_symbols": {"type": "array", "items": {"type": "string"}},
        "delay_symbols": {"type": "array", "items": {"type": "string"}},
        "error_symbols": {"type": "array", "items": {"type": "string"}},
        "header_sources": {"type": "array", "items": {"type": "string"}},
        "build_sources": {"type": "array", "items": {"type": "string"}},
        "notes": {"type": "array", "items": {"type": "string"}},
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
    },
}


DRIVER_FRAMEWORK_PROFILE_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "rtos",
        "board",
        "bus_type",
        "bus_api_symbols",
        "bus_helper_types",
        "transaction_patterns",
        "sample_symbols",
        "framework_sources",
        "exemplar_sources",
        "forbidden_assumptions",
        "notes",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "rtos": {"type": "string"},
        "board": {"type": "string"},
        "bus_type": {"type": ["string", "null"]},
        "bus_api_symbols": {"type": "array", "items": {"type": "string"}},
        "bus_helper_types": {"type": "array", "items": {"type": "string"}},
        "transaction_patterns": {"type": "array", "items": {"type": "string"}},
        "sample_symbols": {"type": "array", "items": {"type": "string"}},
        "framework_sources": {"type": "array", "items": {"type": "string"}},
        "exemplar_sources": {"type": "array", "items": {"type": "string"}},
        "forbidden_assumptions": {"type": "array", "items": {"type": "string"}},
        "notes": {"type": "array", "items": {"type": "string"}},
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
    },
}


BOARD_INTEGRATION_PROFILE_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "rtos",
        "board",
        "integration",
        "connection_type",
        "mode",
        "bus_binding",
        "runtime_assumptions",
        "board_sources",
        "attachment_hints",
        "notes",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "rtos": {"type": "string"},
        "board": {"type": "string"},
        "integration": {"type": ["string", "null"]},
        "connection_type": {"type": ["string", "null"]},
        "mode": {"type": ["string", "null"]},
        "bus_binding": {"type": "object"},
        "runtime_assumptions": {"type": "object"},
        "board_sources": {"type": "array", "items": {"type": "string"}},
        "attachment_hints": {"type": "object"},
        "notes": {"type": "array", "items": {"type": "string"}},
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
    },
}


RTOS_CONTRACT_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": [
        "contract_version",
        "task_package_id",
        "device_id",
        "rtos",
        "board",
        "bus_type",
        "connection",
        "runtime_contract",
        "bus_contract",
        "integration_contract",
        "device_contract",
        "allowed_symbols",
        "forbidden_assumptions",
        "notes",
        "evidence_spans",
        "requires_human",
    ],
    "properties": {
        "contract_version": {"type": "string"},
        "task_package_id": {"type": ["string", "null"]},
        "device_id": {"type": "string"},
        "rtos": {"type": "string"},
        "board": {"type": "string"},
        "bus_type": {"type": ["string", "null"]},
        "connection": {"type": "object"},
        "runtime_contract": {"type": "object"},
        "bus_contract": {"type": "object"},
        "integration_contract": {"type": "object"},
        "device_contract": {"type": "object"},
        "allowed_symbols": {"type": "array", "items": {"type": "string"}},
        "forbidden_assumptions": {"type": "array", "items": {"type": "string"}},
        "notes": {"type": "array", "items": {"type": "string"}},
        "evidence_spans": {"type": "array", "items": evidence_span_schema()},
        "requires_human": {"type": "array", "items": {"type": "string"}},
    },
}


# Typed inventory schema.
TYPED_INVENTORY_SCHEMA = {
    "type": "object",
    "additionalProperties": False,
    "required": ["artifacts", "symbols", "patterns", "conflicts"],
    "properties": {
        "artifacts": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["source_id", "category", "role", "authority_score"],
                "properties": {
                    "source_id": {"type": "string"},
                    "path": {"type": "string"},
                    "category": {"type": "string"},
                    "role": {"type": "string"},
                    "authority_score": {"type": "number"},
                    "task_match_score": {"type": "number"},
                    "matched_needles": {"type": "array", "items": {"type": "string"}},
                    "hit_count": {"type": "integer"},
                    "snippet": {"type": "string"},
                },
            },
        },
        "symbols": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["symbol_id", "name", "kind"],
                "properties": {
                    "symbol_id": {"type": "string"},
                    "name": {"type": "string"},
                    "kind": {"type": "string"},
                    "declared_in": {"type": "string"},
                    "defined_in": {"type": "string"},
                    "signature_text": {"type": "string"},
                    "owner_confidence": {"type": "number"},
                },
            },
        },
        "patterns": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["pattern_id", "pattern_name"],
                "properties": {
                    "pattern_id": {"type": "string"},
                    "pattern_name": {"type": "string"},
                    "source_id": {"type": "string"},
                    "trigger_symbols": {"type": "array", "items": {"type": "string"}},
                    "semantic_summary": {"type": "string"},
                    "confidence": {"type": "number"},
                },
            },
        },
        "conflicts": {
            "type": "array",
            "items": {
                "type": "object",
                "required": ["conflict_id", "preferred_source", "losing_source"],
                "properties": {
                    "conflict_id": {"type": "string"},
                    "fact_description": {"type": "string"},
                    "preferred_source": {"type": "string"},
                    "preferred_category": {"type": "string"},
                    "losing_source": {"type": "string"},
                    "losing_category": {"type": "string"},
                    "conflict_type": {"type": "string"},
                    "requires_human": {"type": "boolean"},
                },
            },
        },
    },
}
