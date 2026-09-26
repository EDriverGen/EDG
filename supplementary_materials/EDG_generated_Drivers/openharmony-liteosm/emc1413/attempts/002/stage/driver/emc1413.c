#include "emc1413.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    int32_t ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;

    ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msg;
    uint8_t buf[2] = {reg, data};
    int32_t ret;

    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;

    ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int32_t raw;
    int ret;

    ret = emc1413_write_then_read(dev, reg_high, &high_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = emc1413_write_then_read(dev, reg_low, &low_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    raw = ((int32_t)high_byte * 1000) + (((int32_t)(low_byte >> 5) & 0x07) * 125);
    *temp = raw;
    return HDF_SUCCESS;
}

int emc1413_init(struct emc1413_dev *dev, DevHandle bus_handle)
{
    uint8_t id;
    int ret;

    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    OsalMSleep(15);

    ret = emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, &id, 1);
    if (ret != HDF_SUCCESS || id != 0x5D) {
        return HDF_FAILURE;
    }

    ret = emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, &id, 1);
    if (ret != HDF_SUCCESS || id != 0x21) {
        return HDF_FAILURE;
    }

    ret = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    return HDF_SUCCESS;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
