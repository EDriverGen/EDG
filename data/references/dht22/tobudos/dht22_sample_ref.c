/*
 * DHT22 sample for TencentOS Tiny
 */
#include "dht22_ref.h"
#include <stdio.h>

void dht22_sample_task(void *arg)
{
    (void)arg;
    struct dht22_device sensor;
    dht22_init(&sensor, GPIOA, GPIO_PIN_0);
    for (int i = 0; i < 5; i++) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            printf("T:%d.%d C  H:%d.%d %%\r\n", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        tos_task_delay(DHT22_MIN_INTERVAL_MS);
    }
}
