/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin          add LM75A sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <lm75a.h>

/*
 * 打印命令用法说明。
 */
static void lm75a_sample_usage(void)
{
    rt_kprintf("usage: lm75a_sample [count] [interval_ms]\r\n");
    rt_kprintf("example: lm75a_sample\r\n");
    rt_kprintf("example: lm75a_sample 10 1000\r\n");
}

/*
 * 把命令行中的正整数参数解析成 rt_uint32_t。
 */
static rt_bool_t lm75a_parse_u32(const char *text, rt_uint32_t *value)
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
 * 把毫摄氏度转换为摄氏度，以易读的格式打印
 * 例如：
 * 25000  -> 25.000 C
 * -125   -> -0.125 C
 */
static void lm75a_print_temperature(rt_int32_t temp_mcelsius)
{
    const char *sign;
    rt_uint32_t abs_temp_mcelsius;

    sign = (temp_mcelsius < 0) ? "-" : "";
    abs_temp_mcelsius = (temp_mcelsius < 0) ?
                        (rt_uint32_t)(-temp_mcelsius) :
                        (rt_uint32_t)temp_mcelsius;

    rt_kprintf("%s%u.%03u C",
               sign,
               abs_temp_mcelsius / 1000U,
               abs_temp_mcelsius % 1000U);
}

/*
 * 读取并打印 LM75A 温度值
 * 命令设计说明：
 * 1. 默认只读 1 次
 * 2. 如果传入 count 和 interval_ms，就支持连续采样
 */
static void lm75a_sample(int argc, char *argv[])
{
    struct lm75a_device temp_sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_uint32_t index;
    rt_err_t result;
    rt_uint8_t config;

    if (argc >= 2)
    {
        if (!lm75a_parse_u32(argv[1], &count) || (count == 0))
        {
            lm75a_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!lm75a_parse_u32(argv[2], &interval_ms))
        {
            lm75a_sample_usage();
            return;
        }
    }

    if (argc > 3)
    {
        lm75a_sample_usage();
        return;
    }

    result = lm75a_init(&temp_sensor, LM75A_DEFAULT_BUS_NAME, LM75A_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("LM75A init failed, bus=%s, ret=%d\r\n", LM75A_DEFAULT_BUS_NAME, result);
        return;
    }

    result = lm75a_probe(&temp_sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("LM75A probe failed, addr=0x%02X, ret=%d\r\n", temp_sensor.addr, result);
        return;
    }

    result = lm75a_read_config(&temp_sensor, &config);
    if (result != RT_EOK)
    {
        rt_kprintf("LM75A read config failed, ret=%d\r\n", result);
        return;
    }

    rt_kprintf("LM75A bus=%s addr=0x%02X config=0x%02X\r\n",
               temp_sensor.bus_name,
               temp_sensor.addr,
               config);

    for (index = 0; index < count; index++)
    {
        rt_int16_t raw;
        rt_int32_t temp_mcelsius;

        result = lm75a_read_raw(&temp_sensor, &raw);
        if (result != RT_EOK)
        {
            rt_kprintf("LM75A read failed at sample %u, ret=%d\r\n", index + 1, result);
            return;
        }

        temp_mcelsius = lm75a_raw_to_mcelsius(raw);

        rt_kprintf("[%u/%u] raw=%d, temp=", index + 1, count, raw);
        lm75a_print_temperature(temp_mcelsius);
        rt_kprintf("\r\n");

        if ((index + 1) < count)
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(lm75a_sample, read LM75A temperature by standard driver);
