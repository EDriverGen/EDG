## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as last i8 (attempt 1, evidence: stim `negative_temperature` matched last_i8)
- [byte-order] raw reads decode as last u8 (attempt 1, evidence: stim `zero_temperature` matched last_u8)
- [compile-includes] `#include "bme280.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "rtthread.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <drivers/dev_i2c.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0xD0] to addr 0x76 (attempt 1, evidence: operation_flows[0].steps[0])
- [init-opcode] init phase must write [0xE0, 0xB6] to addr 0x76 (attempt 1, evidence: operation_flows[1].steps[0])
- [init-opcode] phase=read_cycle requires write [0xF7] to addr 0x76 (attempt 1, evidence: operation_flows[3].steps[0])
- [runtime-pass] stim `zero_temperature` passed on Renode (multi_channel match) (attempt 1, evidence: Stage 6 result_pass=True, test_done=True)

### Stimulus self-check
- 1 stimulus/stimuli were not statically checkable before Renode:
  - `nominal_temperature`: no channel could be L1-verified

### Runtime probe (Renode)
- 2 of 3 stimulus vector(s) failed in Renode:
  - `nominal_temperature` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel temp expected=25000 got=376460 tol=0 first_i2c_read_rx=[0x60]; channel pressure expected=8.38861e+06 got=419424 tol=0 first_i2c_read_rx=[0x60]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `negative_temperature` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel temp expected=-1 got=1.04851e+06 tol=0 first_i2c_read_rx=[0x60]; channel pressure expected=1.67772e+07 got=1.04858e+06 tol=0 first_i2c_read_rx=[0x60]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
