## ATTEMPT 2 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian u16 (attempt 1, evidence: stim `mechanical_single_channel_zero` matched big_endian_u16 on [0x00, 0x00])
- [compile-includes] `#include "bh1750.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <hal/hal_i2c.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <os/os_time.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] phase=read_cycle requires write [0x00] (+10 alt) to addr 0x23 (attempt 1, evidence: registers_or_commands[0], registers_or_commands[1], registers_or_commands[2], registers_or_commands[3], registers_or_commands[4], registers_or_commands[5], registers_or_commands[6], registers_or_commands[7], registers_or_commands[8], registers_or_commands[9], registers_or_commands[10])
- [runtime-pass] stim `mechanical_single_channel_zero` passed on Renode at raw=0 (attempt 1, evidence: Stage 6 result_pass=True, test_done=True)

### Transaction coverage
- the following `expected_transactions` rows have no mechanical counterpart in `device_ir`. Keep a row only if the driver really emits that traffic; drop the speculative ones to tighten the contract:
  - phase=`init_cycle` addr=0x23 prefix_any_of=[(0x01), (0x07), (0x10)]

### Runtime probe (Renode)
- 1 of 2 stimulus vector(s) failed in Renode:
  - `mechanical_single_channel_positive` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: read_raw expected=400000 got=333 tol=0. Numeric scale diagnostic: expected_read_raw/read_raw ~= 1.2e+03 (400000 vs 333), close to 1000x. Treat this as a public-unit conversion bug unless the derivation proves otherwise: keep `expected_*` in the adapter/API output unit and change the driver or adapter calculation so the emitted value has that unit (for milli/micro units, do not divide away the scale). If the device LSB is already expressed in milli/micro units per count, the public output should usually multiply counts by that LSB directly, not divide it back to base units. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
