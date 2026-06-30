/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin        move BH1750 logic into dedicated driver layer
 */
#include <rtthread.h>
#include <stdlib.h>
#include <bh1750.h>

/*
 * 打印命令使用说明。
 */
static void bh1750_sample_usage(void)
{
    rt_kprintf("usage: bh1750_sample [count] [interval_ms]\r\n");
    rt_kprintf("example: bh1750_sample\r\n");
    rt_kprintf("example: bh1750_sample 10 1000\r\n");
}

/*
 * 把命令行中的正整数参数解析成 rt_uint32_t。
 * 如果参数无效，就直接返回 RT_FALSE，让上层统一打印用法说明。
 */
static rt_bool_t bh1750_parse_u32(const char *text, rt_uint32_t *value)
{
    char *end = RT_NULL;
    unsigned long result;

    if ((text == RT_NULL) || (value == RT_NULL))
    {
        return RT_FALSE;
    }

    if (*text == '-')
    {
        return RT_FALSE;
    }

    result = strtoul(text, &end, 0);
    if ((end == text) || (*end != '\0'))
    {
        return RT_FALSE;
    }

    *value = (rt_uint32_t)result;
    return RT_TRUE;
}

/*
 * 读取并打印 BH1750 的光照值。
 *
 * 命令设计说明：
 * 1. 默认读 1 次，更符合 sample 这个名字
 * 2. 如果传入 count 和 interval_ms，就可以连续读多次
 * 3. 真正的 BH1750 读数逻辑已经全部下沉到驱动层
 */
static void bh1750_sample(int argc, char *argv[])
{
    struct bh1750_device light_sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_err_t result;
    rt_uint32_t index;

    if (argc >= 2)
    {
        if (!bh1750_parse_u32(argv[1], &count) || (count == 0))
        {
            bh1750_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!bh1750_parse_u32(argv[2], &interval_ms))
        {
            bh1750_sample_usage();
            return;
        }
    }

    if (argc > 3)
    {
        bh1750_sample_usage();
        return;
    }

    result = bh1750_init(&light_sensor, BH1750_DEFAULT_BUS_NAME, BH1750_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("BH1750 init failed, bus=%s, ret=%d\r\n", BH1750_DEFAULT_BUS_NAME, result);
        return;
    }

    result = bh1750_probe(&light_sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("BH1750 probe failed, addr=0x%02X, ret=%d\r\n", light_sensor.addr, result);
        return;
    }

    rt_kprintf("BH1750 bus=%s addr=0x%02X mode=%s\r\n",
               light_sensor.bus_name,
               light_sensor.addr,
               bh1750_mode_to_string(light_sensor.mode));

    for (index = 0; index < count; index++)
    {
        rt_uint16_t raw;
        rt_uint32_t lux_x100;

        result = bh1750_read_raw(&light_sensor, &raw);
        if (result != RT_EOK)
        {
            rt_kprintf("BH1750 read failed at sample %u, ret=%d\r\n", index + 1, result);
            return;
        }

        lux_x100 = bh1750_raw_to_lux_x100(raw);

        rt_kprintf("[%u/%u] raw=%u, lux=%u.%02u\r\n",
                   index + 1,
                   count,
                   raw,
                   lux_x100 / 100,
                   lux_x100 % 100);

        if ((index + 1) < count)
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(bh1750_sample, read BH1750 light level by standard driver);
