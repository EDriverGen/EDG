/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP421 sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <tmp421_ref.h>

static void tmp421_sample_usage(void)
{
    rt_kprintf("usage: tmp421_sample [count] [interval_ms]\r\n");
}

static rt_bool_t tmp421_parse_u32(const char *text, rt_uint32_t *value)
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

static void tmp421_print_temperature(rt_int32_t temp_mc)
{
    const char *sign = (temp_mc < 0) ? "-" : "";
    rt_uint32_t abs_temp = (temp_mc < 0) ?
                           (rt_uint32_t)(-temp_mc) :
                           (rt_uint32_t)temp_mc;

    rt_kprintf("%s%u.%03u C", sign, abs_temp / 1000U, abs_temp % 1000U);
}

static void tmp421_sample(int argc, char *argv[])
{
    struct tmp421_device sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_uint32_t index;
    rt_err_t result;

    if (argc >= 2)
    {
        if (!tmp421_parse_u32(argv[1], &count) || (count == 0))
        {
            tmp421_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!tmp421_parse_u32(argv[2], &interval_ms))
        {
            tmp421_sample_usage();
            return;
        }
    }

    result = tmp421_init(&sensor, TMP421_DEFAULT_BUS_NAME, TMP421_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("TMP421 init failed, ret=%d\r\n", result);
        return;
    }

    result = tmp421_probe(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("TMP421 probe failed, ret=%d\r\n", result);
        return;
    }

    for (index = 0; index < count; index++)
    {
        rt_int32_t local_mc, remote_mc;

        result = tmp421_read_local_temp(&sensor, &local_mc);
        if (result == RT_EOK)
        {
            rt_kprintf("[%u] local: ", index);
            tmp421_print_temperature(local_mc);
        }
        else
        {
            rt_kprintf("[%u] local read failed, ret=%d", index, result);
        }

        result = tmp421_read_remote_temp(&sensor, &remote_mc);
        if (result == RT_EOK)
        {
            rt_kprintf("  remote: ");
            tmp421_print_temperature(remote_mc);
        }

        rt_kprintf("\r\n");

        if ((count > 1) && (index < count - 1))
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(tmp421_sample, read TMP421 local and remote temperature);
