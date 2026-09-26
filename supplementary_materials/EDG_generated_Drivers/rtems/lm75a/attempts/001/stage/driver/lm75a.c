#include "lm75a.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "unistd.h"
#include <sys/ioctl.h>
#include "rtems.h"
#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG  0x00

static int lm75a_transfer_read(lm75a_dev_t *dev, uint8_t reg, uint8_t *rx, size_t rx_len)
{
    const char *bus_path;
    int fd;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t tx = reg;

    if (dev == NULL || rx == NULL || rx_len == 0 || dev->bus_handle == NULL) {
        return -EINVAL;
    }

    bus_path = (const char *)dev->bus_handle;
    fd = open(bus_path, O_RDWR);
    if (fd < 0) {
        return -errno;
    }

    memset(msgs, 0, sizeof(msgs));
    msgs[0].addr = LM75A_I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &tx;

    msgs[1].addr = LM75A_I2C_ADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = (uint16_t)rx_len;
    msgs[1].buf = rx;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        int err = -errno;
        close(fd);
        return err;
    }

    close(fd);
    return 0;
}

int lm75a_init(lm75a_dev_t *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -EINVAL;
    }

    dev->bus_handle = bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    return 0;
}

int lm75a_read_temperature(lm75a_dev_t *dev, int32_t *raw)
{
    uint8_t buf[2];
    int16_t signed_raw;
    int ret;

    if (dev == NULL || raw == NULL) {
        return -EINVAL;
    }

    ret = lm75a_transfer_read(dev, LM75A_TEMP_REG, buf, sizeof(buf));
    if (ret < 0) {
        return ret;
    }

    signed_raw = (int16_t)((((uint16_t)buf[0] << 8) | (uint16_t)buf[1]) >> 5);
    if (signed_raw & 0x0400) {
        signed_raw |= (int16_t)~0x07FF;
    }

    *raw = (int32_t)signed_raw;
    return 0;
}

int lm75a_read_temperature_mdegc(lm75a_dev_t *dev, int32_t *temperature_mdegC)
{
    uint8_t buf[2];
    int32_t raw11;
    int64_t temp;
    int ret;

    if (dev == NULL || temperature_mdegC == NULL) {
        return -EINVAL;
    }

    ret = lm75a_transfer_read(dev, LM75A_TEMP_REG, buf, sizeof(buf));
    if (ret < 0) {
        return ret;
    }

    raw11 = (int32_t)((((uint16_t)buf[0] << 8) | (uint16_t)buf[1]) >> 5);
    if (raw11 & 0x0400) {
        raw11 |= ~0x07FF;
    }

    temp = (int64_t)raw11 * 125;
    *temperature_mdegC = (int32_t)temp;
    return 0;
}
