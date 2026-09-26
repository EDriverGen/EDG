# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan.expected_transactions omits mechanical transaction phase='init' addr_or_pin='0x44' prefixes=(('0x00', '0x06'),): no prefix overlap; LLM=[('0x30', '0xA2')] ∩ mechanical=[('0x00', '0x06')] is empty

## Consistency report
- 2 stim; 2 L1/L2

## Coverage report
- 3/4 mechanical covered; 1 missing

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
