/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add EMC1413 sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <emc1413_ref.h>

static const char *emc1413_ch_names[] = { "internal", "external1", "external2" };

static void emc1413_sample_usage(void)
{
    rt_kprintf("usage: emc1413_sample [count] [interval_ms]\r\n");
}

static rt_bool_t emc1413_parse_u32(const char *text, rt_uint32_t *value)
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

static void emc1413_print_temperature(rt_int32_t temp_mc)
{
    const char *sign = (temp_mc < 0) ? "-" : "";
    rt_uint32_t abs_temp = (temp_mc < 0) ?
                           (rt_uint32_t)(-temp_mc) :
                           (rt_uint32_t)temp_mc;

    rt_kprintf("%s%u.%03u C", sign, abs_temp / 1000U, abs_temp % 1000U);
}

static void emc1413_sample(int argc, char *argv[])
{
    struct emc1413_device sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 1000;
    rt_uint32_t index;
    rt_err_t result;

    if (argc >= 2)
    {
        if (!emc1413_parse_u32(argv[1], &count) || (count == 0))
        {
            emc1413_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!emc1413_parse_u32(argv[2], &interval_ms))
        {
            emc1413_sample_usage();
            return;
        }
    }

    result = emc1413_init(&sensor, EMC1413_DEFAULT_BUS_NAME, EMC1413_DEFAULT_ADDR);
    if (result != RT_EOK)
    {
        rt_kprintf("EMC1413 init failed, ret=%d\r\n", result);
        return;
    }

    result = emc1413_probe(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("EMC1413 probe failed, ret=%d\r\n", result);
        return;
    }

    for (index = 0; index < count; index++)
    {
        int ch;

        rt_kprintf("[%u]", index);

        for (ch = 0; ch < EMC1413_CH_COUNT; ch++)
        {
            rt_int32_t temp_mc;

            result = emc1413_read_temperature(&sensor,
                                              (enum emc1413_channel)ch,
                                              &temp_mc);
            if (result == RT_EOK)
            {
                rt_kprintf(" %s: ", emc1413_ch_names[ch]);
                emc1413_print_temperature(temp_mc);
            }
            else
            {
                rt_kprintf(" %s: err=%d", emc1413_ch_names[ch], result);
            }
        }

        rt_kprintf("\r\n");

        if ((count > 1) && (index < count - 1))
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(emc1413_sample, read EMC1413 three-channel temperature sensor);
