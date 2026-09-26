# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan multi_channel stimulus 'positive_local_remote' is only partially checkable: ch=temp_local: bytes=[0x00, 0x19, 0x40, 0x1A, 0x80] no interp = 25625.0; ch=temp_remote: bytes=[0x00, 0x19, 0x40, 0x1A, 0x80] no interp = 26875.0. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.
- test_plan multi_channel stimulus 'negative_local_remote' is only partially checkable: ch=temp_local: bytes=[0x00, 0xF6, 0x80, 0xF5, 0x00] no interp = -10000.0. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.

## Consistency report
- 3 stim; 2 L1/L2; 1 llm_only

## Coverage report
- 6/6 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
