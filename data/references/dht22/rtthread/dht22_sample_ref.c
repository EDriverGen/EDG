/*
 * DHT22 sample for RT-Thread
 */
#include "dht22_ref.h"

static void dht22_sample(int argc, char *argv[])
{
    struct dht22_device sensor;
    rt_base_t pin = 0;  /* PA0, user should modify */

    if (dht22_init(&sensor, pin) != RT_EOK)
    { rt_kprintf("DHT22 init failed\n"); return; }

    for (int i = 0; i < 5; i++)
    {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == RT_EOK)
            rt_kprintf("T:%d.%d C  H:%d.%d %%\n",
                       temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        else
            rt_kprintf("Read error\n");
        rt_thread_mdelay(DHT22_MIN_INTERVAL_MS);
    }
}
MSH_CMD_EXPORT(dht22_sample, "DHT22 temperature/humidity sensor");
