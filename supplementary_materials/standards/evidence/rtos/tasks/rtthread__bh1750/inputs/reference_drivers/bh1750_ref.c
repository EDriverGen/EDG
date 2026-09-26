/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin        refactor BH1750 driver to standard structure
 */
#include <bh1750.h>

/*
 * The following three values are BH1750 function commands, not register addresses.
 * A single command byte controls the operating mode, unlike sensors with register maps.
 */
#define BH1750_CMD_POWER_DOWN       0x00
#define BH1750_CMD_POWER_ON         0x01
#define BH1750_CMD_RESET            0x07

/*
 * Send a single-byte command to BH1750.
 *
 * Why use rt_i2c_msg here?
 * RT-Thread's I2C framework describes each read/write transaction with this structure:
 * - addr  : Slave address
 * - flags : Read or write
 * - len   : Data length
 * - buf   : Data buffer
 */
static rt_err_t bh1750_write_cmd(struct bh1750_device *dev, rt_uint8_t cmd)
{
    struct rt_i2c_msg msg;

    if ((dev == RT_NULL) || (dev->bus == RT_NULL))
    {
        return -RT_EINVAL;
    }

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.len   = 1;
    msg.buf   = &cmd;

    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/*
 * Read the specified number of bytes from BH1750.
 *
 * This driver reads only a 2-byte measurement result; a separate read helper
 * keeps the code organized and supports future extensions.
 */
static rt_err_t bh1750_read_bytes(struct bh1750_device *dev,
                                  rt_uint8_t          *buffer,
                                  rt_size_t            size)
{
    struct rt_i2c_msg msg;

    if ((dev == RT_NULL) || (dev->bus == RT_NULL) || (buffer == RT_NULL) || (size == 0))
    {
        return -RT_EINVAL;
    }

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_RD;
    msg.len   = size;
    msg.buf   = buffer;

    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/*
 * BH1750 measurement modes require different waiting times.
 *
 * - High-resolution mode: typically about 120 ms, maximum about 180 ms
 * - Low-resolution mode: typically about 16 ms, maximum about 24 ms
 *
 */
static rt_int32_t bh1750_get_wait_time_ms(rt_uint8_t mode)
{
    switch (mode)
    {
    case BH1750_CONT_H_RES_MODE:
    case BH1750_CONT_H_RES_MODE2:
    case BH1750_ONE_H_RES_MODE:
    case BH1750_ONE_H_RES_MODE2:
        return 180;

    case BH1750_CONT_L_RES_MODE:
    case BH1750_ONE_L_RES_MODE:
        return 24;

    default:
        return 180;
    }
}

/*
 * Put BH1750 into the power-on state.
 *
 * Note:
 * The BH1750 Reset command is valid only in the power-on state,
 * so Power On is normally sent before measurement.
 */
static rt_err_t bh1750_power_on(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
}

/*
 * Put BH1750 into the power-down state.
 *
 * In one-shot mode, the chip automatically returns to power-down after measuring;
 * this separate power-down function is retained for probing and future extensions.
 */
static rt_err_t bh1750_power_down(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

/*
 * Reset the BH1750 measurement-data register.
 *
 * This clears the BH1750 internal data register; it is not an MCU hardware reset.
 */
static rt_err_t bh1750_reset_data_register(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_RESET);
}

/*
 * Send a measurement-mode command to start a new illuminance sample.
 */
static rt_err_t bh1750_start_measurement(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, dev->mode);
}

rt_bool_t bh1750_is_valid_address(rt_uint8_t addr)
{
    return (addr == BH1750_ADDR_LOW) || (addr == BH1750_ADDR_HIGH);
}

rt_bool_t bh1750_is_valid_mode(rt_uint8_t mode)
{
    switch (mode)
    {
    case BH1750_CONT_H_RES_MODE:
    case BH1750_CONT_H_RES_MODE2:
    case BH1750_CONT_L_RES_MODE:
    case BH1750_ONE_H_RES_MODE:
    case BH1750_ONE_H_RES_MODE2:
    case BH1750_ONE_L_RES_MODE:
        return RT_TRUE;

    default:
        return RT_FALSE;
    }
}

const char *bh1750_mode_to_string(rt_uint8_t mode)
{
    switch (mode)
    {
    case BH1750_CONT_H_RES_MODE:
        return "continuous high resolution";
    case BH1750_CONT_H_RES_MODE2:
        return "continuous high resolution 2";
    case BH1750_CONT_L_RES_MODE:
        return "continuous low resolution";
    case BH1750_ONE_H_RES_MODE:
        return "one-time high resolution";
    case BH1750_ONE_H_RES_MODE2:
        return "one-time high resolution 2";
    case BH1750_ONE_L_RES_MODE:
        return "one-time low resolution";
    default:
        return "unknown mode";
    }
}

rt_err_t bh1750_init(struct bh1750_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : BH1750_DEFAULT_BUS_NAME;
    if (!bh1750_is_valid_address(addr))
    {
        return -RT_EINVAL;
    }

    dev->bus = rt_i2c_bus_device_find(target_bus_name);
    if (dev->bus == RT_NULL)
    {
        return -RT_ERROR;
    }

    dev->bus_name = target_bus_name;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;

    return RT_EOK;
}

rt_err_t bh1750_set_mode(struct bh1750_device *dev, rt_uint8_t mode)
{
    if ((dev == RT_NULL) || !bh1750_is_valid_mode(mode))
    {
        return -RT_EINVAL;
    }

    dev->mode = mode;
    return RT_EOK;
}

rt_err_t bh1750_probe(struct bh1750_device *dev)
{
    rt_err_t result;

    if ((dev == RT_NULL) || (dev->bus == RT_NULL))
    {
        return -RT_EINVAL;
    }

    /*
     * Probe the device as follows:
     * 1. Send a valid Power On command
     * 2. An ACK indicates a responding device at the address
     * 3. After a successful probe, send Power Down to restore the chip's state
     */
    result = bh1750_power_on(dev);
    if (result != RT_EOK)
    {
        return result;
    }

    return bh1750_power_down(dev);
}

rt_err_t bh1750_read_raw(struct bh1750_device *dev, rt_uint16_t *raw)
{
    rt_err_t result;
    rt_uint8_t data[2];

    if ((dev == RT_NULL) || (raw == RT_NULL) || (dev->bus == RT_NULL))
    {
        return -RT_EINVAL;
    }

    if (!bh1750_is_valid_mode(dev->mode))
    {
        return -RT_EINVAL;
    }

    /*
     * Perform a measurement using the datasheet's recommended sequence:
     * 1. Power on
     * 2. Reset the data register
     * 3. Send the measurement-mode command
     * 4. Wait for measurement completion
     * 5. Read the 2-byte result
     */
    result = bh1750_power_on(dev);
    if (result != RT_EOK)
    {
        return result;
    }

    result = bh1750_reset_data_register(dev);
    if (result != RT_EOK)
    {
        return result;
    }

    result = bh1750_start_measurement(dev);
    if (result != RT_EOK)
    {
        return result;
    }

    rt_thread_mdelay(bh1750_get_wait_time_ms(dev->mode));

    result = bh1750_read_bytes(dev, data, sizeof(data));
    if (result != RT_EOK)
    {
        return result;
    }

    /*
     * BH1750 returns a 16-bit result with the high byte first and low byte second.
     * Shift the high byte left, then combine it with the low byte.
     */
    *raw = ((rt_uint16_t)data[0] << 8) | data[1];

    return RT_EOK;
}

rt_uint32_t bh1750_raw_to_lux_x100(rt_uint16_t raw)
{
    /*
     * The datasheet gives the default conversion:
     * lux = raw / 1.2
     *
     * To avoid floating-point operations on the MCU, use the integer form:
     * lux_x100 = raw * 100 / 1.2
     *          = raw * 1000 / 12
     *
     * This preserves two decimal places, with the result expressed as lux * 100.
     */
    return ((rt_uint32_t)raw * 1000U) / 12U;
}

rt_err_t bh1750_read_lux_x100(struct bh1750_device *dev, rt_uint32_t *lux_x100)
{
    rt_uint16_t raw;
    rt_err_t result;

    if ((dev == RT_NULL) || (lux_x100 == RT_NULL))
    {
        return -RT_EINVAL;
    }

    result = bh1750_read_raw(dev, &raw);
    if (result != RT_EOK)
    {
        return result;
    }

    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return RT_EOK;
}
