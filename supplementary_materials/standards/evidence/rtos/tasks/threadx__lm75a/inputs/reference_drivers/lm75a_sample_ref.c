/*
 * SPDX-License-Identifier: MIT
 *
 * LM75A Temperature Sensor Driver for ThreadX
 */
#include "lm75a_ref.h"
#include <tx_api.h>
#include <stdio.h>
extern void *g_lm75a_i2c_context;
extern const struct lm75a_i2c_ops g_lm75a_i2c_ops;

void lm75a_sample_entry(ULONG input)
{
    (void)input;
    struct lm75a_device dev;
    int ret;

    
    lm75a_init(&dev, g_lm75a_i2c_context, &g_lm75a_i2c_ops, LM75A_DEFAULT_ADDR);

    ret = lm75a_probe(&dev);
    if (ret != 0) {
        printf("LM75A Temperature Sensor not found!\r\n");
        return;
    }

    printf("LM75A Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = lm75a_read_temperature(&dev, &temp);
    if (ret == 0) {
        printf("LM75A: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}
