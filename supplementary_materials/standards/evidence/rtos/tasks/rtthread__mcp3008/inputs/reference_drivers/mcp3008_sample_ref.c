/*
 * MCP3008 sample for RT-Thread
 */
#include "mcp3008_ref.h"

static void mcp3008_sample(int argc, char *argv[])
{
    struct mcp3008_device adc;
    const char *dev = (argc > 1) ? argv[1] : "spi10";

    if (mcp3008_init(&adc, dev, 3300) != RT_EOK)
    { rt_kprintf("MCP3008 init failed\n"); return; }

    for (int i = 0; i < 8; i++)
    {
        rt_uint16_t raw, mv;
        if (mcp3008_read_raw(&adc, i, MCP3008_SINGLE, &raw) == RT_EOK)
        {
            mcp3008_read_voltage(&adc, i, &mv);
            rt_kprintf("CH%d: raw=%d  %d.%03d V\n", i, raw, mv/1000, mv%1000);
        }
        else
            rt_kprintf("CH%d: read error\n", i);
    }
}
MSH_CMD_EXPORT(mcp3008_sample, "MCP3008 ADC sensor");
