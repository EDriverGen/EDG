# Evaluation Stub Roots

These per-RTOS directories provide the minimal headers, symbols, and weak
implementations needed by the evaluation harness. They are used only for
compiling and linking generated drivers in the simulator.

The stubs do not define the RTOS contract. Driver generation should rely on
the RTOS contract extracted from the manifest-tracked source repositories.

| File | Role |
| --- | --- |
| `stubs.c`, `stubs_*.c` | Bus and runtime implementations used by the harness. |
| `*.h` | Header shims for generated and reference drivers. |
| `task_package_helpers.c` | Weak placeholders for task-level helper hooks. |

Keep stubs minimal and bus-oriented. Add a new stub only when the evaluation
harness needs a link-time symbol that is not provided elsewhere.
