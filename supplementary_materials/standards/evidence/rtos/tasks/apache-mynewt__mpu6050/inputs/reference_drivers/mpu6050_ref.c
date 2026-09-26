#include "mpu6050_ref.h"

static int mpu6050_read_reg(struct mpu6050_device *dev, uint8_t reg,
                            uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buf == 0 || len == 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = len;
    xfer.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int mpu6050_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data xfer;
    uint8_t frame[2] = {reg, val};
    if (dev == 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = sizeof(frame);
    xfer.buffer = frame;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

static int16_t mpu6050_be16(const uint8_t *buf)
{
    return (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
}

int mpu6050_probe(struct mpu6050_device *dev)
{
    uint8_t id = 0;
    if (mpu6050_read_reg(dev, MPU6050_REG_WHO_AM_I, &id, 1) != 0) {
        return -1;
    }
    return id == MPU6050_WHO_AM_I_VAL ? 0 : -3;
}

int mpu6050_init(struct mpu6050_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0 || (addr != MPU6050_ADDR_LOW && addr != MPU6050_ADDR_HIGH)) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    if (mpu6050_probe(dev) != 0) {
        return -1;
    }
    return mpu6050_write_reg(dev, MPU6050_REG_PWR_MGMT1, 0x00);
}

int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (dev == 0 || ax == 0 || ay == 0 || az == 0) {
        return -1;
    }
    if (mpu6050_read_reg(dev, MPU6050_REG_ACCEL, buf, sizeof(buf)) != 0) {
        return -1;
    }
    *ax = mpu6050_be16(&buf[0]);
    *ay = mpu6050_be16(&buf[2]);
    *az = mpu6050_be16(&buf[4]);
    return 0;
}

int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];
    if (dev == 0 || gx == 0 || gy == 0 || gz == 0) {
        return -1;
    }
    if (mpu6050_read_reg(dev, MPU6050_REG_GYRO, buf, sizeof(buf)) != 0) {
        return -1;
    }
    *gx = mpu6050_be16(&buf[0]);
    *gy = mpu6050_be16(&buf[2]);
    *gz = mpu6050_be16(&buf[4]);
    return 0;
}
