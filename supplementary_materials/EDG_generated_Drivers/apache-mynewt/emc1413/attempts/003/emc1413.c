#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>

#include "apache_mynewt.h"
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    struct hal_i2c_master_data data;
    int rc;

    data.address = dev->i2c_addr;
    data.buffer = &reg;
    data.len = 1;
    rc = hal_i2c_master_write(dev->bus_num, &data, OS_TICK_PER_SECOND / 10, 1);
    if (rc != 0) return rc;

    data.buffer = buf;
    data.len = len;
    rc = hal_i2c_master_read(dev->bus_num, &data, OS_TICK_PER_SECOND / 10, 1);
    return rc;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data data;
    uint8_t buf[2] = {reg, val};
    data.address = dev->i2c_addr;
    data.buffer = buf;
    data.len = 2;
    return hal_i2c_master_write(dev->bus_num, &data, OS_TICK_PER_SECOND / 10, 1);
}

static int emc1413_read_temp(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high, low;
    int rc;

    rc = emc1413_write_then_read(dev, reg_high, &high, 1);
    if (rc != 0) return rc;

    rc = emc1413_write_then_read(dev, reg_low, &low, 1);
    if (rc != 0) return rc;

    *temp = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    uint8_t buf[1];
    int rc;

    dev->i2c_addr = EMC1413_I2C_ADDR;
    dev->bus_num = 0;

    os_time_delay(OS_TICK_PER_SECOND * 15 / 1000);

    /* Probe: read Manufacturer ID */
    rc = emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, buf, 1);
    if (rc != 0 || buf[0] != 0x5D) return -1;

    /* Probe: read Product ID */
    rc = emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, buf, 1);
    if (rc != 0 || buf[0] != 0x21) return -1;

    /* Write Configuration */
    rc = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (rc != 0) return rc;

    /* Write Conversion Rate */
    rc = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (rc != 0) return rc;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temp(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp);
}
