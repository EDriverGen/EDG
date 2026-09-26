## ATTEMPT 3 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [compile-includes] `#include "riot.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "sht30.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "xtimer.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <periph/i2c.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0x00, 0x06] to addr 0x44 (attempt 2, evidence: operation_flows[6].steps[0])
- [init-opcode] init phase must write [0x30, 0xA2] to addr 0x44 (attempt 2, evidence: operation_flows[5].steps[0])
- [init-opcode] phase=read_cycle requires write <read> to addr 0x44 (attempt 2, evidence: operation_flows[0].steps[2])
- [init-opcode] phase=read_cycle requires write [0x24, 0x00] to addr 0x44 (attempt 2, evidence: operation_flows[0].steps[0])

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - driver_source uses scaled conversion coefficient(s) 100000, 175000 from Device IR milli/micro-unit formulae without any `int64_t`, `uint64_t`, or `long long` intermediate. Promote the raw-code multiplication/division to a 64-bit intermediate before assigning back to the public output type; 16-bit or wider raw fields can overflow `int32_t` during `raw * scale` even when the final result fits.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
