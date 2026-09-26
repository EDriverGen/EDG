#include "tmp421.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

#define TMP421_REG_STATUS 0x08
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE1_HIGH 0x01
#define TMP421_REG_REMOTE1_LOW 0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_DELAY_MS 130

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 400000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_TRANSFER(dev->bus, &config, buf, len);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int tmp421_read_reg(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    uint8_t cmd = reg;
    struct i2c_msg_s msg[2];
    msg[0].frequency = 400000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = &cmd;
    msg[0].length = 1;
    msg[1].frequency = 400000;
    msg[1].addr = dev->addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = len;
    int ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int ret;
    int timeout = 10;
    do {
        ret = tmp421_read_reg(dev, TMP421_REG_STATUS, &status, 1);
        if (ret < 0) return ret;
        if (!(status & TMP421_BUSY_BIT)) break;
        up_mdelay(15);
        timeout--;
    } while (timeout > 0);
    if (timeout == 0) return -ETIMEDOUT;
    return 0;
}

static int tmp421_read_temperature(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    int ret;
    uint8_t high_byte, low_byte;
    ret = tmp421_read_reg(dev, high_reg, &high_byte, 1);
    if (ret < 0) return ret;
    ret = tmp421_read_reg(dev, low_reg, &low_byte, 1);
    if (ret < 0) return ret;
    int16_t high_signed = (int16_t)(int8_t)high_byte;
    uint8_t low_nibble = low_byte >> 4;
    int32_t combined = ((int32_t)high_signed << 4) + low_nibble;
    *temp = combined * 625 / 10;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) return -EINVAL;
    dev->bus = bus;
    dev->addr = TMP421_I2C_ADDR;
    uint8_t buf[1];
    int ret;
    ret = tmp421_read_reg(dev, TMP421_REG_MANUFACTURER_ID, buf, 1);
    if (ret < 0) return ret;
    if (buf[0] != 0x55) return -ENODEV;
    ret = tmp421_read_reg(dev, TMP421_REG_DEVICE_ID, buf, 1);
    if (ret < 0) return ret;
    if (buf[0] != 0x21) return -ENODEV;
    up_mdelay(TMP421_CONVERSION_DELAY_MS);
    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp)
{
    if (!dev || !temp) return -EINVAL;
    int ret = tmp421_poll_busy(dev);
    if (ret < 0) return ret;
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp)
{
    if (!dev || !temp) return -EINVAL;
    int ret = tmp421_poll_busy(dev);
    if (ret < 0) return ret;
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE1_HIGH, TMP421_REG_REMOTE1_LOW, temp);
}