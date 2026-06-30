/*
 * DHT22 sample for ChibiOS
 */
#include "dht22_ref.h"
#include "chprintf.h"

void dht22_sample_thread(void *arg)
{
    (void)arg;
    struct dht22_device sensor;
    dht22_init(&sensor, GPIOA, 0);

    while (true) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            chprintf((BaseSequentialStream *)&SD1, "T:%d.%d C  H:%d.%d %%\r\n",
                     temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        chThdSleepMilliseconds(DHT22_MIN_INTERVAL_MS);
    }
}
