/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for ChibiOS
 */
#include "lsm303dlhc_ref.h"

static int lsm303dlhc_read_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                             uint8_t reg, uint8_t *buf, uint16_t len)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL || buf == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, addr,
                                   &reg, 1, buf, len, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

static int lsm303dlhc_write_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                              uint8_t reg, uint8_t val)
{
    msg_t ret;
    uint8_t buf[2] = {reg, val};
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, addr,
                                   buf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}


int lsm303dlhc_init(struct lsm303dlhc_device *dev, I2CDriver *bus, uint16_t accel_addr)
{
    if (dev == NULL) return -1;
    dev->bus = bus;
    dev->accel_addr = accel_addr;
    dev->mag_addr   = LSM303DLHC_ADDR_MAG;
    return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
    uint8_t val;
    return lsm303dlhc_read_reg_at(dev, dev->accel_addr, LSM303DLHC_REG_CTRL_REG1_A, &val, 1);
}

int lsm303dlhc_enable_accel(struct lsm303dlhc_device *dev)
{
    /* 50 Hz, all axes enabled */
    return lsm303dlhc_write_reg_at(dev, dev->accel_addr, LSM303DLHC_REG_CTRL_REG1_A, 0x47);
}

int lsm303dlhc_enable_mag(struct lsm303dlhc_device *dev)
{
    int ret;
    /* 15 Hz output rate */
    ret = lsm303dlhc_write_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_CRA_REG_M, 0x10);
    if (ret != 0) return ret;
    /* Gain +/- 1.3 gauss */
    ret = lsm303dlhc_write_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_CRB_REG_M, 0x20);
    if (ret != 0) return ret;
    /* Continuous conversion */
    return lsm303dlhc_write_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_MR_REG_M, 0x00);
}

int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    int ret;
    if (dev == NULL) return -1;
    /* Auto-increment read (set MSB of sub-address) */
    ret = lsm303dlhc_read_reg_at(dev, dev->accel_addr, LSM303DLHC_REG_OUT_X_L_A | 0x80, buf, 6);
    if (ret != 0) return ret;
    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    int ret;
    if (dev == NULL) return -1;
    ret = lsm303dlhc_read_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_OUT_X_H_M, buf, 6);
    if (ret != 0) return ret;
    /* Mag data is MSB first, order: X, Z, Y */
    *x = (int16_t)((buf[0] << 8) | buf[1]);
    *z = (int16_t)((buf[2] << 8) | buf[3]);
    *y = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}
