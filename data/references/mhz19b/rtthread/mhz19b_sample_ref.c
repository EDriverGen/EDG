/*
 * MH-Z19B sample for RT-Thread
 */
#include "mhz19b_ref.h"

static void mhz19b_sample(int argc, char *argv[])
{
    struct mhz19b_device co2;
    const char *dev = (argc > 1) ? argv[1] : "uart2";

    if (mhz19b_init(&co2, dev) != RT_EOK)
    { rt_kprintf("MH-Z19B init failed\n"); return; }

    for (int i = 0; i < 5; i++)
    {
        rt_uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == RT_EOK)
            rt_kprintf("CO2: %d ppm\n", ppm);
        else
            rt_kprintf("Read error\n");
        rt_thread_mdelay(2000);
    }
}
MSH_CMD_EXPORT(mhz19b_sample, "MH-Z19B CO2 sensor");
