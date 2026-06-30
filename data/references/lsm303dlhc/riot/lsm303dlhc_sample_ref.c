/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for RIOT OS
 */
#include "lsm303dlhc_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct lsm303dlhc_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    lsm303dlhc_init(&dev, bus, LSM303DLHC_ADDR_ACCEL);

    ret = lsm303dlhc_probe(&dev);
    if (ret != 0) {
        printf("LSM303DLHC Accelerometer/Magnetometer not found!\r\n");
        return 1;
    }

    printf("LSM303DLHC Accelerometer/Magnetometer detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    lsm303dlhc_enable_accel(&dev);
    lsm303dlhc_enable_mag(&dev);
    int16_t ax, ay, az, mx, my, mz;
    ret = lsm303dlhc_read_accel(&dev, &ax, &ay, &az);
    if (ret == 0) printf("Accel: x=%d y=%d z=%d\r\n", ax, ay, az);
    ret = lsm303dlhc_read_mag(&dev, &mx, &my, &mz);
    if (ret == 0) printf("Mag: x=%d y=%d z=%d\r\n", mx, my, mz);
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
