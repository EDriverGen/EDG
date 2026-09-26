/*
 * SPDX-License-Identifier: MIT
 *
 * TMP105 Temperature Sensor Driver for ThreadX
 */
#include "tmp105_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_tmp105_i2c_context;
extern const struct tmp105_i2c_ops g_tmp105_i2c_ops;

void tmp105_sample_entry(ULONG input)
{
    (void)input;
    struct tmp105_device dev;
    int ret;

    
    tmp105_init(&dev, g_tmp105_i2c_context, &g_tmp105_i2c_ops, TMP105_ADDR_DEFAULT);

    ret = tmp105_probe(&dev);
    if (ret != 0) {
        printf("TMP105 Temperature Sensor not found!\r\n");
        return;
    }

    printf("TMP105 Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = tmp105_read_temperature(&dev, &temp);
    if (ret == 0) {
        printf("TMP105: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
