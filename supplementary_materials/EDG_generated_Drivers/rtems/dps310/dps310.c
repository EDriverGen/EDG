#include "dps310.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define DPS310_I2C_ADDR 0x77

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
        return -1;
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
        return -1;
    }
    return 0;
}

int dps310_init(struct dps310_device *dev, void *bus_handle)
{
    const char *path = (const char *)bus_handle;
    int fd;
    uint8_t wbuf[2];
    uint8_t rbuf[1];

    fd = open(path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    dev->fd = fd;
    dev->i2c_addr = DPS310_I2C_ADDR;

    /* Read ID register (0x0D) */
    wbuf[0] = 0x0D;
    if (i2c_write_then_read(dev->fd, dev->i2c_addr, wbuf, 1, rbuf, 1) < 0) {
        close(dev->fd);
        return -1;
    }

    /* Soft reset: write 0x0C with value 0x09 */
    wbuf[0] = 0x0C;
    wbuf[1] = 0x09;
    if (i2c_write(dev->fd, dev->i2c_addr, wbuf, 2) < 0) {
        close(dev->fd);
        return -1;
    }

    /* Read calibration coefficients (0x10, 18 bytes) */
    wbuf[0] = 0x10;
    uint8_t coeff_buf[18];
    if (i2c_write_then_read(dev->fd, dev->i2c_addr, wbuf, 1, coeff_buf, 18) < 0) {
        close(dev->fd);
        return -1;
    }

    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_raw)
{
    uint8_t wbuf[1];
    uint8_t rbuf[3];
    int32_t raw;

    wbuf[0] = 0x00;
    if (i2c_write_then_read(dev->fd, dev->i2c_addr, wbuf, 1, rbuf, 3) < 0) {
        return -1;
    }

    raw = ((int32_t)rbuf[0] << 16) | ((int32_t)rbuf[1] << 8) | (int32_t)rbuf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(struct dps310_device *dev, int32_t *temp_raw)
{
    uint8_t wbuf[1];
    uint8_t rbuf[3];
    int32_t raw;

    wbuf[0] = 0x03;
    if (i2c_write_then_read(dev->fd, dev->i2c_addr, wbuf, 1, rbuf, 3) < 0) {
        return -1;
    }

    raw = ((int32_t)rbuf[0] << 16) | ((int32_t)rbuf[1] << 8) | (int32_t)rbuf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *temp_raw = raw;
    return 0;
}
