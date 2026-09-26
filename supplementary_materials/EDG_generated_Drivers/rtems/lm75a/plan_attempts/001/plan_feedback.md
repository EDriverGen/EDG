# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Provider/schema failure
- source: `schema`
- message: response failed CONTRACT_TEST_PLAN_SCHEMA (2 error(s))
- schema: test_plan.expected_transactions.0.write_prefix_any_of.0.0: 0 is not of type 'string'
- schema: test_plan.expected_transactions.0.write_prefix_any_of.0.1: 0 is not of type 'string'

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
