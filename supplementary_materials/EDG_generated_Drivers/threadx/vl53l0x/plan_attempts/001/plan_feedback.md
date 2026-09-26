# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan consistency failed for 'distance_100mm': bytes=[0x00, 0x99] cannot yield expected_read_raw=100.0 under any standard interpretation; derivation=`mock_preload read_bytes = [0x00, 100] => big-endian uint16 = 0x0064 = 100 mm`
- test_plan consistency failed for 'distance_8191mm': bytes=[0x00, 0x99] cannot yield expected_read_raw=8191.0 under any standard interpretation; derivation=`mock_preload read_bytes = [0x1F, 0xFF] => big-endian uint16 = 0x1FFF = 8191 mm`

## Consistency report
- 3 stim; 1 L1/L2; 2 inconsistent

## Coverage report
- 1/1 mechanical covered

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
