## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [compile-includes] `#include "gpio_if.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "hcsr04.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "osal_time.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write <empty> to addr gpio1 (attempt 1, evidence: eval_class=single_channel)
- [init-opcode] phase=read_cycle requires write <read> to addr gpio1 (attempt 1, evidence: read_sequence)

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - <RUN_DIR>/attempts/001/stage/out/stage/hcsr04_eval_adapter.c:34:46: error: 'gpio1' undeclared (first use in this function)
  - <RUN_DIR>/attempts/001/stage/out/stage/hcsr04_eval_adapter.c:34:53: error: 'gpio2' undeclared (first use in this function)

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
