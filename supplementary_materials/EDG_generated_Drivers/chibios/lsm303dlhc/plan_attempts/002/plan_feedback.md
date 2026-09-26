# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 2 was not accepted.

## Blocking plan validation errors
- channel 'accel_x' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_g'
- channel 'accel_y' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_g'
- channel 'accel_z' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_g'
- channel 'mag_x' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_gauss'
- channel 'mag_y' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_gauss'
- channel 'mag_z' contradicts SECTION B3: semantic_kind=raw_count but api_contract unit is 'milli_gauss'

## Consistency report
- 2 stim; 2 L1/L2

## Coverage report
- 8/8 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
