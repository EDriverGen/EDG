/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin          add LM75A driver with standard structure
 */
#include <lm75a.h>

/*
 * 判断设备对象是否处于可访问状态。
 * 很多公开接口都会先做这一步检查，避免上层在空指针或未初始化对象上继续操作。
 */
static rt_bool_t lm75a_is_device_ready(struct lm75a_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

/*
 * 通过“先写寄存器指针，再读数据”的方式读取寄存器内容。
 *
 * LM75A 的寄存器访问流程很典型：
 * 1. 先向芯片写 1 字节寄存器号，告诉它“下一次访问哪个寄存器”
 * 2. 再发起读操作，把该寄存器的数据取回来
 *
 * 在 RT-Thread 的 I2C 框架里，这种流程可以用两条 rt_i2c_msg 组成一次传输。
 */
static rt_err_t lm75a_read_registers(struct lm75a_device *dev,
                                     rt_uint8_t           reg,
                                     rt_uint8_t          *buffer,
                                     rt_size_t            size)
{
    struct rt_i2c_msg msgs[2];

    if (!lm75a_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0))
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
 *
 * LM75A 的写寄存器格式同样很简单：
 * 第 1 个字节是寄存器号，后面紧跟要写入的数据。
 */
static rt_err_t lm75a_write_registers(struct lm75a_device *dev,
                                      rt_uint8_t           reg,
                                      const rt_uint8_t    *buffer,
                                      rt_size_t            size)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[3];

    if (!lm75a_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0) || (size > 2))
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
 * 把温度寄存器/阈值寄存器的两个字节数据，解析成“0.125 C 为单位”的有符号原始值。
 *
 * 数据手册说明：
 * - 这类寄存器总共是 16 位
 * - 真正有效的是高 11 位
 * - 数据格式是二进制补码
 * - 低 5 位是无效位
 *
 * 因此最直接的做法就是：
 * 1. 先把两个字节拼成 16 位数
 * 2. 再做一次带符号的右移 5 位
 *
 * 这样得到的结果就变成了“每 1 个数值单位 = 0.125 C”。
 */
static rt_int16_t lm75a_unpack_temp_register(const rt_uint8_t data[2])
{
    rt_uint16_t reg_value;

    reg_value = ((rt_uint16_t)data[0] << 8) | data[1];
    return ((rt_int16_t)reg_value) >> 5;
}

/*
 * 把“0.125 C 为单位”的有符号原始值重新打包成寄存器格式。
 *
 * 因为 LM75A 的有效数据在 bit15 ~ bit5，所以这里只需要把原始值左移 5 位。
 */
static void lm75a_pack_temp_register(rt_int16_t raw, rt_uint8_t data[2])
{
    rt_uint16_t reg_value;

    reg_value = ((rt_uint16_t)((rt_int16_t)raw)) << 5;
    data[0] = (rt_uint8_t)(reg_value >> 8);
    data[1] = (rt_uint8_t)(reg_value & 0xFF);
}

/*
 * 判断一个原始温度值是否在 LM75A 支持的范围内。
 *
 * -55 C ~ 125 C
 * 换算到 0.125 C 单位后，就是 -440 ~ 1000。
 */
static rt_bool_t lm75a_is_valid_raw_temp(rt_int16_t raw)
{
    return (raw >= (LM75A_TEMP_MC_MIN / LM75A_TEMP_STEP_MC)) &&
           (raw <= (LM75A_TEMP_MC_MAX / LM75A_TEMP_STEP_MC));
}

/*
 * 把毫摄氏度转换成 LM75A 可写入的原始格式。
 *
 * LM75A 的最小步进是 125 mC，所以如果上层给的值不是 125 mC 的整数倍，
 * 这里会自动四舍五入到最接近的可表示值。
 */
