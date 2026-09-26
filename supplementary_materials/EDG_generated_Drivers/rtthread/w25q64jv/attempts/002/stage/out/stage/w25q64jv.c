#include "w25q64jv.h"
#include <rtdevice.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include "rtthread.h"
#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_CMD_READ 0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID 0x9F

static int w25q64jv_transfer(struct w25q64jv_device *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    rt_ssize_t ret = rt_spi_transfer(dev->spi, tx, rx, len);
    if (ret != len)
        return -EIO;
    return 0;
}

int w25q64jv_init(struct w25q64jv_device *dev, struct rt_spi_device *spi)
{
    if (!dev || !spi)
        return -EINVAL;
    dev->spi = spi;

    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx[4] = {0};
    int ret = w25q64jv_transfer(dev, &cmd, rx, 4);
    if (ret)
        return ret;
    return 0;
}

int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0)
        return -EINVAL;
    if (addr + len > 8388608)
        return -EINVAL;

    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;

    int ret = w25q64jv_transfer(dev, tx, NULL, 4);
    if (ret)
        return ret;

    ret = w25q64jv_transfer(dev, NULL, buf, len);
    if (ret)
        return ret;

    return 0;
}

int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0)
        return -EINVAL;
    if (addr + len > 8388608)
        return -EINVAL;
    if (len > W25Q64JV_PAGE_SIZE)
        return -EINVAL;

    uint8_t cmd = W25Q64JV_CMD_WRITE_ENABLE;
    int ret = w25q64jv_transfer(dev, &cmd, NULL, 1);
    if (ret)
        return ret;

    uint8_t tx[4 + W25Q64JV_PAGE_SIZE];
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    for (size_t i = 0; i < len; i++)
        tx[4 + i] = buf[i];

    ret = w25q64jv_transfer(dev, tx, NULL, 4 + len);
    if (ret)
        return ret;

    return 0;
}
