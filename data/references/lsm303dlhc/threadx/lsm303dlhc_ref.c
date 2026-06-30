/*
 * SPDX-License-Identifier: MIT
 *
 * LSM303DLHC Accelerometer/Magnetometer Driver for ThreadX
 */
#include "lsm303dlhc_ref.h"


static int lsm303dlhc_threadx_i2c_write(struct lsm303dlhc_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int lsm303dlhc_threadx_i2c_read(struct lsm303dlhc_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int lsm303dlhc_threadx_i2c_write_read(struct lsm303dlhc_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define LSM303DLHC_I2C_WRITE(_bus, _addr, _data, _len) \
    lsm303dlhc_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define LSM303DLHC_I2C_READ(_bus, _addr, _data, _len) \
    lsm303dlhc_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define LSM303DLHC_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    lsm303dlhc_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int lsm303dlhc_read_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                             uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || buf == NULL) return -1;
    return LSM303DLHC_I2C_WRITE_READ(dev->bus_context, addr, &reg, 1, buf, len);
}

static int lsm303dlhc_write_reg_at(struct lsm303dlhc_device *dev, uint16_t addr,
                              uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return LSM303DLHC_I2C_WRITE(dev->bus_context, addr, buf, 2);
}


int lsm303dlhc_init(struct lsm303dlhc_device *dev, void *bus_context, const struct lsm303dlhc_i2c_ops *ops, uint16_t accel_addr)
{
    if (dev == NULL || ops == NULL) return -1;
    dev->bus_context = bus_context;
    dev->ops = ops;
    dev->accel_addr = accel_addr;
    dev->mag_addr   = LSM303DLHC_ADDR_MAG;
    return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
    uint8_t accel_ctrl;
    uint8_t ira;
    uint8_t irb;
    uint8_t irc;
    int ret;

    if (dev == NULL || dev->bus_context == NULL) return -1;
    ret = lsm303dlhc_read_reg_at(dev, dev->accel_addr, LSM303DLHC_REG_CTRL_REG1_A, &accel_ctrl, 1);
    if (ret != 0) return ret;
    ret = lsm303dlhc_read_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_IRA_REG_M, &ira, 1);
    if (ret != 0) return ret;
    ret = lsm303dlhc_read_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_IRB_REG_M, &irb, 1);
    if (ret != 0) return ret;
    ret = lsm303dlhc_read_reg_at(dev, dev->mag_addr, LSM303DLHC_REG_IRC_REG_M, &irc, 1);
    if (ret != 0) return ret;
    if (ira != LSM303DLHC_IRA_VALUE || irb != LSM303DLHC_IRB_VALUE || irc != LSM303DLHC_IRC_VALUE) return -3;
    return 0;
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

/* RT-Thread API compatibility wrappers */
int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev) {
    return lsm303dlhc_enable_accel(dev);
}

int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel) {
    if (!accel) return -1;
    return lsm303dlhc_read_accel(dev, &accel->x, &accel->y, &accel->z);
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev) {
    return lsm303dlhc_enable_mag(dev);
}

int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag) {
    if (!mag) return -1;
    return lsm303dlhc_read_mag(dev, &mag->x, &mag->y, &mag->z);
}
