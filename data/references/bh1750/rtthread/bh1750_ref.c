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
 * 下面三个命令不是“寄存器地址”，而是 BH1750 识别的功能命令。
 * 它和很多有寄存器表的传感器不一样，发一个字节命令就能控制工作模式。
 */
#define BH1750_CMD_POWER_DOWN       0x00
#define BH1750_CMD_POWER_ON         0x01
#define BH1750_CMD_RESET            0x07

/*
 * 向 BH1750 发送单字节命令。
 *
 * 为什么这里使用 rt_i2c_msg？
 * 因为 RT-Thread 的 I2C 框架就是通过这个结构体描述一次读/写事务的：
 * - addr  : 从机地址
 * - flags : 读还是写
 * - len   : 数据长度
 * - buf   : 数据缓冲区
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
 * 从 BH1750 读取指定长度的数据。
 *
 * 当前驱动只会读取 2 个字节的测量结果，但单独封装一个读取函数后，
 * 代码层次会更清晰，也便于后面扩展。
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
 * BH1750 不同测量模式需要的等待时间不同。
 *
 * - 高分辨率模式：典型约 120 ms，最大约 180 ms
 * - 低分辨率模式：典型约 16 ms，最大约 24 ms
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
 * 让 BH1750 进入上电状态。
 *
 * 注意：
 * BH1750 的 Reset 命令只有在上电状态下才有效，
 * 所以正式测量前通常都要先发 Power On。
 */
static rt_err_t bh1750_power_on(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
}

/*
 * 让 BH1750 进入掉电状态。
 *
 * 对单次测量模式来说，芯片测完后会自动回到掉电状态；
 * 这里单独保留掉电函数，主要是为了探测和后续扩展时使用。
 */
static rt_err_t bh1750_power_down(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

/*
 * 复位 BH1750 的测量结果寄存器。
 *
 * 这里的“复位”不是 MCU 的硬复位，而是把 BH1750 内部的数据寄存器清零。
 */
static rt_err_t bh1750_reset_data_register(struct bh1750_device *dev)
{
    return bh1750_write_cmd(dev, BH1750_CMD_RESET);
}

/*
 * 发送测量模式命令，让 BH1750 开始一次新的光照采样。
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
     * 探测的思路很朴素：
     * 1. 发一条合法的 Power On 命令
     * 2. 如果设备地址存在，就会返回 ACK
     * 3. 探测成功后再发 Power Down，把芯片恢复到比较干净的状态
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
     * 按照数据手册推荐流程完成一次测量：
     * 1. 上电
     * 2. 复位数据寄存器
     * 3. 发送测量模式命令
     * 4. 等待测量完成
     * 5. 读取 2 字节结果
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
     * BH1750 输出 16 位结果，高字节在前、低字节在后。
     * 因此组合时要先左移高字节，再拼低字节。
     */
    *raw = ((rt_uint16_t)data[0] << 8) | data[1];

    return RT_EOK;
}

rt_uint32_t bh1750_raw_to_lux_x100(rt_uint16_t raw)
{
    /*
     * 数据手册给出的默认换算关系：
     * lux = raw / 1.2
     *
     * 为了避免在 MCU 上直接使用浮点，这里改写成整数形式：
     * lux_x100 = raw * 100 / 1.2
     *          = raw * 1000 / 12
     *
     * 这样返回值仍然保留两位小数，只是单位变成了 lux * 100。
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
