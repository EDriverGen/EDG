## ATTEMPT 3 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian u24 (attempt 1, evidence: stim `ch1_zero` matched big_endian_u24)
- [compile-includes] `#include "mcp3008.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "rtdevice.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "rtthread.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] phase=read_cycle requires write <read> to addr spi1 (attempt 1, evidence: operation_flows[0].steps[1])
- [init-opcode] phase=read_cycle requires write [0x01, 0x80, 0x00] to addr spi1 (attempt 1, evidence: operation_flows[0].steps[0])
- [runtime-pass] stim `ch1_zero` passed on Renode (multi_channel match) (attempt 1, evidence: Stage 6 result_pass=True, test_done=True)

### Transaction coverage
- the following `expected_transactions` rows have no mechanical counterpart in `device_ir`. Keep a row only if the driver really emits that traffic; drop the speculative ones to tighten the contract:
  - phase=`read_cycle` addr=spi1 prefix_any_of=read-any

### Runtime probe (Renode)
- 2 of 3 stimulus vector(s) failed in Renode:
  - `ch0_mid_scale` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel ch0 expected=1023 got=0 tol=0. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `ch0_full_scale` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel ch0 expected=1023 got=0 tol=0. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
