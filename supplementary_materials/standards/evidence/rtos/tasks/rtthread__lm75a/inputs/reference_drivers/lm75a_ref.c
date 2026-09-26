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
 * Check whether the device object is accessible.
 * Many public APIs check this first to prevent operations on null or uninitialized objects.
 */
static rt_bool_t lm75a_is_device_ready(struct lm75a_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

/*
 * Read register contents by writing the register pointer, then reading data.
 *
 * LM75A register access follows this sequence:
 * 1. Write a 1-byte register number to select the next register
 * 2. Issue a read to retrieve that register's data
 *
 * RT-Thread's I2C framework expresses this as one transfer containing two rt_i2c_msg entries.
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
 * Write 1~2 data bytes to the specified register.
 *
 * LM75A register writes use the following format:
 * The first byte is the register number, followed by the data to write.
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
 * Decode two temperature/threshold register bytes as a signed raw value in units of 0.125 C.
 *
 * The datasheet specifies:
 * - Each such register contains 16 bits
 * - The upper 11 bits carry valid data
 * - Data use two's complement
 * - The low 5 bits are unused
 *
 * Decode as follows:
 * 1. Combine the two bytes into a 16-bit value
 * 2. Perform a signed right shift by 5 bits
 *
 * Each resulting count represents 0.125 C.
 */
static rt_int16_t lm75a_unpack_temp_register(const rt_uint8_t data[2])
{
    rt_uint16_t reg_value;

    reg_value = ((rt_uint16_t)data[0] << 8) | data[1];
    return ((rt_int16_t)reg_value) >> 5;
}

/*
 * Repack a signed raw value in units of 0.125 C into register format.
 *
 * LM75A data occupy bits15~5, so shift the raw value left by 5 bits.
 */
static void lm75a_pack_temp_register(rt_int16_t raw, rt_uint8_t data[2])
{
    rt_uint16_t reg_value;

    reg_value = ((rt_uint16_t)((rt_int16_t)raw)) << 5;
    data[0] = (rt_uint8_t)(reg_value >> 8);
    data[1] = (rt_uint8_t)(reg_value & 0xFF);
}

/*
 * Check whether a raw temperature is within the LM75A supported range.
 *
 * -55 C ~ 125 C
 * In 0.125 C units, the range is -440~1000.
 */
static rt_bool_t lm75a_is_valid_raw_temp(rt_int16_t raw)
{
    return (raw >= (LM75A_TEMP_MC_MIN / LM75A_TEMP_STEP_MC)) &&
           (raw <= (LM75A_TEMP_MC_MAX / LM75A_TEMP_STEP_MC));
}

/*
 * Convert millidegrees Celsius to the raw format accepted by LM75A.
 *
 * LM75A has a minimum step of 125 mC; values that are not multiples of 125 mC
 * are rounded to the nearest representable value.
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
 * Read a 16-bit temperature-format register.
 * This helper is shared by Temperature, T_HYST, and T_OS.
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
 * Write a 16-bit temperature-format register.
 * This helper is shared by the T_HYST and T_OS threshold registers.
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
     * Reading the configuration register is a reliable LM75A probe:
     * - It leaves the operating state unchanged
     * - Receiving an ACK and reading one byte indicate the device is likely online
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
     * Each raw count represents 0.125 C.
     * 0.125 C = 125 mC
     * Multiply by 125 to obtain millidegrees Celsius.
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
