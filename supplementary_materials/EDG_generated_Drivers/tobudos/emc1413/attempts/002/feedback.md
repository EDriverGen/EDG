## ATTEMPT 2 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [compile-includes] `#include "emc1413.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "stm32f1xx_hal.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "tobudos.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "tos_k.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0x03, 0x00] to addr 0x4C (attempt 1, evidence: operation_flows[1].steps[1])
- [init-opcode] init phase must write [0x04, 0x06] to addr 0x4C (attempt 1, evidence: operation_flows[1].steps[2])
- [init-opcode] init phase must write [0xFD] to addr 0x4C (attempt 1, evidence: operation_flows[0].steps[1])
- [init-opcode] init phase must write [0xFE] to addr 0x4C (attempt 1, evidence: operation_flows[0].steps[0])
- [init-opcode] phase=read_cycle requires write [0x00] to addr 0x4C (attempt 1, evidence: operation_flows[2].steps[0])
- [init-opcode] phase=read_cycle requires write [0x01] to addr 0x4C (attempt 1, evidence: operation_flows[3].steps[0])
- [init-opcode] phase=read_cycle requires write [0x10] to addr 0x4C (attempt 1, evidence: operation_flows[3].steps[1])
- [init-opcode] phase=read_cycle requires write [0x23] to addr 0x4C (attempt 1, evidence: operation_flows[4].steps[0])
- [init-opcode] phase=read_cycle requires write [0x24] to addr 0x4C (attempt 1, evidence: operation_flows[4].steps[1])
- [init-opcode] phase=read_cycle requires write [0x29] to addr 0x4C (attempt 1, evidence: operation_flows[2].steps[1])

### Runtime probe (Renode)
- 2 of 2 stimulus vector(s) failed in Renode:
  - `mechanical_register_mapped_1` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel temp_ext1 expected=100000 got=875 tol=0 source_bytes=[0x64,0x00] first_i2c_read_rx=[0x5D]; channel temp_ext2 expected=127875 got=255875 tol=0 source_bytes=[0x7F,0xE0] first_i2c_read_rx=[0x5D]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `mechanical_register_mapped_2` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel temp_ext1 expected=127875 got=224875 tol=0 source_bytes=[0x7F,0xE0] first_i2c_read_rx=[0x5D]; channel temp_ext2 expected=0 got=255875 tol=0 source_bytes=[0x00,0x00] first_i2c_read_rx=[0x5D]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
