#include "emc1413.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "apache_mynewt.h"
#define EMC1413_REG_INTERNAL_HIGH 0x00
#define EMC1413_REG_INTERNAL_LOW  0x29
#define EMC1413_REG_EXT1_HIGH     0x01
#define EMC1413_REG_EXT1_LOW      0x10
#define EMC1413_REG_EXT2_HIGH     0x23
#define EMC1413_REG_EXT2_LOW      0x24
#define EMC1413_REG_CONFIG        0x03
#define EMC1413_REG_CONV_RATE     0x04
#define EMC1413_REG_MANUF_ID      0xFE
#define EMC1413_REG_PROD_ID       0xFD
#define EMC1413_MANUF_ID_VAL      0x5D
#define EMC1413_PROD_ID_VAL       0x21

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data data;
    int rc;

    data.address = EMC1413_I2C_ADDR;
    data.buffer = &reg;
    data.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TICK_PER_SECOND / 10, 1);
    if (rc != 0) {
        return rc;
    }

    data.buffer = buf;
    data.len = len;
    rc = hal_i2c_master_read(dev->i2c_num, &data, OS_TICK_PER_SECOND / 10, 1);
    return rc;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data data;
    uint8_t buf[2] = {reg, val};
    data.address = EMC1413_I2C_ADDR;
    data.buffer = buf;
    data.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &data, OS_TICK_PER_SECOND / 10, 1);
}

static int emc1413_read_byte(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    return emc1413_write_then_read(dev, reg, val, 1);
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp_milli)
{
    uint8_t high, low;
    int rc;

    rc = emc1413_read_byte(dev, reg_high, &high);
    if (rc != 0) return rc;
    rc = emc1413_read_byte(dev, reg_low, &low);
    if (rc != 0) return rc;

    int32_t raw = ((int32_t)high << 8) | low;
    int32_t integer = (raw >> 8) & 0xFF;
    int32_t frac = (raw >> 5) & 0x07;
    *temp_milli = integer * 1000 + frac * 125;
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    uint8_t val;
    int rc;

    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;

    os_time_delay(OS_TICK_PER_SECOND * 15 / 1000);

    rc = emc1413_read_byte(dev, EMC1413_REG_MANUF_ID, &val);
    if (rc != 0 || val != EMC1413_MANUF_ID_VAL) return -1;

    rc = emc1413_read_byte(dev, EMC1413_REG_PROD_ID, &val);
    if (rc != 0 || val != EMC1413_PROD_ID_VAL) return -1;

    rc = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (rc != 0) return rc;

    rc = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (rc != 0) return rc;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INTERNAL_HIGH, EMC1413_REG_INTERNAL_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
