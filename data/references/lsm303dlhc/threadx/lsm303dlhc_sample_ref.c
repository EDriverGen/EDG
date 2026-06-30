/*
 * SPDX-License-Identifier: MIT
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for ThreadX
 */
#include "lsm303dlhc_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_lsm303dlhc_i2c_context;
extern const struct lsm303dlhc_i2c_ops g_lsm303dlhc_i2c_ops;

void lsm303dlhc_sample_entry(ULONG input)
{
    (void)input;
    struct lsm303dlhc_device dev;
    int ret;

    
    lsm303dlhc_init(&dev, g_lsm303dlhc_i2c_context, &g_lsm303dlhc_i2c_ops, LSM303DLHC_ADDR_ACCEL);

    ret = lsm303dlhc_probe(&dev);
    if (ret != 0) {
        printf("LSM303DLHC Accelerometer/Magnetometer not found!\r\n");
        return;
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
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
