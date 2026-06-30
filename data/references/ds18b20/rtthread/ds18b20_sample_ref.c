/*
 * DS18B20 sample for RT-Thread
 */
#include "ds18b20_ref.h"

static void ds18b20_sample(int argc, char *argv[])
{
    struct ds18b20_device sensor;
    rt_base_t pin = 0;  /* PA0 */
    if (ds18b20_init(&sensor, pin) != RT_EOK)
    { rt_kprintf("DS18B20 init failed\n"); return; }
    for (int i = 0; i < 5; i++) {
        int32_t temp;
        if (ds18b20_read_temp(&sensor, &temp) == RT_EOK)
            rt_kprintf("T:%d.%02d C\n", (int)(temp/100), (int)((temp>=0?temp:-temp)%100));
        else rt_kprintf("Read error\n");
        rt_thread_mdelay(1000);
    }
}
MSH_CMD_EXPORT(ds18b20_sample, "DS18B20 temperature sensor");
