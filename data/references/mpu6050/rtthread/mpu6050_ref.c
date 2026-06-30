/*
 * MPU6050 6-axis IMU Driver
 * WHO_AM_I(0x75)=0x68, PWR_MGMT_1(0x6B), ACCEL(0x3B 6B), GYRO(0x43 6B)
 */
#include "mpu6050_ref.h"


static int mpu6050_read_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->addr; msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg; msgs[0].len = 1;
    msgs[1].addr = dev->addr; msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf; msgs[1].len = len;
    return (rt_i2c_transfer(dev->bus, msgs, 2) == 2) ? 0 : -1;
}
static int mpu6050_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    struct rt_i2c_msg msg;
    msg.addr = dev->addr; msg.flags = RT_I2C_WR;
    msg.buf = buf; msg.len = 2;
    return (rt_i2c_transfer(dev->bus, &msg, 1) == 1) ? 0 : -1;
}

int mpu6050_probe(struct mpu6050_device *dev) {
    uint8_t id;
    int ret = mpu6050_read_reg(dev, 0x75, &id, 1);
    if (ret) return ret;
    return (id == MPU6050_WHO_AM_I_VAL) ? 0 : -3;
}

int mpu6050_init(struct mpu6050_device *dev, struct rt_i2c_bus_device * bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    /* Verify device identity before waking it up */
    int ret = mpu6050_probe(dev);
    if (ret) return ret;
    return mpu6050_write_reg(dev, 0x6B, 0x00);
}

int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6]; int ret;
    if (!dev) return -1;
    ret = mpu6050_read_reg(dev, 0x3B, buf, 6);
    if (ret) return ret;
    *ax = (int16_t)((buf[0]<<8)|buf[1]);
    *ay = (int16_t)((buf[2]<<8)|buf[3]);
    *az = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}

int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6]; int ret;
    if (!dev) return -1;
    ret = mpu6050_read_reg(dev, 0x43, buf, 6);
    if (ret) return ret;
    *gx = (int16_t)((buf[0]<<8)|buf[1]);
    *gy = (int16_t)((buf[2]<<8)|buf[3]);
    *gz = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}
