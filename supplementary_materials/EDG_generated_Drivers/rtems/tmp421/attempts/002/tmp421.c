#include "tmp421.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "rtems.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#define TMP421_ADDR 0x2A
#define STATUS_REG 0x08
#define LOCAL_TEMP_HIGH 0x00
#define LOCAL_TEMP_LOW 0x10
#define REMOTE1_TEMP_HIGH 0x01
#define REMOTE1_TEMP_LOW 0x11
#define MANUFACTURER_ID 0xFE
#define DEVICE_ID 0xFF
#define BUSY_BIT 0x80

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen) {
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

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

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len) {
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = len;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int read_register(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len) {
    return i2c_write_then_read(fd, addr, &reg, 1, buf, len);
}

static int poll_busy(int fd, uint8_t addr) {
    uint8_t status;
    int ret;
    int timeout = 100;
    while (timeout--) {
        ret = read_register(fd, addr, STATUS_REG, &status, 1);
        if (ret < 0) return ret;
        if (!(status & BUSY_BIT)) return 0;
        usleep(1000);
    }
    return -ETIMEDOUT;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle) {
    const char *path = (const char *)bus_handle;
    int fd;
    uint8_t buf[1];
    int ret;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -errno;
    }
    dev->fd = fd;
    dev->addr = TMP421_ADDR;

    buf[0] = MANUFACTURER_ID;
    ret = i2c_write_then_read(fd, dev->addr, buf, 1, buf, 1);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    buf[0] = DEVICE_ID;
    ret = i2c_write_then_read(fd, dev->addr, buf, 1, buf, 1);
    if (ret < 0) {
        close(fd);
        return ret;
    }

    return 0;
}

static int read_temperature_channel(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *val) {
    uint8_t high_byte, low_byte;
    int16_t raw;
    int ret;

    ret = poll_busy(dev->fd, dev->addr);
    if (ret < 0) return ret;

    ret = read_register(dev->fd, dev->addr, high_reg, &high_byte, 1);
    if (ret < 0) return ret;

    ret = read_register(dev->fd, dev->addr, low_reg, &low_byte, 1);
    if (ret < 0) return ret;

    raw = ((int16_t)((uint16_t)high_byte << 4)) | (low_byte >> 4);
    if (raw & 0x0800) {
        raw |= 0xF000;
    }
    *val = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *val) {
    return read_temperature_channel(dev, LOCAL_TEMP_HIGH, LOCAL_TEMP_LOW, val);
}

int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *val) {
    return read_temperature_channel(dev, REMOTE1_TEMP_HIGH, REMOTE1_TEMP_LOW, val);
}
