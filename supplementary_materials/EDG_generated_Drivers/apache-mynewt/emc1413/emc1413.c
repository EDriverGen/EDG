#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>

#include "apache_mynewt.h"
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->addr;
    pdata.buffer = &reg;
    pdata.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    if (rc) return rc;

    pdata.buffer = buf;
    pdata.len = len;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    return rc;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data pdata;
    uint8_t buf[2] = {reg, val};
    pdata.address = dev->addr;
    pdata.buffer = buf;
    pdata.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
}

static int emc1413_read_byte(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    return emc1413_write_then_read(dev, reg, val, 1);
}

static int32_t emc1413_raw_to_millideg(uint8_t high, uint8_t low)
{
    int32_t temp = (int32_t)high * 1000;
    temp += (int32_t)((low >> 5) & 0x07) * 125;
    return temp;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    uint8_t val;
    int rc;

    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->addr = EMC1413_I2C_ADDR;

    os_time_delay(OS_TICKS_PER_SEC * 15 / 1000);

    rc = emc1413_read_byte(dev, EMC1413_REG_MANUFACTURER_ID, &val);
    if (rc) return rc;
    if (val != 0x5D) return -1;

    rc = emc1413_read_byte(dev, EMC1413_REG_PRODUCT_ID, &val);
    if (rc) return rc;
    if (val != 0x21) return -1;

    rc = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (rc) return rc;

    rc = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (rc) return rc;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    uint8_t high, low;
    int rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_INT_HIGH, &high);
    if (rc) return rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_INT_LOW, &low);
    if (rc) return rc;

    *temp_local_val = emc1413_raw_to_millideg(high, low);
    return 0;
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    uint8_t high, low;
    int rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_EXT1_HIGH, &high);
    if (rc) return rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_EXT1_LOW, &low);
    if (rc) return rc;

    *temp_ext1_val = emc1413_raw_to_millideg(high, low);
    return 0;
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    uint8_t high, low;
    int rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_EXT2_HIGH, &high);
    if (rc) return rc;

    rc = emc1413_read_byte(dev, EMC1413_REG_EXT2_LOW, &low);
    if (rc) return rc;

    *temp_ext2_val = emc1413_raw_to_millideg(high, low);
    return 0;
}
