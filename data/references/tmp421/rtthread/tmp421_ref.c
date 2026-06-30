/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP421 driver with standard structure
 */
#include <tmp421_ref.h>

/* ---- 内部辅助 ---- */

static rt_bool_t tmp421_is_device_ready(struct tmp421_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

static rt_err_t tmp421_read_register(struct tmp421_device *dev,
                                     rt_uint8_t            reg,
                                     rt_uint8_t           *value)
{
    struct rt_i2c_msg msgs[2];

    if (!tmp421_is_device_ready(dev) || (value == RT_NULL))
    {
        return -RT_EINVAL;
    }

    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len   = 1;
    msgs[1].buf   = value;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

static rt_err_t tmp421_write_register(struct tmp421_device *dev,
                                      rt_uint8_t            reg,
                                      rt_uint8_t            value)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[2];

    if (!tmp421_is_device_ready(dev))
    {
        return -RT_EINVAL;
    }

    frame[0] = reg;
    frame[1] = value;

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.len   = 2;
    msg.buf   = frame;

    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/*
 * TMP421 温度为 12 位有符号数，存储在 hi/lo 两个独立寄存器中：
 *   hi: 整数部分（有符号 8 位）
 *   lo: 小数部分（高 4 位有效），单位 0.0625 C
 *
 * 扩展范围模式下有 64 C 的偏移量需要减去。
 */
static rt_int32_t tmp421_raw_to_mcelsius(rt_uint8_t hi, rt_uint8_t lo)
{
    rt_int16_t raw;

    raw = ((rt_int16_t)((rt_uint16_t)hi << 8 | lo)) >> 4;
    return (rt_int32_t)raw * 625 / 10;
}

/* ---- 公开接口 ---- */

rt_err_t tmp421_init(struct tmp421_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : TMP421_DEFAULT_BUS_NAME;
    if (addr == 0)
    {
        addr = TMP421_DEFAULT_ADDR;
    }

    dev->bus = (struct rt_i2c_bus_device *)rt_device_find(target_bus_name);
    if (dev->bus == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    dev->bus_name = target_bus_name;
    dev->addr     = addr;

    return RT_EOK;
}

rt_err_t tmp421_probe(struct tmp421_device *dev)
{
    rt_err_t result;
    rt_uint8_t mfr_id;

    result = tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mfr_id);
    if (result != RT_EOK)
    {
        return result;
    }

    if (mfr_id != TMP421_MANUFACTURER_ID_TI)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t tmp421_read_local_temp(struct tmp421_device *dev,
                                rt_int32_t           *temp_mcelsius)
{
    rt_err_t result;
    rt_uint8_t hi, lo;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_HI, &hi);
    if (result != RT_EOK)
        return result;

    result = tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_LO, &lo);
    if (result != RT_EOK)
        return result;

    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return RT_EOK;
}

rt_err_t tmp421_read_remote_temp(struct tmp421_device *dev,
                                 rt_int32_t           *temp_mcelsius)
{
    rt_err_t result;
    rt_uint8_t hi, lo;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_HI, &hi);
    if (result != RT_EOK)
        return result;

    result = tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_LO, &lo);
    if (result != RT_EOK)
        return result;

    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return RT_EOK;
}

rt_err_t tmp421_set_extended_range(struct tmp421_device *dev, rt_bool_t enable)
{
    rt_err_t result;
    rt_uint8_t config;

    result = tmp421_read_register(dev, TMP421_REG_CONFIG_1, &config);
    if (result != RT_EOK)
    {
        return result;
    }

    if (enable)
    {
        config |= TMP421_CONFIG1_RANGE;
    }
    else
    {
        config &= ~TMP421_CONFIG1_RANGE;
    }

    return tmp421_write_register(dev, TMP421_REG_CONFIG_1_WR, config);
}
