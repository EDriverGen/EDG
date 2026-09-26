#include "tmp421.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW 0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_DELAY_MS 130

static int tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int tmp421_read_register(struct tmp421_device *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int ret;
    int timeout = 200;
    while (timeout--) {
        ret = tmp421_read_register(dev, TMP421_REG_STATUS, &status);
        if (ret < 0) {
            return ret;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return 0;
        }
        PrivTaskDelay(1);
    }
    return -ETIMEDOUT;
}

static int tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high_byte, low_byte;
    int16_t raw;
    int ret;

    ret = tmp421_poll_busy(dev);
    if (ret < 0) {
        return ret;
    }

    ret = tmp421_read_register(dev, high_reg, &high_byte);
    if (ret < 0) {
        return ret;
    }

    ret = tmp421_read_register(dev, low_reg, &low_byte);
    if (ret < 0) {
        return ret;
    }

    raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp_milli = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, struct I2cBus *bus_handle)
{
    int ret;
    uint8_t val;
    struct PrivIoctlCfg ioctl_cfg;

    dev->bus = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -EIO;
    }

    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    ret = tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &val);
    if (ret < 0 || val != 0x55) {
        PrivClose(dev->fd);
        return -EIO;
    }

    ret = tmp421_read_register(dev, TMP421_REG_DEVICE_ID, &val);
    if (ret < 0 || val != 0x21) {
        PrivClose(dev->fd);
        return -EIO;
    }

    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE1_HIGH, TMP421_REG_REMOTE1_LOW, temp_remote_val);
}
