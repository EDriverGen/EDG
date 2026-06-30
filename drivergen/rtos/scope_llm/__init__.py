"""LLM-driven directory-level scope triage for RTOS source trees."""

from .triage import PROMPT_VERSION, RoleSpec, ROLES, run_triage_for_role
from .synthesizer import (
    SCOPE_LLM_CACHE_DIR,
    cache_key_for,
    cache_path_for,
    load_or_synthesize_scope_map,
    synthesize_scope_map_entry,
)

__all__ = [
    "PROMPT_VERSION",
    "RoleSpec",
    "ROLES",
    "SCOPE_LLM_CACHE_DIR",
    "cache_key_for",
    "cache_path_for",
    "run_triage_for_role",
    "load_or_synthesize_scope_map",
    "synthesize_scope_map_entry",
]
