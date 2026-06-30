/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add LSM303DLHC driver with standard structure
 */
#include <lsm303dlhc_ref.h>

/* ---- 内部辅助 ---- */

static rt_bool_t lsm303dlhc_is_device_ready(struct lsm303dlhc_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

/*
 * 向指定 I2C 地址读取一个寄存器。
 * LSM303DLHC 加速度计和磁力计有不同的从机地址，
 * 所以 addr 由调用者传入。
 */
static rt_err_t lsm303dlhc_read_reg(struct lsm303dlhc_device *dev,
                                     rt_uint8_t                slave_addr,
                                     rt_uint8_t                reg,
                                     rt_uint8_t               *value)
{
    struct rt_i2c_msg msgs[2];

    if (!lsm303dlhc_is_device_ready(dev) || (value == RT_NULL))
    {
        return -RT_EINVAL;
    }

    msgs[0].addr  = slave_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = slave_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len   = 1;
    msgs[1].buf   = value;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

static rt_err_t lsm303dlhc_write_reg(struct lsm303dlhc_device *dev,
                                      rt_uint8_t                slave_addr,
                                      rt_uint8_t                reg,
                                      rt_uint8_t                value)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[2];

    if (!lsm303dlhc_is_device_ready(dev))
    {
        return -RT_EINVAL;
    }

    frame[0] = reg;
    frame[1] = value;

    msg.addr  = slave_addr;
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
 * 连续读取多个寄存器（需要设置 MSB 自动递增位）。
 * 对于加速度计，bit 7 = 1 表示地址自增。
 */
static rt_err_t lsm303dlhc_read_multi(struct lsm303dlhc_device *dev,
                                       rt_uint8_t                slave_addr,
                                       rt_uint8_t                start_reg,
                                       rt_uint8_t               *buffer,
                                       rt_size_t                 size)
{
    struct rt_i2c_msg msgs[2];
    rt_uint8_t reg;

    if (!lsm303dlhc_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0))
    {
        return -RT_EINVAL;
    }

    /* 加速度计使用 bit7=1 标志自动地址递增 */
    reg = start_reg | 0x80;

    msgs[0].addr  = slave_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg;

    msgs[1].addr  = slave_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len   = size;
    msgs[1].buf   = buffer;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
    {
        return -RT_EIO;
    }

    return RT_EOK;
}

/* ---- 公开接口 ---- */

rt_err_t lsm303dlhc_init(struct lsm303dlhc_device *dev,
                          const char               *bus_name)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : LSM303DLHC_DEFAULT_BUS_NAME;

    dev->bus = (struct rt_i2c_bus_device *)rt_device_find(target_bus_name);
    if (dev->bus == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    dev->bus_name = target_bus_name;
    return RT_EOK;
}

rt_err_t lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
    rt_err_t result;
    rt_uint8_t ira, irb, irc;

    result = lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR,
                                  LSM303DLHC_IRA_REG_M, &ira);
    if (result != RT_EOK)
        return result;

    result = lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR,
                                  LSM303DLHC_IRB_REG_M, &irb);
    if (result != RT_EOK)
        return result;

    result = lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR,
                                  LSM303DLHC_IRC_REG_M, &irc);
    if (result != RT_EOK)
        return result;

    if ((ira != LSM303DLHC_IRA_VALUE) ||
        (irb != LSM303DLHC_IRB_VALUE) ||
        (irc != LSM303DLHC_IRC_VALUE))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t lsm303dlhc_accel_start(struct lsm303dlhc_device *dev)
{
    rt_err_t result;

    /* CTRL_REG1_A: 50Hz ODR, 使能 XYZ 三轴 */
    result = lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR,
                                   LSM303DLHC_CTRL_REG1_A,
                                   LSM303DLHC_ODR_50HZ | LSM303DLHC_AXES_ENABLE);
    if (result != RT_EOK)
        return result;

    /* CTRL_REG4_A: ±2g, 高分辨率 */
    result = lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR,
                                   LSM303DLHC_CTRL_REG4_A,
                                   LSM303DLHC_FS_2G | LSM303DLHC_HR_BIT);
    return result;
}

rt_err_t lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev,
                                    struct lsm303dlhc_xyz    *accel)
{
    rt_err_t result;
    rt_uint8_t data[6];

    if (accel == RT_NULL)
    {
        return -RT_EINVAL;
    }

    result = lsm303dlhc_read_multi(dev, LSM303DLHC_ACCEL_ADDR,
                                    LSM303DLHC_OUT_X_L_A, data, 6);
    if (result != RT_EOK)
    {
        return result;
    }

    /*
     * 数据格式：16 位有符号，左对齐。
     * 在 ±2g 高分辨率模式下，有效位数为 12 位。
     * 原始值需要右移 4 位。
     */
    accel->x = ((rt_int16_t)((rt_uint16_t)data[1] << 8 | data[0])) >> 4;
    accel->y = ((rt_int16_t)((rt_uint16_t)data[3] << 8 | data[2])) >> 4;
    accel->z = ((rt_int16_t)((rt_uint16_t)data[5] << 8 | data[4])) >> 4;

    return RT_EOK;
}

rt_err_t lsm303dlhc_mag_start(struct lsm303dlhc_device *dev)
{
    rt_err_t result;

    /* CRA_REG_M: 15Hz 数据速率 */
    result = lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR,
                                   LSM303DLHC_CRA_REG_M,
                                   LSM303DLHC_MAG_ODR_15HZ);
    if (result != RT_EOK)
        return result;

    /* CRB_REG_M: ±1.3 gauss */
    result = lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR,
                                   LSM303DLHC_CRB_REG_M,
                                   LSM303DLHC_MAG_GAIN_1_3);
    if (result != RT_EOK)
        return result;

    /* MR_REG_M: 连续模式 */
    result = lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR,
                                   LSM303DLHC_MR_REG_M,
                                   LSM303DLHC_MAG_CONTINUOUS);
    return result;
}

rt_err_t lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev,
                                  struct lsm303dlhc_xyz    *mag)
{
    rt_err_t result;
    rt_uint8_t data[6];

    if (mag == RT_NULL)
    {
        return -RT_EINVAL;
    }

    /*
     * 磁力计输出顺序比较特殊：X_H, X_L, Z_H, Z_L, Y_H, Y_L。
     * 从 OUT_X_H_M (0x03) 开始连续读 6 字节。
     */
    result = lsm303dlhc_read_multi(dev, LSM303DLHC_MAG_ADDR,
                                    LSM303DLHC_OUT_X_H_M, data, 6);
    if (result != RT_EOK)
    {
        return result;
    }

    /* 磁力计为大端序 */
    mag->x = (rt_int16_t)((rt_uint16_t)data[0] << 8 | data[1]);
    mag->z = (rt_int16_t)((rt_uint16_t)data[2] << 8 | data[3]);
    mag->y = (rt_int16_t)((rt_uint16_t)data[4] << 8 | data[5]);

    return RT_EOK;
}
