#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>
#include <periph/i2c.h>

#include "riot.h"
#define EMC1413_I2C_ADDR 0x4C

static int emc1413_write_then_read(emc1413_t *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, &reg, 1, 0);
    if (ret < 0) {
        i2c_release(dev->bus);
        return ret;
    }
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int emc1413_write(emc1413_t *dev, uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, data, 2, 0);
    i2c_release(dev->bus);
    return ret;
}

static int emc1413_read_temperature(emc1413_t *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int ret;

    ret = emc1413_write_then_read(dev, high_reg, &high_byte, 1);
    if (ret < 0) return ret;

    ret = emc1413_write_then_read(dev, low_reg, &low_byte, 1);
    if (ret < 0) return ret;

    int32_t raw = ((int32_t)high_byte << 8) | low_byte;
    int32_t high = (raw >> 8) & 0xFF;
    int32_t frac = (raw >> 5) & 0x07;
    *temp = (high * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(emc1413_t *dev, i2c_t bus)
{
    dev->bus = bus;
    dev->addr = EMC1413_I2C_ADDR;

    ztimer_sleep(ZTIMER_MSEC, 15);

    uint8_t man_id, prod_id;
    int ret;

    ret = emc1413_write_then_read(dev, 0xFE, &man_id, 1);
    if (ret < 0 || man_id != 0x5D) return -1;

    ret = emc1413_write_then_read(dev, 0xFD, &prod_id, 1);
    if (ret < 0 || prod_id != 0x21) return -1;

    ret = emc1413_write(dev, 0x03, 0x00);
    if (ret < 0) return ret;

    ret = emc1413_write(dev, 0x04, 0x06);
    if (ret < 0) return ret;

    return 0;
}

int emc1413_read_internal_temperature(emc1413_t *dev, int32_t *temp_local_val)
{
    return emc1413_read_temperature(dev, 0x00, 0x29, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(emc1413_t *dev, int32_t *temp_ext1_val)
{
    return emc1413_read_temperature(dev, 0x01, 0x10, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(emc1413_t *dev, int32_t *temp_ext2_val)
{
    return emc1413_read_temperature(dev, 0x23, 0x24, temp_ext2_val);
}
