# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Provider/schema failure
- source: `schema`
- message: response failed CONTRACT_TEST_PLAN_SCHEMA (1 error(s))
- schema: test_plan.test_stimuli: [{'name': 'positive_temperature_25C', 'mock_preload': {'0x00': ['0x01', '0x90']}, 'expected_read_raw': 25000, 'derivation': "raw = 0x0190 = 400; (((400 >> 4) & 0xFFF) * 625) // 10 = (25 * 625) // 10 = 15625 // 10 = 1562? Wait recalc: 400 >> 4 = 25; 25 * 625 = 15625; 15625 // 10 = 1562.5 -> integer 1562. But expected milli_degC for 25C is 25000. Let's choose raw that gives 25000: need raw_12bit = (25000 * 10) // 625 = 400; raw = 400 << 4 = 6400 = 0x1900. So bytes: [0x19, 0x00]."}] is too short

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
