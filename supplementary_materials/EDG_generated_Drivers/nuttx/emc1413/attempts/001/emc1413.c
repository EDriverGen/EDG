#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

#define EMC1413_REG_MANUFACTURER_ID 0xFE
#define EMC1413_REG_PRODUCT_ID 0xFD
#define EMC1413_REG_CONFIG 0x03
#define EMC1413_REG_CONV_RATE 0x04
#define EMC1413_REG_INT_HIGH 0x00
#define EMC1413_REG_INT_LOW 0x29
#define EMC1413_REG_EXT1_HIGH 0x01
#define EMC1413_REG_EXT1_LOW 0x10
#define EMC1413_REG_EXT2_HIGH 0x23
#define EMC1413_REG_EXT2_LOW 0x24

#define EMC1413_MANUFACTURER_ID 0x5D
#define EMC1413_PRODUCT_ID 0x21

static int emc1413_write_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2] = { reg, val };
    int ret = I2C_TRANSFER(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int emc1413_read_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_TRANSFER(dev->bus, &config, &reg, 1);
    if (ret < 0) {
        return -EIO;
    }
    ret = I2C_TRANSFER(dev->bus, &config, val, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    int ret;
    ret = emc1413_read_reg(dev, high_reg, &high);
    if (ret < 0) return ret;
    ret = emc1413_read_reg(dev, low_reg, &low);
    if (ret < 0) return ret;
    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp = (high_byte * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = EMC1413_I2C_ADDR;
    up_mdelay(15);
    uint8_t val;
    int ret;
    ret = emc1413_read_reg(dev, EMC1413_REG_MANUFACTURER_ID, &val);
    if (ret < 0 || val != EMC1413_MANUFACTURER_ID) return -ENODEV;
    ret = emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &val);
    if (ret < 0 || val != EMC1413_PRODUCT_ID) return -ENODEV;
    ret = emc1413_write_reg(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret < 0) return ret;
    ret = emc1413_write_reg(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret < 0) return ret;
    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp);
}