# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan multi_channel stimulus 'nominal_pressure_and_temp' is only partially checkable: ch=temp: bytes=[0x7F, 0xFF, 0xFF, 0x00, 0x80, 0x00] no interp = 32768.0. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.

## Consistency report
- 3 stim; 3 L1/L2

## Coverage report
- 4/4 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
