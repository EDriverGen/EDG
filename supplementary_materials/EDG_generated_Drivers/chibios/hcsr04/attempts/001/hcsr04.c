#include "hcsr04.h"
#include "hal_pal.h"
#include "stm32_gpio.h"
#include <stdint.h>
#include <stddef.h>

#define TRIG_LINE PAL_LINE(GPIOA, 0)
#define ECHO_LINE PAL_LINE(GPIOA, 1)

int hcsr04_init(struct hcsr04_device *dev) {
    if (!dev) return -1;
    dev->trig_line = TRIG_LINE;
    dev->echo_line = ECHO_LINE;
    palSetPadMode(GPIOA, 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 1, PAL_MODE_INPUT);
    palClearPad(GPIOA, 0);
    return 0;
}

int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw) {
    if (!dev || !raw) return -1;
    uint32_t trig = dev->trig_line;
    uint32_t echo = dev->echo_line;
    palSetPad(GPIOA, 0);
    chSysPolledDelayX(10);
    palClearPad(GPIOA, 0);
    uint32_t timeout = 100000;
    while (palReadPad(GPIOA, 1) == 0) {
        if (--timeout == 0) return -5;
    }
    uint32_t t = 0;
    while (palReadPad(GPIOA, 1) != 0) {
        t++;
        if (t > 30000) return -5;
    }
    *raw = (int32_t)((t * 343) / 2000);
    return 0;
}