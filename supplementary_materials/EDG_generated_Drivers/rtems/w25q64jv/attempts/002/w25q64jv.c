#include "w25q64jv.h"
#include <errno.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

int w25q64jv_init(struct w25q64jv_dev *dev, spi_bus bus)
{
    dev->bus = bus;
    uint8_t tx_buf[1] = {W25Q64JV_CMD_JEDEC_ID};
    uint8_t rx_buf[3] = {0};
    int ret = spi_write_then_read(dev->bus, tx_buf, 1, rx_buf, 3);
    if (ret != 0) {
        return -EIO;
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
    int ret = spi_write_then_read(dev->bus, cmd, 4, buf, len);
    if (ret != 0) {
        return -EIO;
    }
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (len == 0) return 0;
    if (len > 256) return -EINVAL;
    uint8_t enable_cmd[1] = {W25Q64JV_CMD_WRITE_ENABLE};
    int ret = spi_write_then_read(dev->bus, enable_cmd, 1, NULL, 0);
    if (ret != 0) {
        return -EIO;
    }
    uint8_t prog_cmd[4 + 256];
    prog_cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    prog_cmd[1] = (addr >> 16) & 0xFF;
    prog_cmd[2] = (addr >> 8) & 0xFF;
    prog_cmd[3] = addr & 0xFF;
    memcpy(&prog_cmd[4], buf, len);
    ret = spi_write_then_read(dev->bus, prog_cmd, 4 + len, NULL, 0);
    if (ret != 0) {
        return -EIO;
    }
    return 0;
}