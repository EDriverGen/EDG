/*
 * SPDX-License-Identifier: MIT
 *
 * BH1750 Light Sensor Driver for ThreadX
 */
#include "bh1750_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_bh1750_i2c_context;
extern const struct bh1750_i2c_ops g_bh1750_i2c_ops;

void bh1750_sample_entry(ULONG input)
{
    (void)input;
    struct bh1750_device dev;
    int ret;

    
    bh1750_init(&dev, g_bh1750_i2c_context, &g_bh1750_i2c_ops, BH1750_DEFAULT_ADDR);

    ret = bh1750_probe(&dev);
    if (ret != 0) {
        printf("BH1750 Light Sensor not found!\r\n");
        return;
    }

    printf("BH1750 Light Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    uint32_t lux;
    ret = bh1750_read_lux_x100(&dev, &lux);
    if (ret == 0) {
        printf("BH1750: %u.%02u lux\r\n", (unsigned)(lux / 100), (unsigned)(lux % 100));
    }
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
