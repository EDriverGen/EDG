#include "w25q64jv.h"
#include "spi_if.h"
#include "hdf_base.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA 0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID 0x9F
#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_TIMEOUT_MS 1000

static int spi_write_then_read(struct w25q64jv_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct SpiMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.wbuf = tx;
    msg.rbuf = rx;
    msg.len = tx_len + rx_len;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 1;
    int32_t ret = SpiTransfer(dev->bus_handle, &msg, 1);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t tx[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx[3];
    int ret = spi_write_then_read(dev, tx, 1, rx, 3);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    return spi_write_then_read(dev, tx, 4, buf, len);
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    if (len > W25Q64JV_PAGE_SIZE) len = W25Q64JV_PAGE_SIZE;
    uint8_t tx[4 + W25Q64JV_PAGE_SIZE];
    tx[0] = W25Q64JV_CMD_WRITE_ENABLE;
    struct SpiMsg msg_wren;
    memset(&msg_wren, 0, sizeof(msg_wren));
    msg_wren.wbuf = tx;
    msg_wren.rbuf = NULL;
    msg_wren.len = 1;
    msg_wren.speed = 1000000;
    msg_wren.delayUs = 0;
    msg_wren.csChange = 0;
    msg_wren.keepCs = 0;
    int32_t ret = SpiTransfer(dev->bus_handle, &msg_wren, 1);
    if (ret != 0) return -1;
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(tx + 4, buf, len);
    struct SpiMsg msg_pp;
    memset(&msg_pp, 0, sizeof(msg_pp));
    msg_pp.wbuf = tx;
    msg_pp.rbuf = NULL;
    msg_pp.len = 4 + len;
    msg_pp.speed = 1000000;
    msg_pp.delayUs = 0;
    msg_pp.csChange = 0;
    msg_pp.keepCs = 0;
    ret = SpiTransfer(dev->bus_handle, &msg_pp, 1);
    if (ret != 0) return -1;
    return 0;
}