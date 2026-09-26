/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Pressure/Temperature Sensor Driver for ChibiOS
 */
#include "dps310_ref.h"

static int dps310_read_reg(struct dps310_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL || buf == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   &reg, 1, buf, len, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

static int dps310_write_reg(struct dps310_device *dev,
                                uint8_t reg, const uint8_t *data, uint16_t len)
{
    msg_t ret;
    uint8_t buf[16];
    if (dev == NULL || dev->bus == NULL || len > 15) return -1;
    buf[0] = reg;
    for (uint16_t i = 0; i < len; i++) buf[1 + i] = data[i];
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   buf, len + 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}


static int32_t dps310_twos_complement(int32_t val, uint8_t bits)
{
    if (val & ((int32_t)1 << (bits - 1)))
        val -= ((int32_t)1 << bits);
    return val;
}

int dps310_init(struct dps310_device *dev, I2CDriver *bus, uint16_t addr)
{
    if (dev == NULL) return -1;
    if (addr != DPS310_ADDR_LOW && addr != DPS310_ADDR_HIGH) return -1;
    dev->bus  = bus;
    dev->addr = addr;
    dev->coef_ready = 0;
    return 0;
}

int dps310_probe(struct dps310_device *dev)
{
    uint8_t id;
    int ret = dps310_read_reg(dev, DPS310_REG_PROD_ID, &id, 1);
    if (ret != 0) return ret;
    if (id != DPS310_PROD_ID_EXPECTED) return -3;
    return 0;
}

int dps310_read_calibration(struct dps310_device *dev)
{
    uint8_t coef[18];
    int ret;
    if (dev == NULL) return -1;
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
    dev->coef_ready = 1;
    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa)
{
    uint8_t buf[3], cfg_val;
    int32_t raw_psr, raw_tmp;
    int ret;
    if (dev == NULL || pressure_pa == NULL) return -1;
    if (!dev->coef_ready) return -1;

    /* Configure: single measurement, pressure */
    cfg_val = 0x01;
    ret = dps310_write_reg(dev, DPS310_REG_PRS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;
    cfg_val = 0x81;
    ret = dps310_write_reg(dev, DPS310_REG_TMP_CFG, &cfg_val, 1);
    if (ret != 0) return ret;

    /* Trigger pressure measurement */
    cfg_val = 0x01;
    ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;
    chThdSleepMilliseconds(50);


    ret = dps310_read_reg(dev, DPS310_REG_PSR_B2, buf, 3);
    if (ret != 0) return ret;
    raw_psr = dps310_twos_complement(((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2], 24);

    /* Trigger temperature measurement */
    cfg_val = 0x02;
    ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;
    chThdSleepMilliseconds(50);


    ret = dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3);
    if (ret != 0) return ret;
    raw_tmp = dps310_twos_complement(((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2], 24);

    /* Compensate using calibration coefficients (scaled integer) */
    float psr_sc = (float)raw_psr / 524288.0f;
    float tmp_sc = (float)raw_tmp / 524288.0f;
    float pcomp = (float)dev->c00 +
                  psr_sc * ((float)dev->c10 + psr_sc * ((float)dev->c20 + psr_sc * (float)dev->c30)) +
                  tmp_sc * (float)dev->c01 +
                  tmp_sc * psr_sc * ((float)dev->c11 + psr_sc * (float)dev->c21);
    *pressure_pa = (int32_t)pcomp;
    return 0;
}

int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[3], cfg_val;
    int32_t raw_tmp;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    if (!dev->coef_ready) return -1;

    cfg_val = 0x81;
    ret = dps310_write_reg(dev, DPS310_REG_TMP_CFG, &cfg_val, 1);
    if (ret != 0) return ret;
    cfg_val = 0x02;
    ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, &cfg_val, 1);
    if (ret != 0) return ret;
    chThdSleepMilliseconds(50);


    ret = dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3);
    if (ret != 0) return ret;
    raw_tmp = dps310_twos_complement(((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2], 24);

    float tmp_sc = (float)raw_tmp / 524288.0f;
    float temp = (float)dev->c0 * 0.5f + (float)dev->c1 * tmp_sc;
    *temp_mcelsius = (int32_t)(temp * 1000.0f);
    return 0;
}
