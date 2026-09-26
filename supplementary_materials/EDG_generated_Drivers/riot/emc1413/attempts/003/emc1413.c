#include "emc1413.h"
#include <stddef.h>
#include <stdint.h>

#include "riot.h"
#define EMC1413_ADDR 0x4C

static int emc1413_write_then_read(emc1413_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_read_regs(dev->bus, dev->addr, reg, data, len, 0);
    i2c_release(dev->bus);
    return ret;
}

static int emc1413_write(emc1413_t *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
    i2c_release(dev->bus);
    return ret;
}

int emc1413_init(emc1413_t *dev, i2c_t bus)
{
    dev->bus = bus;
    dev->addr = EMC1413_ADDR;

    ztimer_sleep(ZTIMER_MSEC, 15);

    uint8_t id;
    int ret;

    /* Probe: read Manufacturer ID */
    ret = emc1413_write_then_read(dev, 0xFE, &id, 1);
    if (ret < 0 || id != 0x5D) return -1;

    /* Probe: read Product ID */
    ret = emc1413_write_then_read(dev, 0xFD, &id, 1);
    if (ret < 0 || id != 0x21) return -1;

    /* Write Configuration */
    ret = emc1413_write(dev, 0x03, 0x00);
    if (ret < 0) return -1;

    /* Write Conversion Rate */
    ret = emc1413_write(dev, 0x04, 0x06);
    if (ret < 0) return -1;

    return 0;
}

static int emc1413_read_temperature(emc1413_t *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    int ret;

    ret = emc1413_write_then_read(dev, high_reg, &high, 1);
    if (ret < 0) return -1;

    ret = emc1413_write_then_read(dev, low_reg, &low, 1);
    if (ret < 0) return -1;

    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp = (high_byte * 1000) + (frac * 125);
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
