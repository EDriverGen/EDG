/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add DPS310 sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <dps310_ref.h>

static void dps310_sample_usage(void)
{
    rt_kprintf("usage: dps310_sample [count] [interval_ms]\r\n");
}

static rt_bool_t dps310_parse_u32(const char *text, rt_uint32_t *value)
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

static void dps310_sample(int argc, char *argv[])
{
    struct dps310_device sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_uint32_t index;
    rt_err_t result;

    if (argc >= 2)
    {
        if (!dps310_parse_u32(argv[1], &count) || (count == 0))
        {
            dps310_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!dps310_parse_u32(argv[2], &interval_ms))
        {
            dps310_sample_usage();
            return;
        }
    }

    result = dps310_init(&sensor, DPS310_DEFAULT_BUS_NAME, DPS310_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("DPS310 init failed, ret=%d\r\n", result);
        return;
    }

    result = dps310_probe(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("DPS310 probe failed, ret=%d\r\n", result);
        return;
    }

    result = dps310_reset(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("DPS310 reset failed, ret=%d\r\n", result);
        return;
    }

    result = dps310_read_calibration(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("DPS310 calibration read failed, ret=%d\r\n", result);
        return;
    }

    for (index = 0; index < count; index++)
    {
        rt_int32_t temp_c100;
        rt_int32_t pres_pa100;

        result = dps310_read_temperature(&sensor, &temp_c100);
        if (result == RT_EOK)
        {
            const char *sign = (temp_c100 < 0) ? "-" : "";
            rt_uint32_t abs_t = (temp_c100 < 0) ?
                                (rt_uint32_t)(-temp_c100) :
                                (rt_uint32_t)temp_c100;
            rt_kprintf("[%u] temp: %s%u.%02u C", index,
                       sign, abs_t / 100U, abs_t % 100U);
        }
        else
        {
            rt_kprintf("[%u] temp read failed, ret=%d", index, result);
        }

        result = dps310_read_pressure(&sensor, &pres_pa100);
        if (result == RT_EOK)
        {
            rt_kprintf("  pressure: %u.%02u Pa",
                       (rt_uint32_t)(pres_pa100 / 100),
                       (rt_uint32_t)(pres_pa100 % 100));
        }
        else
        {
            rt_kprintf("  pressure read failed, ret=%d", result);
        }

        rt_kprintf("\r\n");

        if ((count > 1) && (index < count - 1))
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(dps310_sample, read DPS310 pressure and temperature);
