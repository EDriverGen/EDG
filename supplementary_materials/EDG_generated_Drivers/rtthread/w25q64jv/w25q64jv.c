#include "w25q64jv.h"
#include <rtdevice.h>
#include <errno.h>

#include "rtthread.h"
#define W25Q64JV_CMD_READ_DATA  0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID   0x9F

int w25q64jv_init(struct w25q64jv_device *dev, struct rt_spi_device *spi)
{
    uint8_t cmd[1];
    uint8_t rx[3];
    int ret;

    dev->spi = spi;

    cmd[0] = W25Q64JV_CMD_JEDEC_ID;
    ret = rt_spi_transfer(spi, cmd, rx, 4);
    if (ret != 4) {
        return -EIO;
    }
    return 0;
}

int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    int ret;

    if (len == 0) return 0;

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    ret = rt_spi_transfer(dev->spi, cmd, buf, 4 + len);
    if (ret != (int)(4 + len)) {
        return -EIO;
    }
    return 0;
}

int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    uint8_t tx_buf[260];
    int ret;

    if (len == 0) return 0;
    if (len > 256) len = 256;

    cmd[0] = W25Q64JV_CMD_WRITE_ENABLE;
    ret = rt_spi_transfer(dev->spi, cmd, NULL, 1);
    if (ret != 1) {
        return -EIO;
    }

    tx_buf[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++) {
        tx_buf[4 + i] = buf[i];
    }

    ret = rt_spi_transfer(dev->spi, tx_buf, NULL, 4 + len);
    if (ret != (int)(4 + len)) {
        return -EIO;
    }

    return 0;
}
