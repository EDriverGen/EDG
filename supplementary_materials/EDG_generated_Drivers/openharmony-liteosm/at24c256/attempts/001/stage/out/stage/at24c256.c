#include "at24c256.h"
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <string.h>

#include "openharmony_liteosm.h"
#define AT24C256_WRITE_CYCLE_MS 5

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return HDF_SUCCESS;
}

static int at24c256_wait_write_cycle(struct at24c256_dev *dev)
{
    uint32_t retry = 100;
    while (retry--) {
        struct I2cMsg msg;
        msg.addr = dev->i2c_addr;
        msg.buf = NULL;
        msg.len = 0;
        msg.flags = 0;
        int32_t ret = I2cTransfer((DevHandle)dev->bus_handle, &msg, 1);
        if (ret == 1) {
            return HDF_SUCCESS;
        }
        OsalMSleep(1);
    }
    return HDF_ERR_TIMEOUT;
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (addr + len > AT24C256_SIZE) {
        return HDF_ERR_INVALID_PARAM;
    }
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }
        uint8_t cmd[2];
        cmd[0] = (uint8_t)((addr + offset) >> 8);
        cmd[1] = (uint8_t)(addr + offset);
        size_t msg_count = 2;
        struct I2cMsg msgs[2];
        msgs[0].addr = dev->i2c_addr;
        msgs[0].buf = cmd;
        msgs[0].len = 2;
        msgs[0].flags = 0;
        msgs[1].addr = dev->i2c_addr;
        msgs[1].buf = (uint8_t *)(buf + offset);
        msgs[1].len = chunk;
        msgs[1].flags = 0;
        int32_t ret = I2cTransfer((DevHandle)dev->bus_handle, msgs, msg_count);
        if (ret != msg_count) {
            return HDF_FAILURE;
        }
        OsalMSleep(AT24C256_WRITE_CYCLE_MS);
        if (at24c256_wait_write_cycle(dev) != HDF_SUCCESS) {
            return HDF_ERR_TIMEOUT;
        }
        offset += chunk;
    }
    return HDF_SUCCESS;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (addr + len > AT24C256_SIZE) {
        return HDF_ERR_INVALID_PARAM;
    }
    uint8_t cmd[2];
    cmd[0] = (uint8_t)(addr >> 8);
    cmd[1] = (uint8_t)(addr);
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = cmd;
    msgs[0].len = 2;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer((DevHandle)dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
