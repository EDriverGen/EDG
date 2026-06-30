/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * LM75A Temperature Sensor Driver for RIOT OS
 */
#include "lm75a_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct lm75a_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    lm75a_init(&dev, bus, LM75A_DEFAULT_ADDR);

    ret = lm75a_probe(&dev);
    if (ret != 0) {
        printf("LM75A Temperature Sensor not found!\r\n");
        return 1;
    }

    printf("LM75A Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = lm75a_read_temperature(&dev, &temp);
    if (ret == 0) {
        printf("LM75A: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
