/*
 * DHT22 sample for RIOT
 */
#include "dht22_ref.h"
#include <stdio.h>

int main(void)
{
    struct dht22_device sensor;
    dht22_init(&sensor, GPIO_PIN(0, 0));

    while (1) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            printf("T:%d.%d C  H:%d.%d %%\n", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        xtimer_msleep(DHT22_MIN_INTERVAL_MS);
    }
    return 0;
}
