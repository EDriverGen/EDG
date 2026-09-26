#include "tmp421.h"
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "arch.h"

#define TMP421_ADDR 0x2A
#define REG_STATUS 0x08
#define REG_LOCAL_HIGH 0x00
#define REG_LOCAL_LOW 0x10
#define REG_REMOTE1_HIGH 0x01
#define REG_REMOTE1_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF
#define BUSY_BIT 0x80

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 400000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_WRITEREAD(dev->bus, &config, &reg, 1, buf, len);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int tmp421_read_reg(struct tmp421_dev *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int ret;
    for (int i = 0; i < 10; i++) {
        ret = tmp421_read_reg(dev, REG_STATUS, &status);
        if (ret < 0) return ret;
        if (!(status & BUSY_BIT)) return 0;
        up_mdelay(15);
    }
    return -ETIMEDOUT;
}

int tmp421_init(struct tmp421_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = TMP421_ADDR;
    uint8_t id;
    int ret;
    ret = tmp421_read_reg(dev, REG_MANUFACTURER_ID, &id);
    if (ret < 0) return ret;
    ret = tmp421_read_reg(dev, REG_DEVICE_ID, &id);
    if (ret < 0) return ret;
    return 0;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *val)
{
    int ret = tmp421_poll_busy(dev);
    if (ret < 0) return ret;
    uint8_t high, low;
    ret = tmp421_write_then_read(dev, high_reg, &high, 1);
    if (ret < 0) return ret;
    ret = tmp421_write_then_read(dev, low_reg, &low, 1);
    if (ret < 0) return ret;
    int16_t raw = ((int16_t)((int8_t)high) << 4) | (low >> 4);
    *val = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *val)
{
    return tmp421_read_temperature(dev, REG_LOCAL_HIGH, REG_LOCAL_LOW, val);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *val)
{
    return tmp421_read_temperature(dev, REG_REMOTE1_HIGH, REG_REMOTE1_LOW, val);
}