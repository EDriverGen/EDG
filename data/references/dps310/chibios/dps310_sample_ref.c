/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Pressure/Temperature Sensor Driver for ChibiOS
 */
#include "dps310_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void dps310_sample(void)
{
    struct dps310_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    dps310_init(&dev, bus, DPS310_DEFAULT_ADDR);

    ret = dps310_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "DPS310 Pressure/Temperature Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "DPS310 Pressure/Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    dps310_read_calibration(&dev);
    int32_t pres, temp;
    ret = dps310_read_pressure(&dev, &pres);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "DPS310: %d Pa\r\n", (int)pres);
    ret = dps310_read_temperature(&dev, &temp);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "DPS310: %d.%03d C\r\n", (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
        chThdSleepMilliseconds(1000);
    }
}
