/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP105 driver with standard structure
 */
#include <tmp105_ref.h>

/* ---- 内部辅助 ---- */

static rt_bool_t tmp105_is_device_ready(struct tmp105_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

/*
 * 通用寄存器读取：先写寄存器号，再读数据。
 */
static rt_err_t tmp105_read_registers(struct tmp105_device *dev,
                                      rt_uint8_t            reg,
                                      rt_uint8_t           *buffer,
                                      rt_size_t             size)
{
    struct rt_i2c_msg msgs[2];

    if (!tmp105_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0))
    {
        return -RT_EINVAL;
    }

    msgs[0].addr  = dev->addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = dev->addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len   = size;
    msgs[1].buf   = buffer;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/*
 * 向指定寄存器写入 1~2 字节数据。
 */
static rt_err_t tmp105_write_registers(struct tmp105_device *dev,
                                       rt_uint8_t            reg,
                                       const rt_uint8_t     *buffer,
                                       rt_size_t             size)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[3];

    if (!tmp105_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0) || (size > 2))
    {
        return -RT_EINVAL;
    }

    frame[0] = reg;
    rt_memcpy(&frame[1], buffer, size);

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.len   = size + 1;
    msg.buf   = frame;

    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/*
 * 把温度寄存器的两字节解析成毫摄氏度。
 *
 * TMP105 在 12 位模式下：
 *   温度值 = (高字节 << 4 | 低字节 >> 4) * 0.0625
 *   数据格式是二进制补码，有效位为高 12 位。
 */
static rt_int32_t tmp105_raw_to_mcelsius(const rt_uint8_t data[2])
{
    rt_int16_t raw;

    raw = ((rt_int16_t)((rt_uint16_t)data[0] << 8 | data[1])) >> 4;
    /* raw 的单位是 0.0625 C，即 62.5 mC */
    return (rt_int32_t)raw * 625 / 10;
}

/* ---- 公开接口 ---- */

rt_bool_t tmp105_is_valid_address(rt_uint8_t addr)
{
    return (addr == TMP105_ADDR_LOW) || (addr == TMP105_ADDR_HIGH);
}

rt_err_t tmp105_init(struct tmp105_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : TMP105_DEFAULT_BUS_NAME;
    if (addr == 0)
    {
        addr = TMP105_DEFAULT_ADDR;
    }

    if (!tmp105_is_valid_address(addr))
    {
        return -RT_EINVAL;
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

rt_err_t tmp105_probe(struct tmp105_device *dev)
{
    rt_uint8_t config;

    return tmp105_read_registers(dev, TMP105_REG_CONFIG, &config, 1);
}

rt_err_t tmp105_read_temperature(struct tmp105_device *dev,
                                 rt_int32_t           *temp_mcelsius)
{
    rt_err_t result;
    rt_uint8_t data[2];

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = tmp105_read_registers(dev, TMP105_REG_TEMPERATURE, data, 2);
    if (result != RT_EOK)
    {
        return result;
    }

    *temp_mcelsius = tmp105_raw_to_mcelsius(data);
    return RT_EOK;
}

rt_err_t tmp105_set_resolution(struct tmp105_device *dev, rt_uint8_t bits)
{
    rt_err_t result;
    rt_uint8_t config;
    rt_uint8_t res_bits;

    if (!tmp105_is_device_ready(dev))
    {
        return -RT_EINVAL;
    }

    if (bits < 9 || bits > 12)
    {
        return -RT_EINVAL;
    }

    res_bits = (rt_uint8_t)(bits - 9);

    result = tmp105_read_registers(dev, TMP105_REG_CONFIG, &config, 1);
    if (result != RT_EOK)
    {
        return result;
    }

    config &= ~(TMP105_CONF_RES_0 | TMP105_CONF_RES_1);
    config |= (res_bits << 5);

    return tmp105_write_registers(dev, TMP105_REG_CONFIG, &config, 1);
}

rt_err_t tmp105_read_config(struct tmp105_device *dev, rt_uint8_t *config)
{
    if (config == RT_NULL)
    {
        return -RT_EINVAL;
    }

    return tmp105_read_registers(dev, TMP105_REG_CONFIG, config, 1);
}
