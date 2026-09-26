## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [compile-includes] `#include "hdf_base.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "mcp3008.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "spi_if.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] phase=read_cycle requires write <read> to addr spi1 (attempt 1, evidence: operation_flows[0].steps[1])
- [init-opcode] phase=read_cycle requires write [0x01, 0x80, 0x00] to addr spi1 (attempt 1, evidence: operation_flows[0].steps[0])

### Stimulus self-check
- 1 stimulus/stimuli were not statically checkable before Renode:
  - `ch0_mid_scale`: no channel could be L1-verified

### Transaction coverage
- the following `expected_transactions` rows have no mechanical counterpart in `device_ir`. Keep a row only if the driver really emits that traffic; drop the speculative ones to tighten the contract:
  - phase=`read_cycle` addr=spi1 prefix_any_of=read-any

### Runtime probe (Renode)
- 2 of 2 stimulus vector(s) failed in Renode:
  - `ch0_mid_scale` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel ch0 expected=512 got=0 tol=0. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `ch1_full_scale` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel ch1 expected=1023 got=0 tol=0. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
