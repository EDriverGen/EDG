## ATTEMPT 2 FEEDBACK

### Compile errors
- the previous driver failed to compile (syntax or link). Fix these arm-gcc errors:
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:11:12: error: implicit declaration of function 'write' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:15:12: error: implicit declaration of function 'read' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:28:15: error: implicit declaration of function 'open' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:28:26: error: 'O_RDWR' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:28:35: error: 'O_NOCTTY' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:32:20: error: storage size of 'tty' isn't known
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:34:9: error: implicit declaration of function 'tcgetattr' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:35:9: error: implicit declaration of function 'close' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:38:5: error: implicit declaration of function 'cfsetospeed' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:38:23: error: 'B9600' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:39:5: error: implicit declaration of function 'cfsetispeed' [-Wimplicit-function-declaration]
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:40:35: error: 'CSIZE' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:40:44: error: 'CS8' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:41:21: error: 'PARENB' undeclared (first use in this function)
  - /tmp/stubcc_mhz19b_54duzr86/drv/mhz19b.c:42:21: error: 'CSTOPB' undeclared (first use in this function)
  - ... and 17 more compile diagnostic(s)

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
