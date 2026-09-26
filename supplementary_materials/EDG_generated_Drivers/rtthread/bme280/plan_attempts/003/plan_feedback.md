# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 3 was not accepted.

## Blocking plan validation errors
- test_plan.test_stimuli[0] 'nominal_temperature' expected_channels['pressure']=8388608, but mock_preload register(s) 0xF7, 0xF8, 0xF9 bytes [0x66 0x66 0x00] encode raw_count 419328 via (((0x66 << 16) | (0x66 << 8) | 0x00) & 0xFFFFF). Update either mock_preload or expected_channels; derivation text is not authoritative.

## Consistency report
- 3 stim; 3 L1/L2

## Coverage report
- 3/3 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
