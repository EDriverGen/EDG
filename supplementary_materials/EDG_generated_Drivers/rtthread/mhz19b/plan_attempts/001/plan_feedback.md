# Plan-stage feedback

Regenerate only `api_contract` and `test_plan`. Do not output `driver_header` or `driver_source`.

Planner attempt 1 was not accepted.

## Blocking plan validation errors
- test_plan.test_stimuli[0] 'co2_1000ppm' derivation contains draft/self-correction text marker 'Wait'. The final JSON must already contain the corrected mock_preload bytes and expected_* values; derivation must only explain the final arithmetic for the bytes actually present in mock_preload.
- UART packet stimulus 'co2_1000ppm' mock_preload.read_bytes has checksum 0x7D, but the generated write prefix [0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79] establishes a two's-complement checksum over frame bytes[1:-1]. For response frame [0xFF, 0x86, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x7D], the final byte should be 0x8F. Recompute the checksum for the response frame; do not reuse the request checksum.

## Consistency report
- 2 stim; 2 L1/L2

## Coverage report
- 2/2 mechanical covered; 1 LLM extras

The next response must keep the same split-stage contract: only `api_contract` and `test_plan` at the top level.
