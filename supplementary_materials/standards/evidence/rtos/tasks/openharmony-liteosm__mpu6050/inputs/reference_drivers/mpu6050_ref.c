#include "mpu6050_ref.h"
#include <string.h>


static int openharmony_i2c_write(DevHandle bus, uint16_t addr,
                                 const uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = (uint8_t *)data;
    msg.len = len;
    msg.flags = 0;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_read(DevHandle bus, uint16_t addr,
                                uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = data;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_write_read(DevHandle bus, uint16_t addr,
                                      const uint8_t *wdata, uint16_t wlen,
                                      uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;
    msg[0].addr = addr;
    msg[0].buf = (uint8_t *)wdata;
    msg[0].len = wlen;
    msg[0].flags = 0;
    msg[1].addr = addr;
    msg[1].buf = rdata;
    msg[1].len = rlen;
    msg[1].flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, msg, 2) == 2) ? 0 : -1;
}

static int mpu_read_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int mpu_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return openharmony_i2c_write(dev->bus, dev->addr, buf, 2);
}

int mpu6050_init(struct mpu6050_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev || !bus) return -1;
    if (addr != MPU6050_ADDR_LOW && addr != MPU6050_ADDR_HIGH) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->bus = bus;
    dev->addr = addr;
    /* Verify chip identity (WHO_AM_I @0x75 == 0x68) before configuring it.
     * A robust driver MUST refuse to wake-up an unknown / mis-wired device. */
    int ret = mpu6050_probe(dev);
    if (ret) return ret;
    /*
     * Datasheet/Register Map:
     * - Device powers up in sleep mode.
     * - Using a gyro PLL clock source is recommended for better stability.
     * Writing 0x01 to PWR_MGMT_1 clears SLEEP and selects PLL with X gyro.
     */
    return mpu_write_reg(dev, 0x6B, 0x01);
}

int mpu6050_probe(struct mpu6050_device *dev) {
    uint8_t id;
    int ret = mpu_read_reg(dev, 0x75, &id, 1);
    if (ret) return ret;
    return (id == MPU6050_WHO_AM_I_VAL) ? 0 : -3;
}

int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6]; int ret;
    if (!dev || !ax || !ay || !az) return -1;
    ret = mpu_read_reg(dev, 0x3B, buf, 6);
    if (ret) return ret;
    *ax = (int16_t)((buf[0]<<8)|buf[1]);
    *ay = (int16_t)((buf[2]<<8)|buf[3]);
    *az = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}

int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6]; int ret;
    if (!dev || !gx || !gy || !gz) return -1;
    ret = mpu_read_reg(dev, 0x43, buf, 6);
    if (ret) return ret;
    *gx = (int16_t)((buf[0]<<8)|buf[1]);
    *gy = (int16_t)((buf[2]<<8)|buf[3]);
    *gz = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}
