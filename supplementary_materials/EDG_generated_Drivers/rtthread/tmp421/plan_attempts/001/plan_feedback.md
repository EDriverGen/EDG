# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan.test_stimuli[0] 'local_positive_temp' derivation contains draft/self-correction text marker 'Wait'. The final JSON must already contain the corrected mock_preload bytes and expected_* values; derivation must only explain the final arithmetic for the bytes actually present in mock_preload.
- test_plan multi_channel stimulus 'local_positive_temp' is only partially checkable: ch=temp_local: bytes=[0x00, 0x1A, 0x50] no interp = 26250.0; ch=temp_remote: derivation says channel is not read. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.
- test_plan multi_channel stimulus 'remote_positive_temp' is only partially checkable: ch=temp_remote: bytes=[0x00, 0x1A, 0x50] no interp = 26312.0. Every expected_channels entry must have matching mock_preload data and an executable derivation for this same stimulus; do not declare placeholder channels as 0 or 'not read'.

## Consistency report
- 2 stim; 1 L1/L2; 1 llm_only

## Coverage report
- 6/6 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
