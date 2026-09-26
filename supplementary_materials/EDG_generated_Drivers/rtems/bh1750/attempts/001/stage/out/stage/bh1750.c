#include "bh1750.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include "rtems.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "unistd.h"
#include <sys/ioctl.h>
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_MT_HI 0x40
#define BH1750_CMD_MT_LO 0x60

static int bh1750_i2c_write(bh1750_dev_t *dev, const uint8_t *bytes, size_t len)
{
    int fd;
    struct i2c_msg msg;
    struct i2c_rdwr_ioctl_data rdwr;

    if (dev == NULL || dev->bus_handle == NULL || bytes == NULL || len == 0U) {
        return -EINVAL;
    }

    fd = open((const char *)dev->bus_handle, O_RDWR);
    if (fd < 0) {
        return -errno;
    }

    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.len = (uint16_t)len;
    msg.buf = (uint8_t *)bytes;

    rdwr.msgs = &msg;
    rdwr.nmsgs = 1U;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        int err = -errno;
        close(fd);
        return err;
    }

    close(fd);
    return 0;
}

static int bh1750_i2c_read(bh1750_dev_t *dev, uint8_t *bytes, size_t len)
{
    int fd;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t cmd = BH1750_CMD_CONT_HRES;

    if (dev == NULL || dev->bus_handle == NULL || bytes == NULL || len == 0U) {
        return -EINVAL;
    }

    fd = open((const char *)dev->bus_handle, O_RDWR);
    if (fd < 0) {
        return -errno;
    }

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1U;
    msgs[0].buf = &cmd;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (uint16_t)len;
    msgs[1].buf = bytes;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2U;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        int err = -errno;
        close(fd);
        return err;
    }

    close(fd);
    return 0;
}

int bh1750_init(bh1750_dev_t *dev, void *bus_handle)
{
    if (dev == NULL) {
        return -EINVAL;
    }

    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    return 0;
}

int bh1750_set_mtreg(bh1750_dev_t *dev, uint8_t mtreg)
{
    uint8_t hi;
    uint8_t lo;
    int ret;

    if (dev == NULL) {
        return -EINVAL;
    }

    if (mtreg < 31U) {
        mtreg = 31U;
    } else if (mtreg > 254U) {
        mtreg = 254U;
    }

    hi = (uint8_t)(BH1750_CMD_MT_HI | ((mtreg >> 5) & 0x07U));
    lo = (uint8_t)(BH1750_CMD_MT_LO | (mtreg & 0x1FU));

    ret = bh1750_i2c_write(dev, &hi, 1U);
    if (ret != 0) {
        return ret;
    }

    ret = bh1750_i2c_write(dev, &lo, 1U);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int bh1750_read_illuminance(bh1750_dev_t *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;
    uint16_t sample;

    if (dev == NULL || raw == NULL) {
        return -EINVAL;
    }

    ret = bh1750_i2c_write(dev, (const uint8_t[]){ BH1750_CMD_CONT_HRES }, 1U);
    if (ret != 0) {
        return ret;
    }

    ret = bh1750_i2c_read(dev, buf, 2U);
    if (ret != 0) {
        return ret;
    }

    sample = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
    *raw = (int32_t)sample;
    return 0;
}
