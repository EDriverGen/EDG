/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * DPS310 Pressure/Temperature Sensor Driver for RIOT OS
 */
#include "dps310_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct dps310_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    dps310_init(&dev, bus, DPS310_DEFAULT_ADDR);

    ret = dps310_probe(&dev);
    if (ret != 0) {
        printf("DPS310 Pressure/Temperature Sensor not found!\r\n");
        return 1;
    }

    printf("DPS310 Pressure/Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    dps310_read_calibration(&dev);
    int32_t pres, temp;
    ret = dps310_read_pressure(&dev, &pres);
    if (ret == 0) printf("DPS310: %d Pa\r\n", (int)pres);
    ret = dps310_read_temperature(&dev, &temp);
    if (ret == 0) printf("DPS310: %d.%03d C\r\n", (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
