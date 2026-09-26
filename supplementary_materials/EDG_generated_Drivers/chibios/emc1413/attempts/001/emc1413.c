#include "emc1413.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
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

#define EMC1413_EXPECTED_MANUFACTURER_ID 0x5D
#define EMC1413_EXPECTED_PRODUCT_ID 0x21

#define I2C_TIMEOUT_MS 100
#define I2C_TIMEOUT TIME_MS2I(I2C_TIMEOUT_MS)

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *data, size_t len)
{
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, &reg, 1, data, len, I2C_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t value)
{
    uint8_t txbuf[2] = {reg, value};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, 2, NULL, 0, I2C_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    if (emc1413_write_then_read(dev, high_reg, &high_byte, 1) != 0)
        return -1;
    if (emc1413_write_then_read(dev, low_reg, &low_byte, 1) != 0)
        return -1;
    *temp = ((int32_t)high_byte * 1000) + ((((low_byte >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    chThdSleepMilliseconds(15);

    uint8_t id;
    if (emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, &id, 1) != 0)
        return -1;
    if (id != EMC1413_EXPECTED_MANUFACTURER_ID)
        return -1;

    if (emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, &id, 1) != 0)
        return -1;
    if (id != EMC1413_EXPECTED_PRODUCT_ID)
        return -1;

    if (emc1413_write(dev, EMC1413_REG_CONFIG, 0x00) != 0)
        return -1;
    if (emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06) != 0)
        return -1;

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
