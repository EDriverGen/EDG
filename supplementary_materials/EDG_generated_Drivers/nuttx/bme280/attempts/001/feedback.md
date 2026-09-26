## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as last u8 (attempt 1, evidence: stim `zero_values` matched last_u8)
- [compile-includes] `#include "arch.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "bme280.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "nuttx.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <nuttx/i2c/i2c_master.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0xD0] to addr 0x76 (attempt 1, evidence: registers_or_commands[0])
- [init-opcode] init phase must write [0xE0] to addr 0x76 (attempt 1, evidence: registers_or_commands[1])
- [init-opcode] init phase must write [0xF5] to addr 0x76 (attempt 1, evidence: registers_or_commands[5])
- [init-opcode] phase=read_cycle requires write [0xF2] (+10 alt) to addr 0x76 (attempt 1, evidence: registers_or_commands[2], registers_or_commands[3], registers_or_commands[4], registers_or_commands[6], registers_or_commands[7], registers_or_commands[8], registers_or_commands[9], registers_or_commands[10], registers_or_commands[11], registers_or_commands[12], registers_or_commands[13])
- [runtime-pass] stim `max_values` passed on Renode (multi_channel match) (attempt 1, evidence: Stage 6 result_pass=True, test_done=True)
- [runtime-pass] stim `zero_values` passed on Renode (multi_channel match) (attempt 1, evidence: Stage 6 result_pass=True, test_done=True)

### Runtime probe (Renode)
- 1 of 3 stimulus vector(s) failed in Renode:
  - `nominal_values` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel temp expected=256 got=128 tol=0 first_i2c_read_rx=[0x60]; channel pressure expected=128 got=256 tol=0 first_i2c_read_rx=[0x60]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
