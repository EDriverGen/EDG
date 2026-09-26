## ATTEMPT 1 FEEDBACK

### Synthesis errors
- the next response must be a single **bare JSON object** starting with `{` and ending with `}` — no markdown fences, no prose before or after the braces, no trailing comments. The pipeline reads the first non-whitespace character; anything other than `{` is rejected immediately.
- raw response from the previous attempt (truncated for feedback):
  ```
  ```json
  {
    "driver_header": "#ifndef DHT22_H\n#define DHT22_H\n\n#include <stdint.h>\n#include <stdbool.h>\n#include <periph/gpio.h>\n\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\nstruct dht22_device {\n    gpio_t pin;\n};\n\nint dht22_init(struct dht22_device *dev, gpio_t pin);\nint dht22_read(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);\n\n#ifdef __cplusplus\n}\n#endif
  ... [39682 chars omitted] ...
  ev->pin);\n    gpio_init(dev->pin, GPIO_OUT);\n    gpio_set(dev->pin);\n    gpio_init(dev->pin, GPIO_OUT);\n    gpio_set(dev->pin);\n    gpio_init(dev->pin, GPIO_OUT);\n    gpio_set(dev->pin);\n    gp
  ```

### Task
- Respect every invariant above.
- Fix every issue marked as an error; they are blocking.
- Address warnings where feasible; justify via Stage 6 outcomes when you deliberately leave one.
- Regenerate all 4 top-level JSON keys (`driver_header`, `driver_source`, `api_contract`, `test_plan`) as a single bare JSON object.
