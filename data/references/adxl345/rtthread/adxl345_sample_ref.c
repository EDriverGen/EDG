/*
 * ADXL345 sample for RT-Thread
 */
#include "adxl345_ref.h"

static void adxl345_sample(int argc, char *argv[])
{
    struct adxl345_device acc;
    const char *dev = (argc > 1) ? argv[1] : "spi10";

    if (adxl345_init(&acc, dev, ADXL345_RANGE_2G) != RT_EOK)
    { rt_kprintf("ADXL345 init failed\n"); return; }

    for (int i = 0; i < 10; i++)
    {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == RT_EOK)
            rt_kprintf("X:%d Y:%d Z:%d mg\n", x, y, z);
        else
            rt_kprintf("Read error\n");
        rt_thread_mdelay(100);
    }
}
MSH_CMD_EXPORT(adxl345_sample, "ADXL345 accelerometer sensor");
