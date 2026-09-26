## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian u16 (attempt 1, evidence: stim `distance_100mm` matched big_endian_u16 on [0x00, 0x64])
- [compile-includes] `#include "bus.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "bus_i2c.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "bus_pin.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "dev_i2c.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "transform.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "vl53l0x.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stddef.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] phase=read_cycle requires write [0xC0] (+4 alt) to addr 0x52 (attempt 1, evidence: registers_or_commands[0], registers_or_commands[1], registers_or_commands[2], registers_or_commands[3], registers_or_commands[4])

### Runtime probe (Renode)
- 3 of 3 stimulus vector(s) failed in Renode:
  - `distance_100mm` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[0] phase=read_cycle addr=0x52 missing write prefix any_of=['[0xC0]', '[0xC1]', '[0xC2]', '[0x51]', '[0x61]'] in actual I2C trace; observed I2C addr(s)=[0x52]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `distance_0mm` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[0] phase=read_cycle addr=0x52 missing write prefix any_of=['[0xC0]', '[0xC1]', '[0xC2]', '[0x51]', '[0x61]'] in actual I2C trace; observed I2C addr(s)=[0x52]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `distance_8191mm` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: expected_transactions[0] phase=read_cycle addr=0x52 missing write prefix any_of=['[0xC0]', '[0xC1]', '[0xC2]', '[0x51]', '[0x61]'] in actual I2C trace; observed I2C addr(s)=[0x52]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
