/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add EMC1413 driver with standard structure
 */
#include <emc1413_ref.h>

/* ---- 内部辅助 ---- */

static rt_bool_t emc1413_is_device_ready(struct emc1413_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

static rt_err_t emc1413_read_register(struct emc1413_device *dev,
                                      rt_uint8_t             reg,
                                      rt_uint8_t            *value)
{
    struct rt_i2c_msg msgs[2];

    if (!emc1413_is_device_ready(dev) || (value == RT_NULL))
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

static rt_err_t emc1413_write_register(struct emc1413_device *dev,
                                       rt_uint8_t             reg,
                                       rt_uint8_t             value)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[2];

    if (!emc1413_is_device_ready(dev))
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
 * 获取指定通道的高/低字节寄存器地址。
 */
static rt_err_t emc1413_get_temp_regs(enum emc1413_channel channel,
                                      rt_uint8_t          *hi_reg,
                                      rt_uint8_t          *lo_reg)
{
    switch (channel)
    {
    case EMC1413_CH_INTERNAL:
        *hi_reg = EMC1413_REG_INTERNAL_TEMP_HI;
        *lo_reg = EMC1413_REG_INTERNAL_TEMP_LO;
        break;
    case EMC1413_CH_EXTERNAL_1:
        *hi_reg = EMC1413_REG_EXT1_TEMP_HI;
        *lo_reg = EMC1413_REG_EXT1_TEMP_LO;
        break;
    case EMC1413_CH_EXTERNAL_2:
        *hi_reg = EMC1413_REG_EXT2_TEMP_HI;
        *lo_reg = EMC1413_REG_EXT2_TEMP_LO;
        break;
    default:
        return -RT_EINVAL;
    }
    return RT_EOK;
}

/*
 * EMC1413 温度格式：
 *   高字节: 整数部分，有符号 8 位
 *   低字节: 小数部分，高 3 位有效 (bit 7/6/5)，分辨率 0.125 C
 *
 * 转换公式: temp_mc = hi * 1000 + ((lo >> 5) & 0x07) * 125
 */
static rt_int32_t emc1413_raw_to_mcelsius(rt_uint8_t hi, rt_uint8_t lo)
{
    rt_int32_t integer_part;
    rt_uint32_t frac_part;

    integer_part = (rt_int8_t)hi;
    frac_part = ((lo >> 5) & 0x07) * 125;

    return integer_part * 1000 + (rt_int32_t)frac_part;
}

/* ---- 公开接口 ---- */

rt_err_t emc1413_init(struct emc1413_device *dev,
                      const char            *bus_name,
                      rt_uint8_t             addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : EMC1413_DEFAULT_BUS_NAME;
    if (addr == 0)
    {
        addr = EMC1413_DEFAULT_ADDR;
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

rt_err_t emc1413_probe(struct emc1413_device *dev)
{
    rt_err_t result;
    rt_uint8_t mfr_id;

    result = emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &mfr_id);
    if (result != RT_EOK)
    {
        return result;
    }

    if (mfr_id != EMC1413_MANUFACTURER_ID)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t emc1413_read_temperature(struct emc1413_device *dev,
                                  enum emc1413_channel   channel,
                                  rt_int32_t            *temp_mcelsius)
{
    rt_err_t result;
    rt_uint8_t hi_reg, lo_reg;
    rt_uint8_t hi, lo;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = emc1413_get_temp_regs(channel, &hi_reg, &lo_reg);
    if (result != RT_EOK)
    {
        return result;
    }

    result = emc1413_read_register(dev, hi_reg, &hi);
    if (result != RT_EOK)
    {
        return result;
    }

    result = emc1413_read_register(dev, lo_reg, &lo);
    if (result != RT_EOK)
    {
        return result;
    }

    *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
    return RT_EOK;
}

rt_err_t emc1413_set_extended_range(struct emc1413_device *dev,
                                    rt_bool_t              enable)
{
    rt_err_t result;
    rt_uint8_t config;

    result = emc1413_read_register(dev, EMC1413_REG_CONFIG, &config);
    if (result != RT_EOK)
    {
        return result;
    }

    if (enable)
    {
        config |= EMC1413_CONFIG_RANGE;
    }
    else
    {
        config &= ~EMC1413_CONFIG_RANGE;
    }

    return emc1413_write_register(dev, EMC1413_REG_CONFIG, config);
}
