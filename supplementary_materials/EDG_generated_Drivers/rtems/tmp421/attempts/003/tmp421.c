#include "tmp421.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define TMP421_I2C_ADDR 0x2A

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

static int read_status_register(int fd, uint8_t addr, uint8_t *status) {
    uint8_t wbuf = 0x08;
    return i2c_write_then_read(fd, addr, &wbuf, 1, status, 1);
}

static int poll_busy(int fd, uint8_t addr) {
    uint8_t status;
    int ret;
    int timeout = 100;
    while (timeout-- > 0) {
        ret = read_status_register(fd, addr, &status);
        if (ret < 0) return ret;
        if (!(status & 0x80)) return 0;
        usleep(1000);
    }
    return -ETIMEDOUT;
}

static int read_temperature_raw(int fd, uint8_t addr, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli) {
    uint8_t wbuf;
    uint8_t rbuf[1];
    uint8_t high_byte, low_byte;
    int16_t raw;
    int ret;

    wbuf = high_reg;
    ret = i2c_write_then_read(fd, addr, &wbuf, 1, rbuf, 1);
    if (ret < 0) return ret;
    high_byte = rbuf[0];

    wbuf = low_reg;
    ret = i2c_write_then_read(fd, addr, &wbuf, 1, rbuf, 1);
    if (ret < 0) return ret;
    low_byte = rbuf[0];

    raw = ((int16_t)((uint16_t)high_byte << 4)) | (low_byte >> 4);
    *temp_milli = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle) {
    const char *path = (const char *)bus_handle;
    int fd;
    uint8_t wbuf;
    uint8_t rbuf[1];
    int ret;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -errno;
    }
    dev->fd = fd;
    dev->i2c_addr = TMP421_I2C_ADDR;

    wbuf = 0xFE;
    ret = i2c_write_then_read(fd, dev->i2c_addr, &wbuf, 1, rbuf, 1);
    if (ret < 0) { close(fd); return ret; }

    wbuf = 0xFF;
    ret = i2c_write_then_read(fd, dev->i2c_addr, &wbuf, 1, rbuf, 1);
    if (ret < 0) { close(fd); return ret; }

    return 0;
}

int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *temp_local_val) {
    int ret;
    ret = poll_busy(dev->fd, dev->i2c_addr);
    if (ret < 0) return ret;
    return read_temperature_raw(dev->fd, dev->i2c_addr, 0x00, 0x10, temp_local_val);
}

int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *temp_remote_val) {
    int ret;
    ret = poll_busy(dev->fd, dev->i2c_addr);
    if (ret < 0) return ret;
    return read_temperature_raw(dev->fd, dev->i2c_addr, 0x01, 0x11, temp_remote_val);
}
