#include "lsm303dlhc.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define ACCEL_ADDR 0x19
#define MAG_ADDR 0x1E

static int i2c_write(int fd, uint8_t addr, uint8_t reg, uint8_t val) {
    struct i2c_msg msg;
    uint8_t buf[2] = {reg, val};
    msg.addr = addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = buf;
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = &msg;
    rdwr.nmsgs = 1;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t reg, uint8_t *rxbuf, uint16_t len) {
    struct i2c_msg msgs[2];
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = rxbuf;
    struct i2c_rdwr_ioctl_data rdwr;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;
    return ioctl(fd, I2C_RDWR, &rdwr);
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle) {
    const char *path = (const char *)bus_handle;
    int fd = open(path, O_RDWR);
    if (fd < 0) return -errno;
    dev->fd = fd;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    int ret;
    ret = i2c_write(fd, ACCEL_ADDR, 0x20, 0x57);
    if (ret < 0) return -EIO;
    ret = i2c_write(fd, ACCEL_ADDR, 0x23, 0x08);
    if (ret < 0) return -EIO;
    usleep(70000);
    ret = i2c_write(fd, MAG_ADDR, 0x00, 0x0C);
    if (ret < 0) return -EIO;
    ret = i2c_write(fd, MAG_ADDR, 0x01, 0x20);
    if (ret < 0) return -EIO;
    ret = i2c_write(fd, MAG_ADDR, 0x02, 0x00);
    if (ret < 0) return -EIO;
    usleep(1000);
    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    int ret = i2c_write_then_read(dev->fd, dev->accel_addr, 0xA8, buf, 6);
    if (ret < 0) return -EIO;
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int32_t *mx, int32_t *my, int32_t *mz) {
    uint8_t buf[6];
    int ret = i2c_write_then_read(dev->fd, dev->mag_addr, 0x03, buf, 6);
    if (ret < 0) return -EIO;
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_z = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_y = (int16_t)((buf[4] << 8) | buf[5]);
    *mx = ((int32_t)raw_x * 1000) / 1100;
    *my = ((int32_t)raw_y * 1000) / 1100;
    *mz = ((int32_t)raw_z * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t) {
    uint8_t buf[2];
    int ret = i2c_write_then_read(dev->fd, dev->mag_addr, 0x31, buf, 2);
    if (ret < 0) return -EIO;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
