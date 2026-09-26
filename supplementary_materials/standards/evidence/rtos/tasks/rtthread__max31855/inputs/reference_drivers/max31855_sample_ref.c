/*
 * MAX31855 sample for RT-Thread
 */
#include "max31855_ref.h"

static void max31855_sample(int argc, char *argv[])
{
    struct max31855_device tc;
    const char *dev = (argc > 1) ? argv[1] : "spi10";

    if (max31855_init(&tc, dev) != RT_EOK)
    { rt_kprintf("MAX31855 init failed\n"); return; }

    for (int i = 0; i < 5; i++)
    {
        rt_uint32_t raw;
        rt_int32_t tc_temp, int_temp;
        if (max31855_read_raw(&tc, &raw) != RT_EOK)
        { rt_kprintf("Read error\n"); rt_thread_mdelay(1000); continue; }

        if (max31855_has_fault(raw))
            rt_kprintf("Fault: 0x%02x\n", max31855_get_fault(raw));
        else {
            max31855_get_thermocouple_temp(raw, &tc_temp);
            max31855_get_internal_temp(raw, &int_temp);
            rt_kprintf("TC:%d.%03dC Int:%d.%03dC\n",
                       tc_temp/1000,(tc_temp>=0?tc_temp:-tc_temp)%1000,
                       int_temp/1000,(int_temp>=0?int_temp:-int_temp)%1000);
        }
        rt_thread_mdelay(1000);
    }
}
MSH_CMD_EXPORT(max31855_sample, "MAX31855 thermocouple sensor");
