#include "bh1750.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <dev/i2c/i2c.h>
#include <rtems.h>

#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_MT_HI 0x40
#define BH1750_CMD_MT_LO 0x60
#define BH1750_MEAS_DELAY_MS 180U

static int bh1750_io_error(void)
{
    return (errno != 0) ? -errno : -EIO;
}

static int bh1750_i2c_transfer(bh1750_dev_t *dev, struct i2c_msg *msgs, uint32_t nmsgs)
{
    int fd;
    int ret;
    struct i2c_rdwr_ioctl_data rdwr;

    if (dev == NULL || dev->bus_handle == NULL || msgs == NULL || nmsgs == 0U) {
        return -EINVAL;
    }

    errno = 0;
    fd = open((const char *)dev->bus_handle, O_RDWR);
    if (fd < 0) {
        return bh1750_io_error();
    }

    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;

    errno = 0;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        int err = bh1750_io_error();
        (void)close(fd);
        return err;
    }

    (void)close(fd);
    return 0;
}

static int bh1750_i2c_write(bh1750_dev_t *dev, const uint8_t *bytes, size_t len)
{
    struct i2c_msg msg;

    if (dev == NULL || dev->bus_handle == NULL || bytes == NULL || len == 0U) {
        return -EINVAL;
    }

    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.len = (uint16_t)len;
    msg.buf = (uint8_t *)bytes;

    return bh1750_i2c_transfer(dev, &msg, 1U);
}

static int bh1750_i2c_read(bh1750_dev_t *dev, uint8_t *bytes, size_t len)
{
    struct i2c_msg msg;

    if (dev == NULL || dev->bus_handle == NULL || bytes == NULL || len == 0U) {
        return -EINVAL;
    }

    msg.addr = dev->i2c_addr;
    msg.flags = I2C_M_RD;
    msg.len = (uint16_t)len;
    msg.buf = bytes;

    return bh1750_i2c_transfer(dev, &msg, 1U);
}

int bh1750_init(bh1750_dev_t *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
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

    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(BH1750_MEAS_DELAY_MS));

    ret = bh1750_i2c_read(dev, buf, 2U);
    if (ret != 0) {
        return ret;
    }

    sample = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
    *raw = (int32_t)sample;
    return 0;
}
