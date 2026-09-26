# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- channel 'temperature' contradicts SECTION B3: semantic_kind=physical_scaled requires public_unit 'milli_degC', but api_contract unit is ''

## Consistency report
- 2 stim; 2 L1/L2

## Coverage report
- 3/3 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
