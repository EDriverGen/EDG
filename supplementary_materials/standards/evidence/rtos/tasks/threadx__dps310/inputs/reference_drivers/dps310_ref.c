/*
 * SPDX-License-Identifier: MIT
 *
 * DPS310 Pressure/Temperature Sensor Driver for ThreadX
 */
#include "dps310_ref.h"
#include <string.h>


static int dps310_threadx_i2c_write(struct dps310_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int dps310_threadx_i2c_read(struct dps310_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int dps310_threadx_i2c_write_read(struct dps310_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define DPS310_I2C_WRITE(_bus, _addr, _data, _len) \
    dps310_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define DPS310_I2C_READ(_bus, _addr, _data, _len) \
    dps310_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define DPS310_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    dps310_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int dps310_read_reg(struct dps310_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return DPS310_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}

static int dps310_write_reg(struct dps310_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t buf[16];
    if (dev == NULL || dev->bus_context == NULL || len > 15) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1 + i] = data[i];
    return DPS310_I2C_WRITE(dev->bus_context, dev->addr, buf, len + 1);
}


static int32_t dps310_twos_complement(int32_t val, uint8_t bits)
{
    if (val & ((int32_t)1 << (bits - 1)))
        val -= ((int32_t)1 << bits);
    return val;
}

static void dps310_sleep_ms(uint32_t delay_ms)
{
    ULONG ticks = (ULONG)((delay_ms * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U);
    if (ticks == 0U) ticks = 1U;
    tx_thread_sleep(ticks);
}

static int dps310_wait_ready(struct dps310_device *dev, uint8_t mask, uint32_t timeout_ms)
{
    uint8_t meas_cfg;
    uint32_t elapsed = 0;
    int ret;

    while (elapsed < timeout_ms) {
        ret = dps310_read_reg(dev, DPS310_REG_MEAS_CFG, &meas_cfg, 1);
        if (ret != 0) return ret;
        if ((meas_cfg & mask) == mask) return 0;
        dps310_sleep_ms(10);
        elapsed += 10;
    }
    return -2;
}

static int dps310_read_raw_temperature(struct dps310_device *dev, int32_t *raw_tmp)
{
    uint8_t buf[3], cfg_val;
    int ret;

    if (dev == NULL || raw_tmp == NULL || !dev->coef_ready) return -1;

    cfg_val = dev->temp_cfg;
    ret = dps310_write_reg(dev, DPS310_REG_TMP_CFG, &cfg_val, 1);
    if (ret != 0) return ret;

    cfg_val = 0x00;
    ret = dps310_write_reg(dev, DPS310_REG_CFG_REG, &cfg_val, 1);
    if (ret != 0) return ret;

    cfg_val = DPS310_MEAS_TMP_SINGLE;
    ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;

    ret = dps310_wait_ready(dev, DPS310_MEAS_TMP_RDY, 100);
    if (ret != 0) return ret;

    ret = dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3);
    if (ret != 0) return ret;

    *raw_tmp = dps310_twos_complement(((int32_t)buf[0] << 16) |
                                      ((int32_t)buf[1] << 8)  |
                                      (int32_t)buf[2], 24);
    return 0;
}

static int dps310_read_raw_pressure(struct dps310_device *dev, int32_t *raw_psr)
{
    uint8_t buf[3], cfg_val;
    int ret;

    if (dev == NULL || raw_psr == NULL || !dev->coef_ready) return -1;

    cfg_val = 0x00;
    ret = dps310_write_reg(dev, DPS310_REG_PRS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;

    cfg_val = 0x00;
    ret = dps310_write_reg(dev, DPS310_REG_CFG_REG, &cfg_val, 1);
    if (ret != 0) return ret;

    cfg_val = DPS310_MEAS_PRS_SINGLE;
    ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;

    ret = dps310_wait_ready(dev, DPS310_MEAS_PRS_RDY, 100);
    if (ret != 0) return ret;

    ret = dps310_read_reg(dev, DPS310_REG_PSR_B2, buf, 3);
    if (ret != 0) return ret;

    *raw_psr = dps310_twos_complement(((int32_t)buf[0] << 16) |
                                      ((int32_t)buf[1] << 8)  |
                                      (int32_t)buf[2], 24);
    return 0;
}

int dps310_init(struct dps310_device *dev, void *bus_context, const struct dps310_i2c_ops *ops, uint16_t addr)
{
    uint8_t reset_cmd = DPS310_RESET_SOFT;

    if (dev == NULL) return -1;
    if (addr != DPS310_ADDR_LOW && addr != DPS310_ADDR_HIGH) return -1;
    if (bus_context == NULL || ops == NULL) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->bus_context  = bus_context;
    dev->ops = ops;
    dev->addr = addr;
    dev->temp_cfg = 0x00;

    if (dps310_write_reg(dev, DPS310_REG_RESET, &reset_cmd, 1) != 0) return -1;
    dps310_sleep_ms(40);
    return dps310_wait_ready(dev, DPS310_MEAS_SENSOR_RDY | DPS310_MEAS_COEF_RDY, 200);
}

int dps310_probe(struct dps310_device *dev)
{
    uint8_t id;
    int ret = dps310_read_reg(dev, DPS310_REG_PROD_ID, &id, 1);
    if (ret != 0) return ret;
    if ((id & DPS310_PROD_ID_MASK) != DPS310_PROD_ID_EXPECTED) return -3;
    return 0;
}

int dps310_read_calibration(struct dps310_device *dev)
{
    uint8_t coef[18];
    uint8_t coef_srce;
    int ret;
    if (dev == NULL) return -1;
    ret = dps310_wait_ready(dev, DPS310_MEAS_COEF_RDY, 200);
    if (ret != 0) return ret;
    ret = dps310_read_reg(dev, DPS310_REG_COEF_START, coef, 18);
    if (ret != 0) return ret;

    dev->c0  = dps310_twos_complement(((int32_t)coef[0] << 4) | (coef[1] >> 4), 12);
    dev->c1  = dps310_twos_complement(((int32_t)(coef[1] & 0x0F) << 8) | coef[2], 12);
    dev->c00 = dps310_twos_complement(((int32_t)coef[3] << 12) | ((int32_t)coef[4] << 4) | (coef[5] >> 4), 20);
    dev->c10 = dps310_twos_complement(((int32_t)(coef[5] & 0x0F) << 16) | ((int32_t)coef[6] << 8) | coef[7], 20);
    dev->c01 = dps310_twos_complement(((int32_t)coef[8] << 8) | coef[9], 16);
    dev->c11 = dps310_twos_complement(((int32_t)coef[10] << 8) | coef[11], 16);
    dev->c20 = dps310_twos_complement(((int32_t)coef[12] << 8) | coef[13], 16);
    dev->c21 = dps310_twos_complement(((int32_t)coef[14] << 8) | coef[15], 16);
    dev->c30 = dps310_twos_complement(((int32_t)coef[16] << 8) | coef[17], 16);

    ret = dps310_read_reg(dev, DPS310_REG_COEF_SRCE, &coef_srce, 1);
    if (ret != 0) return ret;
    dev->temp_cfg = (coef_srce & 0x80U) ? 0x80U : 0x00U;
    dev->coef_ready = 1;
    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa)
{
    int32_t raw_psr, raw_tmp;
    int ret;
    if (dev == NULL || pressure_pa == NULL) return -1;
    if (!dev->coef_ready) return -1;
    ret = dps310_read_raw_temperature(dev, &raw_tmp);
    if (ret != 0) return ret;
    ret = dps310_read_raw_pressure(dev, &raw_psr);
    if (ret != 0) return ret;

    /* Compensate using calibration coefficients (scaled integer) */
    float psr_sc = (float)raw_psr / DPS310_SCALE_FACTOR_1;
    float tmp_sc = (float)raw_tmp / DPS310_SCALE_FACTOR_1;
    float pcomp = (float)dev->c00 +
                  psr_sc * ((float)dev->c10 + psr_sc * ((float)dev->c20 + psr_sc * (float)dev->c30)) +
                  tmp_sc * (float)dev->c01 +
                  tmp_sc * psr_sc * ((float)dev->c11 + psr_sc * (float)dev->c21);
    *pressure_pa = (int32_t)pcomp;
    return 0;
}

int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_mcelsius)
{
    int32_t raw_tmp;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    if (!dev->coef_ready) return -1;
    ret = dps310_read_raw_temperature(dev, &raw_tmp);
    if (ret != 0) return ret;
    float tmp_sc = (float)raw_tmp / DPS310_SCALE_FACTOR_1;
    float temp = (float)dev->c0 * 0.5f + (float)dev->c1 * tmp_sc;
    *temp_mcelsius = (int32_t)(temp * 1000.0f);
    return 0;
}
