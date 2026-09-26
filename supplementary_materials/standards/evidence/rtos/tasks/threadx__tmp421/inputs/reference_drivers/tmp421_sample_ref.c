/*
 * SPDX-License-Identifier: MIT
 *
 * TMP421 Remote Temperature Sensor Driver for ThreadX
 */
#include "tmp421_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_tmp421_i2c_context;
extern const struct tmp421_i2c_ops g_tmp421_i2c_ops;

void tmp421_sample_entry(ULONG input)
{
    (void)input;
    struct tmp421_device dev;
    int ret;

    
    tmp421_init(&dev, g_tmp421_i2c_context, &g_tmp421_i2c_ops, TMP421_ADDR_DEFAULT);

    ret = tmp421_probe(&dev);
    if (ret != 0) {
        printf("TMP421 Remote Temperature Sensor not found!\r\n");
        return;
    }

    printf("TMP421 Remote Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t local_t, remote_t;
    ret = tmp421_read_local_temp(&dev, &local_t);
    if (ret == 0) {
        printf("TMP421 local: %d.%03d C\r\n", (int)(local_t / 1000), (int)(local_t >= 0 ? local_t % 1000 : (-local_t) % 1000));
    }
    ret = tmp421_read_remote_temp(&dev, &remote_t);
    if (ret == 0) {
        printf("TMP421 remote: %d.%03d C\r\n", (int)(remote_t / 1000), (int)(remote_t >= 0 ? remote_t % 1000 : (-remote_t) % 1000));
    }
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
