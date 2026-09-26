## ATTEMPT 1 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as little endian i16 (attempt 1, evidence: stim `mechanical_register_mapped_2` matched little_endian_i16)
- [byte-order] raw reads decode as little endian u16 (attempt 1, evidence: stim `mechanical_register_mapped_1` matched little_endian_u16)
- [compile-includes] `#include "adxl345.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "hdf_base.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include "spi_if.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <string.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 1, evidence: L1 syntax compile passed for driver_header+driver_source)
- [init-opcode] init phase must write [0x2D, 0x00] to addr spi1 (attempt 1, evidence: operation_flows[0].steps[0])
- [init-opcode] init phase must write [0x2D, 0x08] to addr spi1 (attempt 1, evidence: operation_flows[1].steps[0])
- [init-opcode] phase=read_cycle requires write [0xF2] to addr spi1 (attempt 1, evidence: operation_flows[2].steps[0])

### Runtime probe (Renode)
- 2 of 2 stimulus vector(s) failed in Renode:
  - `mechanical_register_mapped_1` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel accel_x expected=1000 got=0 tol=0 source_bytes=[0xE8,0x03]; channel accel_y expected=1073 got=0 tol=0 source_bytes=[0x31,0x04]; channel accel_z expected=1146 got=0 tol=0 source_bytes=[0x7A,0x04]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.
  - `mechanical_register_mapped_2` [expectation_mismatch]: firmware printed RESULT: PASS but generated test expectations did not match observed output: channel accel_x expected=-1000 got=0 tol=0 source_bytes=[0x18,0xFC]; channel accel_y expected=-1073 got=0 tol=0 source_bytes=[0xCF,0xFB]; channel accel_z expected=-1146 got=0 tol=0 source_bytes=[0x86,0xFB]. If observed and expected differ mainly by a scale factor (for example base units vs milli-units), fix the driver/API unit conversion and keep `expected_*` tied to the public adapter output unit; do not merely change the test expected value to match an incorrectly scaled driver result.

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
