#include "tmp421.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>
#include <string.h>

#include "openharmony_liteosm.h"
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE_HIGH 0x01
#define TMP421_REG_REMOTE_LOW 0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_DELAY_MS 130

static int32_t tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    int32_t ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = I2C_FLAG_READ;
    msgs[1].buf = buf;
    msgs[1].len = len;

    ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t tmp421_write(struct tmp421_device *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msg;
    uint8_t buf[2];
    int32_t ret;

    buf[0] = reg;
    buf[1] = data;
    msg.addr = dev->i2c_addr;
    msg.flags = 0;
    msg.buf = buf;
    msg.len = 2;

    ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int32_t ret;
    int32_t timeout = 10;

    while (timeout--) {
        ret = tmp421_write_then_read(dev, TMP421_REG_STATUS, &status, 1);
        if (ret != HDF_SUCCESS) {
            return HDF_FAILURE;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return HDF_SUCCESS;
        }
        OsalMSleep(15);
    }
    return HDF_FAILURE;
}

static int32_t tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte;
    uint8_t low_byte;
    int32_t ret;
    int16_t raw;

    ret = tmp421_poll_busy(dev);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    ret = tmp421_write_then_read(dev, high_reg, &high_byte, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    ret = tmp421_write_then_read(dev, low_reg, &low_byte, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp = ((int32_t)raw * 625) / 10;
    return HDF_SUCCESS;
}

int32_t tmp421_init(struct tmp421_device *dev, DevHandle bus_handle)
{
    uint8_t id;
    int32_t ret;

    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    ret = tmp421_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &id, 1);
    if (ret != HDF_SUCCESS || id != 0x55) {
        return HDF_FAILURE;
    }

    ret = tmp421_write_then_read(dev, TMP421_REG_DEVICE_ID, &id, 1);
    if (ret != HDF_SUCCESS || id != 0x21) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t tmp421_read_temp_local(struct tmp421_device *dev, int32_t *temp_local)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local);
}

int32_t tmp421_read_temp_remote(struct tmp421_device *dev, int32_t *temp_remote)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE_HIGH, TMP421_REG_REMOTE_LOW, temp_remote);
}
