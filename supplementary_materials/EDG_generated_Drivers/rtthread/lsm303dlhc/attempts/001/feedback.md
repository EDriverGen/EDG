## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [compile-includes] `#include "lsm303dlhc.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <rtdevice.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0x00, 0x0C] to addr 0x1E (attempt 1, evidence: operation_flows[1].steps[0])
- [init-opcode] init phase must write [0x01, 0x20] to addr 0x1E (attempt 1, evidence: operation_flows[1].steps[1])
- [init-opcode] init phase must write [0x02, 0x00] to addr 0x1E (attempt 1, evidence: operation_flows[1].steps[2])
- [init-opcode] init phase must write [0x20, 0x57] to addr 0x19 (attempt 1, evidence: operation_flows[0].steps[0])
- [init-opcode] init phase must write [0x23, 0x08] to addr 0x19 (attempt 1, evidence: operation_flows[0].steps[1])
- [init-opcode] phase=read_cycle requires write [0x03] to addr 0x1E (attempt 1, evidence: operation_flows[3].steps[1])
- [init-opcode] phase=read_cycle requires write [0x31] to addr 0x1E (attempt 1, evidence: operation_flows[3].steps[2])
- [init-opcode] phase=read_cycle requires write [0xA8] to addr 0x19 (attempt 1, evidence: operation_flows[2].steps[1])

### Runtime probe (Renode)
- 2 of 2 stimulus vector(s) failed in Renode:
  - `mechanical_register_mapped_1` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel accel_x expected=1000 got=-6141 tol=0 source_bytes=[0x03,0xE8] first_i2c_read_rx=[0xE8,0x03,0x31,0x04,0x7A,0x04]; channel accel_y expected=1073 got=12548 tol=0 source_bytes=[0x04,0x31] first_i2c_read_rx=[0xE8,0x03,0x31,0x04,0x7A,0x04]; channel accel_z expected=1146 got=31236 tol=0 source_bytes=[0x04,0x7A] first_i2c_read_rx=[0xE8,0x03,0x31,0x04,0x7A,0x04]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `mechanical_register_mapped_2` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel accel_x expected=-1000 got=6396 tol=0 source_bytes=[0xFC,0x18] first_i2c_read_rx=[0x18,0xFC,0xCF,0xFB,0x86,0xFB]; channel accel_y expected=-1073 got=-12293 tol=0 source_bytes=[0xFB,0xCF] first_i2c_read_rx=[0x18,0xFC,0xCF,0xFB,0x86,0xFB]; channel accel_z expected=-1146 got=-30981 tol=0 source_bytes=[0xFB,0x86] first_i2c_read_rx=[0x18,0xFC,0xCF,0xFB,0x86,0xFB]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
