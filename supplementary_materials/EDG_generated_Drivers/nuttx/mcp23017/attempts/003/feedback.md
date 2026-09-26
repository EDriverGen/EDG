## ATTEMPT 3 FEEDBACK

### Confirmed Invariants (sticky across rounds)
- [byte-order] raw reads decode as big endian u16 (attempt 2, evidence: stim `all_low` matched big_endian_u16)
- [byte-order] raw reads decode as last u8 (attempt 2, evidence: stim `porta_all_high` matched last_u8)
- [compile-includes] `#include "mcp23017.h"` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <errno.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <nuttx/i2c/i2c_master.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdbool.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)
- [compile-includes] `#include <stdint.h>` resolved cleanly under stub_compile — KEEP this exact include (do NOT switch to a different vendor / cross-family / LL header in the next attempt) (attempt 2, evidence: L1 syntax compile passed for driver_header+driver_source)

### Compile errors
- *SYMBOL SURFACE MISMATCH*: the compiler found a likely public replacement for an undeclared symbol. Treat the undeclared symbol as forbidden in the next attempt when the replacement appears in SECTION C or a public/stub header:
    - replace forbidden `I2C_WRITE` with public `GPIOC_WRITE`
    - replace forbidden `I2C_READ` with public `I2C_M_READ`

- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mcp23017_0nlpikdy/drv/mcp23017.c:18:38: error: passing argument 2 of 'I2C_TRANSFER' from incompatible pointer type [-Wincompatible-pointer-types]
  - /tmp/stubcc_mcp23017_0nlpikdy/drv/mcp23017.c:18:47: error: passing argument 3 of 'I2C_TRANSFER' makes integer from pointer without a cast [-Wint-conversion]
  - /tmp/stubcc_mcp23017_0nlpikdy/drv/mcp23017.c:18:15: error: too many arguments to function 'I2C_TRANSFER'
  - /tmp/stubcc_mcp23017_0nlpikdy/drv/mcp23017.c:31:15: error: implicit declaration of function 'I2C_WRITE'; did you mean 'GPIOC_WRITE'? [-Wimplicit-function-declaration]
  - /tmp/stubcc_mcp23017_0nlpikdy/drv/mcp23017.c:35:11: error: implicit declaration of function 'I2C_READ'; did you mean 'I2C_M_READ'? [-Wimplicit-function-declaration]

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
