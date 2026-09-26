## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian i32 (attempt 1, evidence: stim `negative_pressure_and_temp` matched big_endian_i32)
- [byte-order] raw reads decode as big endian u24 (attempt 1, evidence: stim `nominal_pressure_and_temp` matched big_endian_u24)
- [byte-order] raw reads decode as big endian u32 (attempt 1, evidence: stim `zero_pressure_and_temp` matched big_endian_u32)
- [compile-includes] `#include "bus.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "bus_i2c.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "bus_pin.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "dev_i2c.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "dps310.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "transform.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0x0C] to addr 0x77 (attempt 1, evidence: registers_or_commands[12])
- [init-opcode] init phase must write [0x0D] to addr 0x77 (attempt 1, evidence: registers_or_commands[13])
- [init-opcode] init phase must write [0x10] to addr 0x77 (attempt 1, evidence: registers_or_commands[14])
- [init-opcode] phase=read_cycle requires write [0x00] (+12 alt) to addr 0x77 (attempt 1, evidence: registers_or_commands[0], registers_or_commands[1], registers_or_commands[2], registers_or_commands[3], registers_or_commands[4], registers_or_commands[5], registers_or_commands[6], registers_or_commands[7], registers_or_commands[8], registers_or_commands[9], registers_or_commands[10], registers_or_commands[11], registers_or_commands[15])

### Runtime probe (Renode)
- 3 of 3 stimulus vector(s) failed in Renode:
  - `nominal_pressure_and_temp` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[2] phase=init addr=0x77 missing write prefix any_of=['[0x10]'] in actual I2C trace; observed I2C addr(s)=[0x77]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `zero_pressure_and_temp` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[2] phase=init addr=0x77 missing write prefix any_of=['[0x10]'] in actual I2C trace; observed I2C addr(s)=[0x77]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `negative_pressure_and_temp` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[2] phase=init addr=0x77 missing write prefix any_of=['[0x10]'] in actual I2C trace; observed I2C addr(s)=[0x77]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
