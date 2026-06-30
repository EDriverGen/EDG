/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for ChibiOS
 */
#include "tmp105_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void tmp105_sample(void)
{
    struct tmp105_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    tmp105_init(&dev, bus, TMP105_ADDR_DEFAULT);

    ret = tmp105_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "TMP105 Temperature Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "TMP105 Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = tmp105_read_temperature(&dev, &temp);
    if (ret == 0) {
        chprintf((BaseSequentialStream *)&SD1, "TMP105: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        chThdSleepMilliseconds(1000);
    }
}
