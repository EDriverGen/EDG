#include "tmp421.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define TMP421_I2C_ADDR 0x2A

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
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

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
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

static int read_register(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return i2c_write_then_read(fd, addr, &reg, 1, buf, len);
}

static int poll_busy(int fd, uint8_t addr)
{
    uint8_t status;
    int ret;
    int timeout = 100;
    while (timeout--) {
        ret = read_register(fd, addr, 0x08, &status, 1);
        if (ret < 0) return ret;
        if (!(status & 0x80)) return 0;
        usleep(1000);
    }
    return -ETIMEDOUT;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd;
    uint8_t buf[1];
    int ret;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -EIO;
    }
    dev->fd = fd;
    dev->i2c_addr = TMP421_I2C_ADDR;

    /* Probe Manufacturer ID */
    ret = read_register(fd, dev->i2c_addr, 0xFE, buf, 1);
    if (ret < 0 || buf[0] != 0x55) {
        close(fd);
        return -EIO;
    }

    /* Probe Device ID */
    ret = read_register(fd, dev->i2c_addr, 0xFF, buf, 1);
    if (ret < 0 || buf[0] != 0x21) {
        close(fd);
        return -EIO;
    }

    return 0;
}

static int read_temperature_channel(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *val)
{
    uint8_t high_byte, low_byte;
    int16_t raw;
    int ret;

    ret = poll_busy(dev->fd, dev->i2c_addr);
    if (ret < 0) return ret;

    ret = read_register(dev->fd, dev->i2c_addr, high_reg, &high_byte, 1);
    if (ret < 0) return ret;

    ret = read_register(dev->fd, dev->i2c_addr, low_reg, &low_byte, 1);
    if (ret < 0) return ret;

    /* Check open circuit or power fault for remote channel */
    if (low_reg == 0x11) {
        if (low_byte & 0x01) return -ENODEV;
        if (low_byte & 0x02) return -EIO;
    }

    /* Combine: 12-bit signed, right shift low nibble */
    raw = ((int16_t)((uint16_t)high_byte << 4)) | (low_byte >> 4);
    /* Sign extend from bit 11 */
    if (raw & 0x0800) {
        raw |= 0xF000;
    }

    /* Convert to milli_degC: raw * 625 / 10 */
    *val = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_read_temperature_local(struct tmp421_device *dev, int32_t *val)
{
    return read_temperature_channel(dev, 0x00, 0x10, val);
}

int tmp421_read_temperature_remote1(struct tmp421_device *dev, int32_t *val)
{
    return read_temperature_channel(dev, 0x01, 0x11, val);
}
