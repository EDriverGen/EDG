/*
 * DHT22 sample for ThreadX
 */
#include "dht22_ref.h"
#include <stdio.h>

extern const struct dht22_gpio_ops platform_gpio_ops;
extern void *platform_gpio_ctx;

void dht22_sample_entry(ULONG param)
{
    (void)param;
    struct dht22_device sensor;
    if (dht22_init(&sensor, &platform_gpio_ops, platform_gpio_ctx) != 0)
    { printf("Init failed\r\n"); return; }
    for (int i = 0; i < 5; i++) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            printf("T:%d.%d C  H:%d.%d %%\r\n", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        tx_thread_sleep(200);
    }
}
