#include "tmp421.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#include <errno.h>
#include <string.h>

#include "bus_i2c.h"
#define TMP421_ADDR 0x2A
#define REG_STATUS 0x08
#define REG_LOCAL_HIGH 0x00
#define REG_LOCAL_LOW 0x10
#define REG_REMOTE_HIGH 0x01
#define REG_REMOTE_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF
#define BUSY_BIT 0x80

static int i2c_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) != 0) {
        return -EIO;
    }
    if (PrivWrite(dev->fd, &reg, 1) != 1) {
        return -EIO;
    }
    if (len > 0) {
        if (PrivRead(dev->fd, buf, len) != (int)len) {
            return -EIO;
        }
    }
    return 0;
}

static int i2c_write(struct tmp421_device *dev, uint8_t reg, uint8_t val)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) != 0) {
        return -EIO;
    }
    uint8_t buf[2] = {reg, val};
    if (PrivWrite(dev->fd, buf, 2) != 2) {
        return -EIO;
    }
    return 0;
}

static int poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int ret;
    for (int i = 0; i < 10; i++) {
        ret = i2c_write_then_read(dev, REG_STATUS, &status, 1);
        if (ret != 0) return ret;
        if (!(status & BUSY_BIT)) return 0;
        PrivTaskDelay(15);
    }
    return -ETIMEDOUT;
}

static int read_temperature(struct tmp421_device *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    int ret = poll_busy(dev);
    if (ret != 0) return ret;
    uint8_t high, low;
    ret = i2c_write_then_read(dev, reg_high, &high, 1);
    if (ret != 0) return ret;
    ret = i2c_write_then_read(dev, reg_low, &low, 1);
    if (ret != 0) return ret;
    int16_t raw = ((int16_t)((int8_t)high) << 4) | (low >> 4);
    *temp = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, struct I2cBus *bus_handle)
{
    dev->bus = bus_handle;
    dev->addr = TMP421_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) {
        return -EIO;
    }
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) != 0) {
        PrivClose(dev->fd);
        return -EIO;
    }
    uint8_t id;
    if (i2c_write_then_read(dev, REG_MANUFACTURER_ID, &id, 1) != 0 || id != 0x55) {
        PrivClose(dev->fd);
        return -EIO;
    }
    if (i2c_write_then_read(dev, REG_DEVICE_ID, &id, 1) != 0 || id != 0x21) {
        PrivClose(dev->fd);
        return -EIO;
    }
    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val)
{
    return read_temperature(dev, REG_LOCAL_HIGH, REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val)
{
    return read_temperature(dev, REG_REMOTE_HIGH, REG_REMOTE_LOW, temp_remote_val);
}
