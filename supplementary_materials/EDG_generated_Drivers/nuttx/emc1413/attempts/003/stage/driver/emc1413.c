#include "emc1413.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
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

static int emc1413_write_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg_s msg;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    msg.frequency = 100000;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 2;
    int ret = I2C_TRANSFER(dev->bus, &msg, 1);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

static int emc1413_read_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    struct i2c_msg_s msg[2];
    msg[0].frequency = 100000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = &reg;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = dev->addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = val;
    msg[1].length = 1;
    int ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

int emc1413_init(struct emc1413_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = EMC1413_I2C_ADDR;

    up_mdelay(15);

    uint8_t id;
    int ret;

    ret = emc1413_read_reg(dev, EMC1413_REG_MANUFACTURER_ID, &id);
    if (ret < 0 || id != 0x5D) {
        return -ENODEV;
    }

    ret = emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &id);
    if (ret < 0 || id != 0x21) {
        return -ENODEV;
    }

    ret = emc1413_write_reg(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret < 0) {
        return ret;
    }

    ret = emc1413_write_reg(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret < 0) {
        return ret;
    }

    return OK;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high, low;
    int ret;

    ret = emc1413_read_reg(dev, reg_high, &high);
    if (ret < 0) {
        return ret;
    }

    ret = emc1413_read_reg(dev, reg_low, &low);
    if (ret < 0) {
        return ret;
    }

    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp = (high_byte * 1000) + (frac * 125);

    return OK;
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