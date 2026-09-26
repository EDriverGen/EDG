#include "emc1413.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
#define EMC1413_I2C_ADDR 0x4C
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, &reg, 1, buf, len, TIME_INFINITE);
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t txbuf[2] = {reg, val};
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 2, NULL, 0, TIME_INFINITE);
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_read_byte(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    return emc1413_write_then_read(dev, reg, val, 1);
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_read_byte(dev, high_reg, &high) != 0) return -1;
    if (emc1413_read_byte(dev, low_reg, &low) != 0) return -1;
    int32_t frac = (low >> 5) & 0x07;
    *temp = ((int32_t)high * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    chThdSleepMilliseconds(15);

    uint8_t id;
    if (emc1413_read_byte(dev, EMC1413_REG_MANUFACTURER_ID, &id) != 0) return -1;
    if (id != 0x5D) return -1;
    if (emc1413_read_byte(dev, EMC1413_REG_PRODUCT_ID, &id) != 0) return -1;
    if (id != 0x21) return -1;

    if (emc1413_write(dev, EMC1413_REG_CONFIG, 0x00) != 0) return -1;
    if (emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06) != 0) return -1;

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
