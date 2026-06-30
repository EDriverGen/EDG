/*
 * MPU6050 6-axis IMU Driver
 * WHO_AM_I(0x75)=0x68, PWR_MGMT_1(0x6B), ACCEL(0x3B 6B), GYRO(0x43 6B)
 */
#include "mpu6050_ref.h"
#include <string.h>


static int mpu6050_threadx_i2c_write(struct mpu6050_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int mpu6050_threadx_i2c_read(struct mpu6050_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int mpu6050_threadx_i2c_write_read(struct mpu6050_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define MPU6050_I2C_WRITE(_bus, _addr, _data, _len) \
    mpu6050_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define MPU6050_I2C_READ(_bus, _addr, _data, _len) \
    mpu6050_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define MPU6050_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    mpu6050_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

static int mpu6050_read_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return MPU6050_I2C_WRITE_READ(dev->bus_context, dev->addr, &reg, 1, buf, len);
}
static int mpu6050_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return MPU6050_I2C_WRITE(dev->bus_context, dev->addr, buf, 2);
}

int mpu6050_init(struct mpu6050_device *dev, void *bus_context, const struct mpu6050_i2c_ops *ops, uint16_t addr) {
    if (!dev || !bus_context || !ops) return -1;
    if (addr != MPU6050_ADDR_LOW && addr != MPU6050_ADDR_HIGH) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->bus_context = bus_context;
    dev->ops = ops;
    dev->addr = addr;
    /* Verify chip identity (WHO_AM_I @0x75 == 0x68) before configuring it.
     * A robust driver MUST refuse to wake-up an unknown / mis-wired device. */
    int ret = mpu6050_probe(dev);
    if (ret) return ret;
    /*
     * Datasheet/Register Map:
     * - Device powers up in sleep mode.
     * - Using a gyro PLL clock source is recommended for better stability.
     */
    return mpu6050_write_reg(dev, 0x6B, 0x01);
}

int mpu6050_probe(struct mpu6050_device *dev) {
    uint8_t id;
    int ret = mpu6050_read_reg(dev, 0x75, &id, 1);
    if (ret) return ret;
    return (id == MPU6050_WHO_AM_I_VAL) ? 0 : -3;
}

int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6]; int ret;
    if (!dev || !ax || !ay || !az) return -1;
    ret = mpu6050_read_reg(dev, 0x3B, buf, 6);
    if (ret) return ret;
    *ax = (int16_t)((buf[0]<<8)|buf[1]);
    *ay = (int16_t)((buf[2]<<8)|buf[3]);
    *az = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}

int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6]; int ret;
    if (!dev || !gx || !gy || !gz) return -1;
    ret = mpu6050_read_reg(dev, 0x43, buf, 6);
    if (ret) return ret;
    *gx = (int16_t)((buf[0]<<8)|buf[1]);
    *gy = (int16_t)((buf[2]<<8)|buf[3]);
    *gz = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}
