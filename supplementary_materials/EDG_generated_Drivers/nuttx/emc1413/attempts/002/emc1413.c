#include "emc1413.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "arch.h"

#define EMC1413_REG_MANUFACTURER_ID 0xFE
#define EMC1413_REG_PRODUCT_ID      0xFD
#define EMC1413_REG_CONFIG          0x03
#define EMC1413_REG_CONV_RATE       0x04
#define EMC1413_REG_INT_HIGH        0x00
#define EMC1413_REG_INT_LOW         0x29
#define EMC1413_REG_EXT1_HIGH       0x01
#define EMC1413_REG_EXT1_LOW        0x10
#define EMC1413_REG_EXT2_HIGH       0x23
#define EMC1413_REG_EXT2_LOW        0x24

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = i2c_write(dev->bus, &config, &reg, 1);
    if (ret < 0) return ret;
    return i2c_read(dev->bus, &config, buf, len);
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2] = {reg, val};
    return i2c_write(dev->bus, &config, buf, 2);
}

int emc1413_init(struct emc1413_dev *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) return -EINVAL;
    dev->bus = bus;
    dev->addr = EMC1413_I2C_ADDR;

    up_mdelay(15);

    uint8_t id;
    int ret;

    ret = emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, &id, 1);
    if (ret < 0) return ret;
    if (id != 0x5D) return -ENODEV;

    ret = emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, &id, 1);
    if (ret < 0) return ret;
    if (id != 0x21) return -ENODEV;

    ret = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret < 0) return ret;

    ret = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret < 0) return ret;

    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high, low;
    int ret;

    ret = emc1413_write_then_read(dev, reg_high, &high, 1);
    if (ret < 0) return ret;

    ret = emc1413_write_then_read(dev, reg_low, &low, 1);
    if (ret < 0) return ret;

    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp = high_byte * 1000 + frac * 125;
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