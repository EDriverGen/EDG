"""JSON repair and fallback prompts."""
from __future__ import annotations

import json
from typing import Any, Mapping, Sequence, Tuple


# ---------------------------------------------------------------------------
# Shared SYSTEM constants.
# ---------------------------------------------------------------------------

ROLE = (
    "You are a JSON repair tool. Your sole job is to convert the supplied "
    "malformed model output into one JSON value that conforms to the "
    "target JSON schema. You do not invent device facts, you do not "
    "explain your work, and you do not add any prose around the JSON."
)

OUTPUT_SHAPE = (
    "Return exactly 1 JSON value (not a string, not a markdown fence, not "
    "an HTTP envelope). UTF-8 only. No trailing commas. The value must "
    "validate against the target schema you are given. Top-level keys "
    "MUST come from the schema's required list and from its named "
    "properties — do not invent additional keys."
)

HARD_RULES: Tuple[str, ...] = (
    # Rule 1 — preserve field names verbatim
    'Hard rule 1 — schema field names are verbatim: Use the exact keys '
    'declared in the target schema (case-sensitive). Do not rename, '
    'pluralise, abbreviate, snake_case-vs-camelCase swap, or "fix the '
    'spelling" of any key. If the malformed answer used a different '
    'name for a field that maps to one of the schema keys, copy the '
    'value over under the schema key. Counter-example: schema requires '
    '"device_id" but the response emits "deviceId" or "id" — wrong, '
    'downstream callers index by exact key.',

    # Rule 2 — preserve original values when valid
    'Hard rule 2 — keep original values when valid: When a value in the '
    'malformed answer is already correctly-typed for the target schema '
    '(string for a string field, integer for an integer field, etc.), '
    'copy it over verbatim. Do not normalise, summarise, or reformat '
    'valid values. Counter-example: rewriting an evidence snippet from '
    '"I²C-bus interface" to "I2C bus interface" because the unicode is '
    '"prettier" — wrong, downstream substring grounding fails.',

    # Rule 3 — fill required fields, no fabrication
    'Hard rule 3 — fill required fields without fabrication: Every '
    'field listed in schema.required MUST appear in the output. If the '
    'malformed answer does not provide a value, prefer an empty value of '
    'the right type ([] for arrays, {} for objects, "" for strings, 0 '
    'for integers when the schema accepts it). Do NOT invent device '
    'facts, page numbers, addresses, or formula text. Counter-example: '
    'making up "address": "0x42" because the malformed answer omitted '
    'the register address — wrong, that fabrication will fail evidence '
    'validation.',

    # Rule 4 — output is JSON only
    'Hard rule 4 — JSON only, no narration: Do NOT prepend "Here is the '
    'JSON:" or wrap the value in ```json ... ``` fences. Do NOT append '
    'commentary, change logs, or "I noticed the input was malformed in '
    'X way" notes. The first character of your response must be "{", '
    '"[", a JSON literal (true/false/null), or a JSON-escaped string '
    'opening quote. Counter-example: wrapping the JSON in a markdown '
    'fence (wrong — most parsers will reject that).',
)

NEGATIVE_EXAMPLE = (
    "Negative example (DO NOT EMIT):\n"
    "  --- malformed input ---\n"
    "  Sure! Here is the device IR you asked for:\n"
    "  ```json\n"
    "  { \"deviceId\": \"example_device\", \"bus\": \"I2C\", "
    "    \"registers\": [{\"addr\": \"0x00\", \"name\": \"Temp\"}] }\n"
    "  ```\n"
    "  --- bad rewrite ---\n"
    "  ```json\n"
    "  { \"device_id\": \"example_device\", \"bus_type\": \"I2C\", "
    "    \"registers_or_commands\": "
    "    [{\"name\": \"Temp\", \"address\": \"0x00\"}] }\n"
    "  ```\n"
    "  --- why bad ---\n"
    "  Almost right but: (a) bus_type should be lowercase 'i2c' (Hard "
    "  rule 2 keeps valid values, but here the schema enum is lowercase "
    "  so the valid form is 'i2c'); (b) the markdown fence around the "
    "  JSON breaks Hard rule 4. Emit the raw JSON object only."
)


# ---------------------------------------------------------------------------
# Fallback-only extra guidance (last-chance rescue).
# ---------------------------------------------------------------------------

FALLBACK_PREAMBLE = (
    "This is a last-chance rescue: the first repair attempt already "
    "failed JSON-schema validation. Stay strictly inside the target "
    "schema and prefer empty values to fabrication when a required "
    "field has no support in the malformed answer."
)


# ---------------------------------------------------------------------------
# Builders.
# ---------------------------------------------------------------------------


def _build_system_prompt(*, fallback: bool) -> str:
    parts = [ROLE, OUTPUT_SHAPE]
    if fallback:
        parts.append(FALLBACK_PREAMBLE)
    parts.append("Hard rules:\n" + "\n".join(f"- {r}" for r in HARD_RULES))
    parts.append(NEGATIVE_EXAMPLE)
    return "\n\n".join(parts)


def _build_user_prompt(
    *,
    task_name: str,
    schema: Mapping[str, Any],
    invalid_output: str,
    system_prompt: str,
    user_prompt: str,
) -> str:
    required = schema.get("required") if isinstance(schema, Mapping) else None
    return (
        f"Task name: {task_name}\n\n"
        f"Required top-level keys: "
        f"{json.dumps(list(required) if isinstance(required, Sequence) and not isinstance(required, (str, bytes)) else [], ensure_ascii=False)}\n\n"
        "Target schema (the rewritten JSON value MUST validate against "
        "this schema; field names are case-sensitive):\n"
        f"{json.dumps(dict(schema) if isinstance(schema, Mapping) else schema, ensure_ascii=False, indent=2)}\n\n"
        "Original task SYSTEM prompt (context only — do NOT echo it back):\n"
        f"{system_prompt}\n\n"
        "Original task USER prompt (context only — do NOT echo it back):\n"
        f"{user_prompt}\n\n"
        "Malformed answer to repair (rewrite this into one schema-"
        "conformant JSON value):\n"
        f"{invalid_output}"
    )


def build_repair_prompt(
    *,
    task_name: str,
    schema: Mapping[str, Any],
    invalid_output: str,
    system_prompt: str,
    user_prompt: str,
) -> Tuple[str, str]:
    """Primary repair pass — first time we ask the model to fix its output."""
    return (
        _build_system_prompt(fallback=False),
        _build_user_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        ),
    )


def build_fallback_prompt(
    *,
    task_name: str,
    schema: Mapping[str, Any],
    invalid_output: str,
    system_prompt: str,
    user_prompt: str,
) -> Tuple[str, str]:
    """Last-chance rescue pass — adds the FALLBACK_PREAMBLE."""
    return (
        _build_system_prompt(fallback=True),
        _build_user_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        ),
    )


__all__ = [
    "ROLE",
    "OUTPUT_SHAPE",
    "HARD_RULES",
    "NEGATIVE_EXAMPLE",
    "FALLBACK_PREAMBLE",
    "build_repair_prompt",
    "build_fallback_prompt",
]
