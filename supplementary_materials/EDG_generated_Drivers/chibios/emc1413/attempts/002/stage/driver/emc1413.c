#include "emc1413.h"
#include "ch.h"
#include "hal.h"
#include <stdint.h>
#include <string.h>

#include "chibios.h"
#define EMC1413_MANUFACTURER_ID 0x5D
#define EMC1413_PRODUCT_ID 0x21

static int emc1413_write_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_read_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t *data)
{
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, &reg, 1, data, 1, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int emc1413_read_temp_channel(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    if (emc1413_read_reg(dev, high_reg, &high_byte) != 0) return -1;
    if (emc1413_read_reg(dev, low_reg, &low_byte) != 0) return -1;
    int32_t raw = ((int32_t)high_byte << 8) | low_byte;
    int32_t high = (raw >> 8) & 0xFF;
    int32_t frac = (raw >> 5) & 0x07;
    *temp = (high * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    chThdSleepMilliseconds(15);

    uint8_t id;
    if (emc1413_read_reg(dev, 0xFE, &id) != 0) return -1;
    if (id != EMC1413_MANUFACTURER_ID) return -1;
    if (emc1413_read_reg(dev, 0xFD, &id) != 0) return -1;
    if (id != EMC1413_PRODUCT_ID) return -1;

    if (emc1413_write_reg(dev, 0x03, 0x00) != 0) return -1;
    if (emc1413_write_reg(dev, 0x04, 0x06) != 0) return -1;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp_channel(dev, 0x00, 0x29, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp_channel(dev, 0x01, 0x10, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp_channel(dev, 0x23, 0x24, temp);
}
