# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Provider/schema failure
- source: `schema`
- message: response failed CONTRACT_TEST_PLAN_SCHEMA (1 error(s))
- schema: test_plan.test_stimuli: [{'name': 'positive_temperature_25C', 'mock_preload': {'0x00': ['0x01', '0x90']}, 'expected_read_raw': 25000, 'derivation': "raw = (0x01 << 8) | 0x90 = 400; ((400 >> 4) & 0xFFF) * 625 / 10 = 25 * 625 / 10 = 15625 / 10 = 1562.5? Wait recalc: 400 >> 4 = 25; 25 * 625 = 15625; 15625 / 10 = 1562.5, but expected 25000. Let's fix: raw=400, ((400>>4)&0xFFF)=25, 25*625=15625, 15625//10=1562.5? Actually integer division: 15625/10=1562 (truncated). But 25°C should be 25000 milli°C. So raw must be 400? 400*0.0625=25°C=25000 milli°C. The formula: (((raw>>4)&0xFFF)*625)//10. For raw=400: (25*625)//10 = 15625//10 = 1562, not 25000. So raw must be 6400? 6400>>4=400, 400*625=250000, //10=25000. So raw=6400 = 0x1900. Bytes: [0x19, 0x00]. Correct."}] is too short

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
