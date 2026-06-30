/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for ChibiOS
 */
#include "lsm303dlhc_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void lsm303dlhc_sample(void)
{
    struct lsm303dlhc_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    lsm303dlhc_init(&dev, bus, LSM303DLHC_ADDR_ACCEL);

    ret = lsm303dlhc_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "LSM303DLHC Accelerometer/Magnetometer not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "LSM303DLHC Accelerometer/Magnetometer detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    lsm303dlhc_enable_accel(&dev);
    lsm303dlhc_enable_mag(&dev);
    int16_t ax, ay, az, mx, my, mz;
    ret = lsm303dlhc_read_accel(&dev, &ax, &ay, &az);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "Accel: x=%d y=%d z=%d\r\n", ax, ay, az);
    ret = lsm303dlhc_read_mag(&dev, &mx, &my, &mz);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "Mag: x=%d y=%d z=%d\r\n", mx, my, mz);
        chThdSleepMilliseconds(1000);
    }
}
