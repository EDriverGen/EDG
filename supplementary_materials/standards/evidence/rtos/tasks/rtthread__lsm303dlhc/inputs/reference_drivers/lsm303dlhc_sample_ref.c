/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add LSM303DLHC sample command
 */
#include <rtthread.h>
#include <stdlib.h>
#include <lsm303dlhc_ref.h>

static void lsm303dlhc_sample_usage(void)
{
    rt_kprintf("usage: lsm303dlhc_sample [count] [interval_ms]\r\n");
}

static rt_bool_t lsm303dlhc_parse_u32(const char *text, rt_uint32_t *value)
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

static void lsm303dlhc_sample(int argc, char *argv[])
{
    struct lsm303dlhc_device sensor;
    rt_uint32_t count = 1;
    rt_uint32_t interval_ms = 500;
    rt_uint32_t index;
    rt_err_t result;

    if (argc >= 2)
    {
        if (!lsm303dlhc_parse_u32(argv[1], &count) || (count == 0))
        {
            lsm303dlhc_sample_usage();
            return;
        }
    }

    if (argc >= 3)
    {
        if (!lsm303dlhc_parse_u32(argv[2], &interval_ms))
        {
            lsm303dlhc_sample_usage();
            return;
        }
    }

    result = lsm303dlhc_init(&sensor, LSM303DLHC_DEFAULT_BUS_NAME);
    if (result != RT_EOK)
    {
        rt_kprintf("LSM303DLHC init failed, ret=%d\r\n", result);
        return;
    }

    result = lsm303dlhc_probe(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("LSM303DLHC probe failed, ret=%d\r\n", result);
        return;
    }

    result = lsm303dlhc_accel_start(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("LSM303DLHC accel start failed, ret=%d\r\n", result);
        return;
    }

    result = lsm303dlhc_mag_start(&sensor);
    if (result != RT_EOK)
    {
        rt_kprintf("LSM303DLHC mag start failed, ret=%d\r\n", result);
        return;
    }

    rt_thread_mdelay(100);

    for (index = 0; index < count; index++)
    {
        struct lsm303dlhc_xyz accel, mag;

        result = lsm303dlhc_accel_read_raw(&sensor, &accel);
        if (result == RT_EOK)
        {
            rt_kprintf("[%u] accel: x=%d y=%d z=%d", index,
                       accel.x, accel.y, accel.z);
        }
        else
        {
            rt_kprintf("[%u] accel read failed, ret=%d", index, result);
        }

        result = lsm303dlhc_mag_read_raw(&sensor, &mag);
        if (result == RT_EOK)
        {
            rt_kprintf("  mag: x=%d y=%d z=%d", mag.x, mag.y, mag.z);
        }
        else
        {
            rt_kprintf("  mag read failed, ret=%d", result);
        }

        rt_kprintf("\r\n");

        if ((count > 1) && (index < count - 1))
        {
            rt_thread_mdelay(interval_ms);
        }
    }
}

MSH_CMD_EXPORT(lsm303dlhc_sample, read LSM303DLHC accelerometer and magnetometer);
