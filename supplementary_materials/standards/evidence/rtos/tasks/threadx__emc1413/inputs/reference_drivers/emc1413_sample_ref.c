/*
 * SPDX-License-Identifier: MIT
 *
 * EMC1413 Temperature Sensor Driver for ThreadX
 */
#include "emc1413_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_emc1413_i2c_context;
extern const struct emc1413_i2c_ops g_emc1413_i2c_ops;

void emc1413_sample_entry(ULONG input)
{
    (void)input;
    struct emc1413_device dev;
    int ret;

    
    emc1413_init(&dev, g_emc1413_i2c_context, &g_emc1413_i2c_ops, EMC1413_ADDR_DEFAULT);

    ret = emc1413_probe(&dev);
    if (ret != 0) {
        printf("EMC1413 Temperature Sensor not found!\r\n");
        return;
    }

    printf("EMC1413 Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t int_t, ext1_t, ext2_t;
    ret = emc1413_read_internal_temp(&dev, &int_t);
    if (ret == 0) printf("EMC1413 internal: %d.%03d C\r\n", (int)(int_t/1000), (int)(int_t>=0?int_t%1000:(-int_t)%1000));
    ret = emc1413_read_external1_temp(&dev, &ext1_t);
    if (ret == 0) printf("EMC1413 ext1: %d.%03d C\r\n", (int)(ext1_t/1000), (int)(ext1_t>=0?ext1_t%1000:(-ext1_t)%1000));
    ret = emc1413_read_external2_temp(&dev, &ext2_t);
    if (ret == 0) printf("EMC1413 ext2: %d.%03d C\r\n", (int)(ext2_t/1000), (int)(ext2_t>=0?ext2_t%1000:(-ext2_t)%1000));
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
