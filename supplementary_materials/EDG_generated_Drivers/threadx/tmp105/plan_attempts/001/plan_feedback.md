# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Provider/schema failure
- source: `schema`
- message: response failed CONTRACT_TEST_PLAN_SCHEMA (1 error(s))
- schema: test_plan.test_stimuli: [{'name': 'positive_temperature_25C', 'mock_preload': {'0x00': ['0x01', '0x90']}, 'expected_read_raw': 25000, 'derivation': "raw = (0x01 << 8) | 0x90 = 400; ((400 >> 4) & 0xFFF) = 25; 25 * 625 / 10 = 1562.5? Wait recalc: 400 >> 4 = 25; 25 * 625 = 15625; 15625 / 10 = 1562.5? No, formula: (((raw >> 4) & 0xFFF) * 625) // 10. raw=400, raw>>4=25, 25*625=15625, 15625//10=1562.5? integer division gives 1562. But 25C should be 25000 milli_degC. Let's choose raw=1600: 0x0640. Then raw>>4=100, 100*625=62500, 62500//10=6250? That's 6.25C. Need 25000: 25000*10/625=400, so raw>>4=400, raw=6400=0x1900. So mock_preload: [0x19, 0x00]. expected_read_raw=25000."}] is too short

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
