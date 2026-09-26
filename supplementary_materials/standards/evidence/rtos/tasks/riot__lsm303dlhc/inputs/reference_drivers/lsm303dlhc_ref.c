/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for RIOT OS
 */
#include "lsm303dlhc_ref.h"

static int lsm303dlhc_read_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                             uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, addr, reg, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int lsm303dlhc_write_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                              uint8_t reg, uint8_t val)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_reg(dev->bus, addr, reg, val, 0);
    i2c_release(dev->bus);
    return ret;
}


int lsm303dlhc_init(struct lsm303dlhc_device *dev, i2c_t bus, uint16_t accel_addr)
{
    if (dev == NULL) return -EINVAL;
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
    if (dev == NULL) return -EINVAL;
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
    if (dev == NULL) return -EINVAL;
    ret = lsm303dlhc_read_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_OUT_X_H_M, buf, 6);
    if (ret != 0) return ret;
    /* Mag data is MSB first, order: X, Z, Y */
    *x = (int16_t)((buf[0] << 8) | buf[1]);
    *z = (int16_t)((buf[2] << 8) | buf[3]);
    *y = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}
