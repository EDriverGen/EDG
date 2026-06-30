/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Driver for ChibiOS
 */
#include "lm75a_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void lm75a_sample(void)
{
    struct lm75a_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    lm75a_init(&dev, bus, LM75A_DEFAULT_ADDR);

    ret = lm75a_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "LM75A Temperature Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "LM75A Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = lm75a_read_temperature(&dev, &temp);
    if (ret == 0) {
        chprintf((BaseSequentialStream *)&SD1, "LM75A: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        chThdSleepMilliseconds(1000);
    }
}
