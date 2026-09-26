# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan multi_channel stimulus 'nominal_positive_local_remote' is only partially checkable: ch=temp_local: bytes=[0x00, 0x19, 0x80, 0x80, 0x1A, 0x40, 0x40] no interp = 25625.0. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.

## Consistency report
- 3 stim; 3 L1/L2

## Coverage report
- 6/6 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
