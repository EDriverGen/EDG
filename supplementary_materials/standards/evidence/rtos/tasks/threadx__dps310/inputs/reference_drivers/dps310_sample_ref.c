/*
 * SPDX-License-Identifier: MIT
 *
 * DPS310 Pressure/Temperature Sensor Driver for ThreadX
 */
#include "dps310_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_dps310_i2c_context;
extern const struct dps310_i2c_ops g_dps310_i2c_ops;

void dps310_sample_entry(ULONG input)
{
    (void)input;
    struct dps310_device dev;
    int ret;

    ret = dps310_init(&dev, g_dps310_i2c_context, &g_dps310_i2c_ops, DPS310_DEFAULT_ADDR);
    if (ret != 0) {
        printf("DPS310 init failed\r\n");
        return;
    }

    ret = dps310_probe(&dev);
    if (ret != 0) {
        printf("DPS310 Pressure/Temperature Sensor not found!\r\n");
        return;
    }

    printf("DPS310 Pressure/Temperature Sensor detected\r\n");

    ret = dps310_read_calibration(&dev);
    if (ret != 0) {
        printf("DPS310 calibration read failed\r\n");
        return;
    }

    int i;
    for (i = 0; i < 10; i++) {
    int32_t pres, temp;
    ret = dps310_read_pressure(&dev, &pres);
    if (ret == 0) printf("DPS310: %d Pa\r\n", (int)pres);
    ret = dps310_read_temperature(&dev, &temp);
    if (ret == 0) printf("DPS310: %d.%03d C\r\n", (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
