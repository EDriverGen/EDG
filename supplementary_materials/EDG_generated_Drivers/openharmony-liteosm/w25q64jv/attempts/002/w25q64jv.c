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
#define W25Q64JV_DUMMY 0x00

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx[3];
    struct SpiMsg msg[2];
    msg[0].wbuf = &cmd;
    msg[0].rbuf = NULL;
    msg[0].len = 1;
    msg[0].speed = 1000000;
    msg[0].delayUs = 0;
    msg[0].csChange = 0;
    msg[0].keepCs = 1;
    msg[1].wbuf = NULL;
    msg[1].rbuf = rx;
    msg[1].len = 3;
    msg[1].speed = 1000000;
    msg[1].delayUs = 0;
    msg[1].csChange = 1;
    msg[1].keepCs = 0;
    int32_t ret = SpiTransfer(dev->bus_handle, msg, 2);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    struct SpiMsg msg[2];
    msg[0].wbuf = cmd;
    msg[0].rbuf = NULL;
    msg[0].len = 4;
    msg[0].speed = 1000000;
    msg[0].delayUs = 0;
    msg[0].csChange = 0;
    msg[0].keepCs = 1;
    msg[1].wbuf = NULL;
    msg[1].rbuf = buf;
    msg[1].len = len;
    msg[1].speed = 1000000;
    msg[1].delayUs = 0;
    msg[1].csChange = 1;
    msg[1].keepCs = 0;
    int32_t ret = SpiTransfer(dev->bus_handle, msg, 2);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    if (len > 256) len = 256;
    uint8_t wren_cmd = W25Q64JV_CMD_WRITE_ENABLE;
    struct SpiMsg wren_msg;
    wren_msg.wbuf = &wren_cmd;
    wren_msg.rbuf = NULL;
    wren_msg.len = 1;
    wren_msg.speed = 1000000;
    wren_msg.delayUs = 0;
    wren_msg.csChange = 1;
    wren_msg.keepCs = 0;
    int32_t ret = SpiTransfer(dev->bus_handle, &wren_msg, 1);
    if (ret != 0) {
        return -1;
    }
    uint8_t pp_cmd[4 + 256];
    pp_cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    pp_cmd[1] = (addr >> 16) & 0xFF;
    pp_cmd[2] = (addr >> 8) & 0xFF;
    pp_cmd[3] = addr & 0xFF;
    memcpy(pp_cmd + 4, buf, len);
    struct SpiMsg pp_msg;
    pp_msg.wbuf = pp_cmd;
    pp_msg.rbuf = NULL;
    pp_msg.len = 4 + len;
    pp_msg.speed = 1000000;
    pp_msg.delayUs = 0;
    pp_msg.csChange = 1;
    pp_msg.keepCs = 0;
    ret = SpiTransfer(dev->bus_handle, &pp_msg, 1);
    if (ret != 0) {
        return -1;
    }
    return 0;
}