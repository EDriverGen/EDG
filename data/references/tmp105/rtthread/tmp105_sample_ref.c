/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP105 sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <tmp105_ref.h>

static void tmp105_sample_usage(void)
{
    rt_kprintf("usage: tmp105_sample [count] [interval_ms]\r\n");
    rt_kprintf("example: tmp105_sample\r\n");
    rt_kprintf("example: tmp105_sample 10 1000\r\n");
}

static rt_bool_t tmp105_parse_u32(const char *text, rt_uint32_t *value)
{
    char *end = RT_NULL;
    unsigned long result;

    if ((text == RT_NULL) || (value == RT_NULL))
        return RT_FALSE;
    if (*text == '-')
        return RT_FALSE;

    result = strtoul(text, &end, 0);
    if ((end == text) || (*end != '\0'))
        return RT_FALSE;

    *value = (rt_uint32_t)result;
    return RT_TRUE;
}

static void tmp105_print_temperature(rt_int32_t temp_mcelsius)
{
    const char *sign = (temp_mcelsius < 0) ? "-" : "";
    rt_uint32_t abs_temp = (temp_mcelsius < 0) ?
                           (rt_uint32_t)(-temp_mcelsius) :
                           (rt_uint32_t)temp_mcelsius;

    rt_kprintf("%s%u.%03u C", sign, abs_temp / 1000U, abs_temp % 1000U);
}

static void tmp105_sample(int argc, char *argv[])
{
    struct tmp105_device sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_uint32_t index;
    rt_err_t result;

    if (argc >= 2)
    {
        if (!tmp105_parse_u32(argv[1], &count) || (count == 0))
        {
            tmp105_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!tmp105_parse_u32(argv[2], &interval_ms))
        {
            tmp105_sample_usage();
            return;
        }
    }

    if (argc > 3)
    {
        tmp105_sample_usage();
        return;
    }

    result = tmp105_init(&sensor, TMP105_DEFAULT_BUS_NAME, TMP105_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("TMP105 init failed, bus=%s, ret=%d\r\n",
                   TMP105_DEFAULT_BUS_NAME, result);
        return;
    }

    result = tmp105_probe(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("TMP105 probe failed, ret=%d\r\n", result);
        return;
    }

    for (index = 0; index < count; index++)
    {
        rt_int32_t temp_mc;

        result = tmp105_read_temperature(&sensor, &temp_mc);
        if (result == RT_EOK)
        {
            rt_kprintf("[%u] temperature: ", index);
            tmp105_print_temperature(temp_mc);
            rt_kprintf("\r\n");
        }
        else
        {
            rt_kprintf("[%u] read failed, ret=%d\r\n", index, result);
        }

        if ((count > 1) && (index < count - 1))
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(tmp105_sample, read TMP105 temperature sensor);