static rt_err_t lm75a_mcelsius_to_raw(rt_int32_t temp_mcelsius, rt_int16_t *raw)
{
    rt_uint32_t abs_temp_mcelsius;
    rt_int16_t converted_raw;

    if (raw == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if ((temp_mcelsius < LM75A_TEMP_MC_MIN) || (temp_mcelsius > LM75A_TEMP_MC_MAX))
    {
        return -RT_EINVAL;
    }

    abs_temp_mcelsius = (temp_mcelsius < 0) ?
                        (rt_uint32_t)(-temp_mcelsius) :
                        (rt_uint32_t)temp_mcelsius;

    converted_raw = (rt_int16_t)((abs_temp_mcelsius + (LM75A_TEMP_STEP_MC / 2)) /
                                 LM75A_TEMP_STEP_MC);

    if (temp_mcelsius < 0)
    {
        converted_raw = (rt_int16_t)(-converted_raw);
    }

    if (!lm75a_is_valid_raw_temp(converted_raw))
    {
        return -RT_EINVAL;
    }

    *raw = converted_raw;
    return RT_EOK;
}

/*
 * 读取某个 16 位温度格式寄存器。
 * 这个辅助函数被 Temperature、T_HYST、T_OS 三个寄存器共用。
 */
static rt_err_t lm75a_read_temp_register_raw(struct lm75a_device *dev,
                                             rt_uint8_t           reg,
                                             rt_int16_t          *raw)
{
    rt_err_t result;
    rt_uint8_t data[2];

    if (raw == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = lm75a_read_registers(dev, reg, data, sizeof(data));
    if (result != RT_EOK)
    {
        return result;
    }

    *raw = lm75a_unpack_temp_register(data);
    return RT_EOK;
}

/*
 * 向某个 16 位温度格式寄存器写入数据。
 * 这个辅助函数被 T_HYST 和 T_OS 两个阈值寄存器共用。
 */
static rt_err_t lm75a_write_temp_register_raw(struct lm75a_device *dev,
                                              rt_uint8_t           reg,
                                              rt_int16_t           raw)
{
    rt_uint8_t data[2];

    if (!lm75a_is_valid_raw_temp(raw))
    {
        return -RT_EINVAL;
    }

    lm75a_pack_temp_register(raw, data);
    return lm75a_write_registers(dev, reg, data, sizeof(data));
}

rt_bool_t lm75a_is_valid_address(rt_uint8_t addr)
{
    return (addr >= LM75A_ADDR_MIN) && (addr <= LM75A_ADDR_MAX);
}

rt_err_t lm75a_init(struct lm75a_device *dev,
                    const char          *bus_name,
                    rt_uint8_t           addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (!lm75a_is_valid_address(addr))
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : LM75A_DEFAULT_BUS_NAME;

    dev->bus = rt_i2c_bus_device_find(target_bus_name);
    if (dev->bus == RT_NULL)
    {
        return -RT_ERROR;
    }

    dev->bus_name = target_bus_name;
    dev->addr = addr;

    return RT_EOK;
}

rt_err_t lm75a_probe(struct lm75a_device *dev)
{
    rt_uint8_t config;

    /*
     * 对 LM75A 来说，读取配置寄存器是一种很稳妥的探测方式：
     * - 不会像“乱写数据”那样改变工作状态
     * - 只要能收到 ACK 并读回 1 个字节，就说明器件大概率在线
     */
    return lm75a_read_config(dev, &config);
}

rt_err_t lm75a_read_config(struct lm75a_device *dev, rt_uint8_t *config)
{
    if (config == RT_NULL)
    {
        return -RT_EINVAL;
    }

    return lm75a_read_registers(dev, LM75A_REG_CONF, config, 1);
}

rt_err_t lm75a_write_config(struct lm75a_device *dev, rt_uint8_t config)
{
    return lm75a_write_registers(dev, LM75A_REG_CONF, &config, 1);
}

rt_err_t lm75a_set_shutdown(struct lm75a_device *dev, rt_bool_t enable)
{
    rt_err_t result;
    rt_uint8_t config;

    result = lm75a_read_config(dev, &config);
    if (result != RT_EOK)
    {
        return result;
    }

    if (enable)
    {
        config |= LM75A_CONF_SHUTDOWN;
    }
    else
    {
        config &= (rt_uint8_t)(~LM75A_CONF_SHUTDOWN);
    }

    return lm75a_write_config(dev, config);
}

rt_err_t lm75a_read_raw(struct lm75a_device *dev, rt_int16_t *raw)
{
    return lm75a_read_temp_register_raw(dev, LM75A_REG_TEMP, raw);
}

rt_int32_t lm75a_raw_to_mcelsius(rt_int16_t raw)
{
    /*
     * 每个原始单位对应 0.125 C。
     * 0.125 C = 125 mC
     * 所以直接乘以 125 就能得到毫摄氏度。
     */
    return (rt_int32_t)raw * LM75A_TEMP_STEP_MC;
}

rt_err_t lm75a_read_temp_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius)
{
    rt_err_t result;
    rt_int16_t raw;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = lm75a_read_raw(dev, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    *temp_mcelsius = lm75a_raw_to_mcelsius(raw);
    return RT_EOK;
}

rt_err_t lm75a_read_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius)
{
    rt_err_t result;
    rt_int16_t raw;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = lm75a_read_temp_register_raw(dev, LM75A_REG_THYST, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    *temp_mcelsius = lm75a_raw_to_mcelsius(raw);
    return RT_EOK;
}

rt_err_t lm75a_write_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius)
{
    rt_err_t result;
    rt_int16_t raw;

    result = lm75a_mcelsius_to_raw(temp_mcelsius, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    return lm75a_write_temp_register_raw(dev, LM75A_REG_THYST, raw);
}

rt_err_t lm75a_read_tos_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius)
{
    rt_err_t result;
    rt_int16_t raw;

    if (temp_mcelsius == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = lm75a_read_temp_register_raw(dev, LM75A_REG_TOS, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    *temp_mcelsius = lm75a_raw_to_mcelsius(raw);
    return RT_EOK;
}

rt_err_t lm75a_write_tos_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius)
{
    rt_err_t result;
    rt_int16_t raw;

    result = lm75a_mcelsius_to_raw(temp_mcelsius, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    return lm75a_write_temp_register_raw(dev, LM75A_REG_TOS, raw);
}
