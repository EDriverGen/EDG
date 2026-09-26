/*
 * DHT22 sample for XiUOS
 */
#include "dht22_ref.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    struct dht22_device sensor;
    const char *path = (argc > 1) ? argv[1] : "/dev/gpio0";
    if (dht22_init(&sensor, path) != 0)
    { printf("Init failed\n"); return -1; }
    for (int i = 0; i < 5; i++) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            printf("T:%d.%d C  H:%d.%d %%\n", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        PrivTaskDelay(DHT22_MIN_INTERVAL_MS);
    }
    dht22_deinit(&sensor);
    return 0;
}
