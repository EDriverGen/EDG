#include "w25q64jv.h"
#include <rtdevice.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include "rtthread.h"
#define W25Q64JV_CMD_READ_DATA      0x03
#define W25Q64JV_CMD_PAGE_PROGRAM   0x02
#define W25Q64JV_CMD_WRITE_ENABLE   0x06
#define W25Q64JV_CMD_JEDEC_ID       0x9F

static int w25q64jv_transfer(struct w25q64jv_device *dev, const void *tx, void *rx, size_t len)
{
    rt_ssize_t ret = rt_spi_transfer(dev->spi, tx, rx, len);
    if (ret != len)
        return -EIO;
    return 0;
}

int w25q64jv_init(struct w25q64jv_device *dev, struct rt_spi_device *spi)
{
    dev->spi = spi;
    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx[3];
    int ret = w25q64jv_transfer(dev, &cmd, rx, 1);
    if (ret)
        return ret;
    ret = w25q64jv_transfer(dev, NULL, rx, 3);
    if (ret)
        return ret;
    return 0;
}

int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, void *buf, size_t len)
{
    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    int ret = w25q64jv_transfer(dev, cmd, NULL, 4);
    if (ret)
        return ret;
    ret = w25q64jv_transfer(dev, NULL, buf, len);
    if (ret)
        return ret;
    return 0;
}

int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const void *buf, size_t len)
{
    uint8_t cmd = W25Q64JV_CMD_WRITE_ENABLE;
    int ret = w25q64jv_transfer(dev, &cmd, NULL, 1);
    if (ret)
        return ret;
    uint8_t *tx_buf = (uint8_t *)buf;
    size_t total_len = 4 + len;
    uint8_t *tx = (uint8_t *)malloc(total_len);
    if (!tx)
        return -ENOMEM;
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++)
        tx[4 + i] = tx_buf[i];
    ret = w25q64jv_transfer(dev, tx, NULL, total_len);
    free(tx);
    return ret;
}
