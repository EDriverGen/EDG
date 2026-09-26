#include "mpu6050_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int mpu6050_transfer(struct mpu6050_device *dev, struct i2c_msg *msgs,
                            uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || msgs == 0 || nmsgs == 0) {
        return -1;
    }
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int mpu6050_read_reg(struct mpu6050_device *dev, uint8_t reg,
                            uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    return mpu6050_transfer(dev, msgs, 2);
}

static int mpu6050_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg msg;
    uint8_t frame[2] = {reg, val};
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    return mpu6050_transfer(dev, &msg, 1);
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

int mpu6050_init(struct mpu6050_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0 || (addr != MPU6050_ADDR_LOW && addr != MPU6050_ADDR_HIGH)) {
        return -1;
    }
    dev->bus_path = bus_path;
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
