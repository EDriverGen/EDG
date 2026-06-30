# `drivergen/rtos/config/`

Static configuration for RTOS context extraction.

| File | Purpose |
| --- | --- |
| `thresholds.json` | Shared limits, budgets, cache settings, and scoring weights. |
| `bus_taxonomy.json` | Maps connection types to canonical bus metadata. |
| `slot_templates/` | Base slot templates for supported connection types. |
| `scope_map/_llm_cache/` | Cached ScopeMaps produced by the scope synthesis flow. |

Configuration loaders live in `drivergen.rtos.config`.
