#include "w25q64jv.h"
#include <string.h>
#include "hdf_base.h"

#include "spi_if.h"
#define W25Q64JV_CMD_READ 0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID 0x9F

int w25q64jv_init(struct w25q64jv_dev *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->spi_handle = bus_handle;

    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx_buf[3];
    struct SpiMsg msg;
    msg.wbuf = &cmd;
    msg.rbuf = rx_buf;
    msg.len = 1;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 1;

    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    msg.wbuf = NULL;
    msg.rbuf = rx_buf;
    msg.len = 3;
    msg.csChange = 1;
    ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (dev == NULL || buf == NULL || len == 0) {
        return HDF_ERR_INVALID_PARAM;
    }

    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    struct SpiMsg msg;
    msg.wbuf = cmd;
    msg.rbuf = NULL;
    msg.len = 4;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 1;

    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    msg.wbuf = NULL;
    msg.rbuf = buf;
    msg.len = len;
    msg.csChange = 1;
    ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, uint32_t len)
{
    if (dev == NULL || buf == NULL || len == 0 || len > W25Q64JV_PAGE_SIZE) {
        return HDF_ERR_INVALID_PARAM;
    }

    uint8_t cmd_wren = W25Q64JV_CMD_WRITE_ENABLE;
    struct SpiMsg msg;
    msg.wbuf = &cmd_wren;
    msg.rbuf = NULL;
    msg.len = 1;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 1;
    msg.keepCs = 0;

    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    uint8_t cmd_prog[4 + W25Q64JV_PAGE_SIZE];
    cmd_prog[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd_prog[1] = (addr >> 16) & 0xFF;
    cmd_prog[2] = (addr >> 8) & 0xFF;
    cmd_prog[3] = addr & 0xFF;
    memcpy(cmd_prog + 4, buf, len);

    msg.wbuf = cmd_prog;
    msg.rbuf = NULL;
    msg.len = 4 + len;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 1;
    msg.keepCs = 0;

    ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}
