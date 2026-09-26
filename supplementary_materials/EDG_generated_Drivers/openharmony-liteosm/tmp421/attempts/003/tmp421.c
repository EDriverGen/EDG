#include "tmp421.h"
#include "osal_time.h"
#include "hdf_base.h"
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
    uint8_t reg_buf = reg;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg_buf;
    msgs[0].len = 1;
    msgs[0].flags = 0;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;

    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int32_t ret;
    uint32_t timeout = 0;
    const uint32_t max_polls = 130;

    do {
        ret = tmp421_write_then_read(dev, TMP421_REG_STATUS, &status, 1);
        if (ret != HDF_SUCCESS) {
            return ret;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return HDF_SUCCESS;
        }
        OsalMSleep(1);
        timeout++;
    } while (timeout < max_polls);

    return HDF_ERR_TIMEOUT;
}

int32_t tmp421_init(struct tmp421_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    uint8_t buf;
    int32_t ret;

    ret = tmp421_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &buf, 1);
    if (ret != HDF_SUCCESS || buf != 0x55) {
        return HDF_ERR_DEVICE_ERROR;
    }

    ret = tmp421_write_then_read(dev, TMP421_REG_DEVICE_ID, &buf, 1);
    if (ret != HDF_SUCCESS || buf != 0x21) {
        return HDF_ERR_DEVICE_ERROR;
    }

    return HDF_SUCCESS;
}

int32_t tmp421_read_temp_local(struct tmp421_device *dev, int32_t *temp_local)
{
    if (dev == NULL || temp_local == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    int32_t ret = tmp421_poll_busy(dev);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    uint8_t high_byte;
    uint8_t low_byte;

    ret = tmp421_write_then_read(dev, TMP421_REG_LOCAL_HIGH, &high_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = tmp421_write_then_read(dev, TMP421_REG_LOCAL_LOW, &low_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | ((low_byte >> 4) & 0x0F);
    *temp_local = ((int32_t)raw * 625) / 10;

    return HDF_SUCCESS;
}

int32_t tmp421_read_temp_remote(struct tmp421_device *dev, int32_t *temp_remote)
{
    if (dev == NULL || temp_remote == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    int32_t ret = tmp421_poll_busy(dev);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    uint8_t high_byte;
    uint8_t low_byte;

    ret = tmp421_write_then_read(dev, TMP421_REG_REMOTE_HIGH, &high_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = tmp421_write_then_read(dev, TMP421_REG_REMOTE_LOW, &low_byte, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | ((low_byte >> 4) & 0x0F);
    *temp_remote = ((int32_t)raw * 625) / 10;

    return HDF_SUCCESS;
}
