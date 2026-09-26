# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Provider/schema failure
- source: `schema`
- message: response failed CONTRACT_TEST_PLAN_SCHEMA (1 error(s))
- schema: test_plan.test_stimuli: [{'name': 'init_and_frame_output', 'mock_preload': {'frame_ok': ['0x00']}, 'expected_frame_err': 0, 'derivation': 'Init sequence writes commands; frame output writes 0x21,0x22 then data bytes. No read-back, so frame_ok sentinel returns 0.'}] is too short

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
