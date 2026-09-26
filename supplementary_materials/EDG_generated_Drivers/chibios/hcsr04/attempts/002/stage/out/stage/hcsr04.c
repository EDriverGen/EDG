#include "hcsr04.h"
#include "hal_pal.h"
#include <string.h>

#define TRIG_PIN PAL_LINE(GPIOA, 0)
#define ECHO_PIN PAL_LINE(GPIOA, 1)

int hcsr04_init(struct hcsr04_device *dev) {
    if (dev == NULL) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->port = (void *)GPIOA;
    dev->trig_pin = 0;
    dev->echo_pin = 1;
    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 1, PAL_MODE_INPUT);
    palClearPad(GPIOA, 0);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    if (dev == NULL || raw == NULL) return -1;
    (void)dev;
    palSetPad(GPIOA, 0);
    chSysPolledDelayX(10);
    palClearPad(GPIOA, 0);
    uint32_t timeout = 100000;
    uint32_t pulse_start = 0;
    uint32_t pulse_end = 0;
    while (palReadPad(GPIOA, 1) == 0) {
        if (--timeout == 0) return -5;
    }
    pulse_start = 0;
    timeout = 100000;
    while (palReadPad(GPIOA, 1) != 0) {
        if (--timeout == 0) return -5;
    }
    pulse_end = 100000 - timeout;
    uint32_t T = pulse_end - pulse_start;
    *raw = (int32_t)((T * 343) / 2000);
    return 0;
}
