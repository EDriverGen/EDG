#include "mpu6050.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define MPU6050_I2C_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_ACCEL_XOUT_H 0x3B

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;
    msg.addr = addr;
    msg.flags = 0;
    msg.len = len;
    msg.buf = buf;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = wlen;
    msgs[0].buf = wbuf;
    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = rlen;
    msgs[1].buf = rbuf;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

int mpu6050_init(struct mpu6050_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return -errno;
    }
    dev->fd = fd;
    dev->i2c_addr = MPU6050_I2C_ADDR;

    // Wake up device: clear SLEEP bit in PWR_MGMT_1
    uint8_t init_cmd[] = {MPU6050_PWR_MGMT_1, 0x00};
    int ret = i2c_write(fd, dev->i2c_addr, init_cmd, sizeof(init_cmd));
    if (ret < 0) {
        close(fd);
        return -EIO;
    }

    // Wait 100 ms for stabilization
    usleep(100000);

    // Probe: read WHO_AM_I register
    uint8_t probe_cmd = MPU6050_WHO_AM_I;
    uint8_t who_am_i;
    ret = i2c_write_then_read(fd, dev->i2c_addr, &probe_cmd, 1, &who_am_i, 1);
    if (ret < 0) {
        close(fd);
        return -EIO;
    }

    return 0;
}

int mpu6050_read_all(struct mpu6050_device *dev, int32_t *ax, int32_t *ay, int32_t *az, int32_t *temp, int32_t *gx, int32_t *gy, int32_t *gz)
{
    uint8_t cmd = MPU6050_ACCEL_XOUT_H;
    uint8_t buf[14];
    int ret = i2c_write_then_read(dev->fd, dev->i2c_addr, &cmd, 1, buf, 14);
    if (ret < 0) {
        return -EIO;
    }

    // Parse big-endian 16-bit signed values
    int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t raw_gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

    // Assign raw counts for accel and gyro (per SECTION B3)
    *ax = raw_ax;
    *ay = raw_ay;
    *az = raw_az;
    *gx = raw_gx;
    *gy = raw_gy;
    *gz = raw_gz;

    // Temperature: convert to milli_degC using integer approximation
    // temp_milli_degC = ((raw * 1000) / 340) + 36530
    int64_t temp_val = (int64_t)raw_temp * 1000 / 340 + 36530;
    *temp = (int32_t)temp_val;

    return 0;
}
