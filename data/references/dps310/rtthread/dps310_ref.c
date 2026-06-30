/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add DPS310 driver with standard structure
 */
#include <dps310_ref.h>

/* ---- 内部辅助 ---- */

static rt_bool_t dps310_is_device_ready(struct dps310_device *dev)
{
    return (dev != RT_NULL) && (dev->bus != RT_NULL);
}

static rt_err_t dps310_read_registers(struct dps310_device *dev,
                                      rt_uint8_t            reg,
                                      rt_uint8_t           *buffer,
                                      rt_size_t             size)
{
    struct rt_i2c_msg msgs[2];

    if (!dps310_is_device_ready(dev) || (buffer == RT_NULL) || (size == 0))
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

static rt_err_t dps310_write_register(struct dps310_device *dev,
                                      rt_uint8_t            reg,
                                      rt_uint8_t            value)
{
    struct rt_i2c_msg msg;
    rt_uint8_t frame[2];

    if (!dps310_is_device_ready(dev))
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
 * 等待传感器就绪。
 */
static rt_err_t dps310_wait_ready(struct dps310_device *dev,
                                  rt_uint8_t            mask,
                                  rt_uint32_t           timeout_ms)
{
    rt_uint32_t elapsed = 0;
    rt_uint8_t meas_cfg;
    rt_err_t result;

    while (elapsed < timeout_ms)
    {
        result = dps310_read_registers(dev, DPS310_REG_MEAS_CFG, &meas_cfg, 1);
        if (result != RT_EOK)
        {
            return result;
        }

        if (meas_cfg & mask)
        {
            return RT_EOK;
        }

        rt_thread_mdelay(10);
        elapsed += 10;
    }

    return -RT_ETIMEOUT;
}

/*
 * 把 24 位二进制补码转换为有符号 32 位整数。
 */
static rt_int32_t dps310_twos_complement_24(rt_uint32_t raw)
{
    if (raw & 0x800000)
    {
        return (rt_int32_t)(raw | 0xFF000000);
    }
    return (rt_int32_t)raw;
}

/*
 * 从校准系数寄存器中解析系数。
 * 系数 c0~c30 的位宽和位置来自数据手册 Section 8.11。
 */
static void dps310_parse_coefficients(const rt_uint8_t raw[18],
                                      struct dps310_calib_coeff *coeff)
{
    /* c0: 12 bit signed */
    coeff->c0 = ((rt_int16_t)(((rt_uint16_t)raw[0] << 4) |
                 ((rt_uint16_t)raw[1] >> 4)));
    if (coeff->c0 & 0x0800)
        coeff->c0 |= 0xFFFFF000;

    /* c1: 12 bit signed */
    coeff->c1 = ((rt_int16_t)((((rt_uint16_t)raw[1] & 0x0F) << 8) |
                 (rt_uint16_t)raw[2]));
    if (coeff->c1 & 0x0800)
        coeff->c1 |= 0xFFFFF000;

    /* c00: 20 bit signed */
    coeff->c00 = ((rt_int32_t)(((rt_uint32_t)raw[3] << 12) |
                  ((rt_uint32_t)raw[4] << 4) |
                  ((rt_uint32_t)raw[5] >> 4)));
    if (coeff->c00 & 0x80000)
        coeff->c00 |= 0xFFF00000;

    /* c10: 20 bit signed */
    coeff->c10 = ((rt_int32_t)((((rt_uint32_t)raw[5] & 0x0F) << 16) |
                  ((rt_uint32_t)raw[6] << 8) |
                  (rt_uint32_t)raw[7]));
    if (coeff->c10 & 0x80000)
        coeff->c10 |= 0xFFF00000;

    /* c01: 16 bit signed */
    coeff->c01 = (rt_int16_t)(((rt_uint16_t)raw[8] << 8) | (rt_uint16_t)raw[9]);

    /* c11: 16 bit signed */
    coeff->c11 = (rt_int16_t)(((rt_uint16_t)raw[10] << 8) | (rt_uint16_t)raw[11]);

    /* c20: 16 bit signed */
    coeff->c20 = (rt_int16_t)(((rt_uint16_t)raw[12] << 8) | (rt_uint16_t)raw[13]);

    /* c21: 16 bit signed */
    coeff->c21 = (rt_int16_t)(((rt_uint16_t)raw[14] << 8) | (rt_uint16_t)raw[15]);

    /* c30: 16 bit signed */
    coeff->c30 = (rt_int16_t)(((rt_uint16_t)raw[16] << 8) | (rt_uint16_t)raw[17]);
}

/* ---- 公开接口 ---- */

rt_err_t dps310_init(struct dps310_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr)
{
    const char *target_bus_name;

    if (dev == RT_NULL)
    {
        return -RT_EINVAL;
    }

    target_bus_name = (bus_name != RT_NULL) ? bus_name : DPS310_DEFAULT_BUS_NAME;
    if (addr == 0)
    {
        addr = DPS310_DEFAULT_ADDR;
    }

    dev->bus = (struct rt_i2c_bus_device *)rt_device_find(target_bus_name);
    if (dev->bus == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    dev->bus_name = target_bus_name;
    dev->addr     = addr;

    /* 默认使用单次、1 倍过采样，缩放因子 = 524288 */
    dev->kT = DPS310_SCALE_FACTOR_1;
    dev->kP = DPS310_SCALE_FACTOR_1;

    return RT_EOK;
}

rt_err_t dps310_probe(struct dps310_device *dev)
{
    rt_err_t result;
    rt_uint8_t id;

    result = dps310_read_registers(dev, DPS310_REG_PRODUCT_ID, &id, 1);
    if (result != RT_EOK)
    {
        return result;
    }

    if ((id & 0xF0) != DPS310_PRODUCT_ID)
    {
        return -RT_ERROR;
    }
    return RT_EOK;
}

rt_err_t dps310_reset(struct dps310_device *dev)
{
    rt_err_t result;

    result = dps310_write_register(dev, DPS310_REG_RESET, DPS310_RESET_SOFT);
    if (result != RT_EOK)
    {
        return result;
    }

    rt_thread_mdelay(40);

    return dps310_wait_ready(dev, DPS310_MEAS_CFG_SENSOR_RDY | DPS310_MEAS_CFG_COEF_RDY, 500);
}

rt_err_t dps310_read_calibration(struct dps310_device *dev)
{
    rt_err_t result;
    rt_uint8_t raw[18];
    rt_uint8_t coef_srce;

    if (!dps310_is_device_ready(dev))
    {
        return -RT_EINVAL;
    }

    result = dps310_read_registers(dev, DPS310_REG_COEF, raw, 18);
    if (result != RT_EOK)
    {
        return result;
    }

    dps310_parse_coefficients(raw, &dev->coeff);

    /* 读取温度系数来源（内部传感器或外部传感器） */
    result = dps310_read_registers(dev, DPS310_REG_COEF_SRCE, &coef_srce, 1);
    if (result != RT_EOK)
    {
        return result;
    }

    /*
     * 如果 TMP_COEF_SRCE bit 7 = 1，使用外部传感器，
     * 需要在 TMP_CFG 中也设置对应位。
     */
    if (coef_srce & 0x80)
    {
        rt_uint8_t tmp_cfg;
        result = dps310_read_registers(dev, DPS310_REG_TMP_CFG, &tmp_cfg, 1);
        if (result != RT_EOK)
            return result;
        tmp_cfg |= 0x80;
        result = dps310_write_register(dev, DPS310_REG_TMP_CFG, tmp_cfg);
        if (result != RT_EOK)
            return result;
    }

    return RT_EOK;
}

rt_err_t dps310_read_temperature(struct dps310_device *dev,
                                 rt_int32_t           *temp_c100)
{
    rt_err_t result;
    rt_uint8_t data[3];
    rt_int32_t raw_temp;
    double t_raw_sc;
    double t_comp;

    if (temp_c100 == RT_NULL)
    {
        return -RT_EINVAL;
    }

    /* 触发单次温度测量 */
    result = dps310_write_register(dev, DPS310_REG_MEAS_CFG, DPS310_MODE_TMP_SINGLE);
    if (result != RT_EOK)
    {
        return result;
    }

    /* 等待温度准备好 */
    result = dps310_wait_ready(dev, DPS310_MEAS_CFG_TMP_RDY, 200);
    if (result != RT_EOK)
    {
        return result;
    }

    result = dps310_read_registers(dev, DPS310_REG_TMP_B2, data, 3);
    if (result != RT_EOK)
    {
        return result;
    }

    raw_temp = dps310_twos_complement_24(
        ((rt_uint32_t)data[0] << 16) |
        ((rt_uint32_t)data[1] << 8)  |
        (rt_uint32_t)data[2]);

    /*
     * 补偿公式（数据手册 Section 4.9.1）：
     * T_raw_sc = T_raw / kT
     * T_comp = c0 * 0.5 + c1 * T_raw_sc
     */
    t_raw_sc = (double)raw_temp / (double)dev->kT;
    t_comp = (double)dev->coeff.c0 * 0.5 + (double)dev->coeff.c1 * t_raw_sc;

    *temp_c100 = (rt_int32_t)(t_comp * 100.0);
    return RT_EOK;
}

rt_err_t dps310_read_pressure(struct dps310_device *dev,
                              rt_int32_t           *pressure_pa100)
{
    rt_err_t result;
    rt_uint8_t data[3];
    rt_int32_t raw_prs;
    rt_int32_t temp_c100;
    double p_raw_sc, t_raw_sc;
    double p_comp;

    if (pressure_pa100 == RT_NULL)
    {
        return -RT_EINVAL;
    }

    /* 先做一次温度测量以获取补偿用 T_raw_sc */
    result = dps310_read_temperature(dev, &temp_c100);
    if (result != RT_EOK)
    {
        return result;
    }

    /* 触发单次气压测量 */
    result = dps310_write_register(dev, DPS310_REG_MEAS_CFG, DPS310_MODE_PRS_SINGLE);
    if (result != RT_EOK)
    {
        return result;
    }

    result = dps310_wait_ready(dev, DPS310_MEAS_CFG_PRS_RDY, 200);
    if (result != RT_EOK)
    {
        return result;
    }

    result = dps310_read_registers(dev, DPS310_REG_PSR_B2, data, 3);
    if (result != RT_EOK)
    {
        return result;
    }

    raw_prs = dps310_twos_complement_24(
        ((rt_uint32_t)data[0] << 16) |
        ((rt_uint32_t)data[1] << 8)  |
        (rt_uint32_t)data[2]);

    p_raw_sc = (double)raw_prs / (double)dev->kP;

    /* 利用最近一次温度原始值计算 t_raw_sc */
    {
        rt_uint8_t tdata[3];
        rt_int32_t raw_temp;

        result = dps310_read_registers(dev, DPS310_REG_TMP_B2, tdata, 3);
        if (result != RT_EOK)
        {
            return result;
        }
        raw_temp = dps310_twos_complement_24(
            ((rt_uint32_t)tdata[0] << 16) |
            ((rt_uint32_t)tdata[1] << 8)  |
            (rt_uint32_t)tdata[2]);
        t_raw_sc = (double)raw_temp / (double)dev->kT;
    }

    /*
     * 补偿公式（数据手册 Section 4.9.2）：
     * P_comp = c00 + p_raw_sc * (c10 + p_raw_sc * (c20 + p_raw_sc * c30))
     *        + t_raw_sc * c01 + t_raw_sc * p_raw_sc * (c11 + p_raw_sc * c21)
     */
    p_comp = (double)dev->coeff.c00 +
             p_raw_sc * ((double)dev->coeff.c10 +
                         p_raw_sc * ((double)dev->coeff.c20 +
                                     p_raw_sc * (double)dev->coeff.c30)) +
             t_raw_sc * (double)dev->coeff.c01 +
             t_raw_sc * p_raw_sc * ((double)dev->coeff.c11 +
                                    p_raw_sc * (double)dev->coeff.c21);

    *pressure_pa100 = (rt_int32_t)(p_comp * 100.0);
    return RT_EOK;
}
