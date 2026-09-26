## ATTEMPT 2 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian u32 (attempt 1, evidence: stim `led0_zero_duty` matched big_endian_u32)
- [byte-order] raw reads decode as little endian u32 (attempt 1, evidence: stim `led0_50_percent_duty` matched little_endian_u32)
- [compile-includes] `#include "pca9685.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "riot.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <periph/i2c.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <xtimer.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] phase=read_cycle requires write <read> to addr 0x71 (attempt 1, evidence: operation_flows[0].steps[1])
- [init-opcode] phase=read_cycle requires write [0x08] to addr 0x71 (attempt 1, evidence: operation_flows[0].steps[0])

### Transaction coverage
- the following `expected_transactions` rows have no mechanical counterpart in `device_ir`. Keep a row only if the driver really emits that traffic; drop the speculative ones to tighten the contract:
  - phase=`read_cycle` addr=0x71 prefix_any_of=read-any

### Runtime probe (Renode)
- 3 of 3 stimulus vector(s) failed in Renode:
  - `led0_50_percent_duty` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel led1 expected=0 got=4095 tol=0 first_i2c_read_rx=[0x00,0x08,0x00,0x00]; expected_transactions[0] phase=read_cycle addr=0x71 missing write prefix any_of=['[0x08]'] in actual I2C trace; matching prefix was observed only at I2C addr(s)=[0x40], expected 0x71. Configure/select the bus slave address before issuing the transfer.; expected_transactions[1] phase=read_cycle addr=0x71 requires an I2C read but trace has none; reads were observed only at I2C addr(s)=[0x40]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `led0_full_duty` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel led1 expected=0 got=4095 tol=0 first_i2c_read_rx=[0xFF,0x0F,0x00,0x00]; expected_transactions[0] phase=read_cycle addr=0x71 missing write prefix any_of=['[0x08]'] in actual I2C trace; matching prefix was observed only at I2C addr(s)=[0x40], expected 0x71. Configure/select the bus slave address before issuing the transfer.; expected_transactions[1] phase=read_cycle addr=0x71 requires an I2C read but trace has none; reads were observed only at I2C addr(s)=[0x40]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `led0_zero_duty` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel led1 expected=0 got=4095 tol=0 first_i2c_read_rx=[0x00,0x00,0x00,0x00]; expected_transactions[0] phase=read_cycle addr=0x71 missing write prefix any_of=['[0x08]'] in actual I2C trace; matching prefix was observed only at I2C addr(s)=[0x40], expected 0x71. Configure/select the bus slave address before issuing the transfer.; expected_transactions[1] phase=read_cycle addr=0x71 requires an I2C read but trace has none; reads were observed only at I2C addr(s)=[0x40]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
