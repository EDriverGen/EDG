#include "w25q64jv.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "arch.h"
#include "nuttx.h"

#include <nuttx/spi/spi.h>
#define W25Q64JV_CMD_READ_DATA     0x03
#define W25Q64JV_CMD_PAGE_PROGRAM  0x02
#define W25Q64JV_CMD_WRITE_ENABLE  0x06
#define W25Q64JV_CMD_JEDEC_ID      0x9F

#define W25Q64JV_PAGE_SIZE         256
#define W25Q64JV_TIMEOUT_MS        1000

static int w25q64jv_transfer(struct w25q64jv_dev_s *dev, const uint8_t *txbuf, uint8_t *rxbuf, size_t len)
{
    struct spi_dev_s *spi = dev->spi;
    if (!spi) return -ENODEV;
    SPI_LOCK(spi, true);
    SPI_SELECT(spi, 0, true);
    SPI_EXCHANGE(spi, txbuf, rxbuf, len);
    SPI_SELECT(spi, 0, false);
    SPI_LOCK(spi, false);
    return OK;
}

static int w25q64jv_write_enable(struct w25q64jv_dev_s *dev)
{
    uint8_t cmd = W25Q64JV_CMD_WRITE_ENABLE;
    return w25q64jv_transfer(dev, &cmd, NULL, 1);
}

static int w25q64jv_poll_busy(struct w25q64jv_dev_s *dev)
{
    uint8_t cmd = 0x05;
    uint8_t status;
    int retries = W25Q64JV_TIMEOUT_MS;
    do {
        uint8_t tx[2] = {cmd, 0};
        uint8_t rx[2];
        int ret = w25q64jv_transfer(dev, tx, rx, 2);
        if (ret < 0) return ret;
        status = rx[1];
        if (!(status & 0x01)) return 0;
        up_mdelay(1);
    } while (--retries > 0);
    return -ETIMEDOUT;
}

int w25q64jv_init(struct w25q64jv_dev_s *dev, struct spi_dev_s *spi)
{
    if (!dev || !spi) return -EINVAL;
    dev->spi = spi;

    uint8_t tx[4] = {W25Q64JV_CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4];
    int ret = w25q64jv_transfer(dev, tx, rx, 4);
    if (ret < 0) return ret;
    if (rx[1] != 0xEF || rx[2] != 0x40 || rx[3] != 0x17) {
        return -ENODEV;
    }
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev_s *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > 8388608) return -EINVAL;

    uint8_t cmd[4];
    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    struct spi_dev_s *spi = dev->spi;
    SPI_LOCK(spi, true);
    SPI_SELECT(spi, 0, true);
    SPI_EXCHANGE(spi, cmd, NULL, 4);
    SPI_EXCHANGE(spi, NULL, buf, len);
    SPI_SELECT(spi, 0, false);
    SPI_LOCK(spi, false);
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev_s *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > 8388608) return -EINVAL;
    if (len > W25Q64JV_PAGE_SIZE) return -EINVAL;

    int ret = w25q64jv_write_enable(dev);
    if (ret < 0) return ret;

    uint8_t cmd[4 + W25Q64JV_PAGE_SIZE];
    cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    memcpy(cmd + 4, buf, len);

    ret = w25q64jv_transfer(dev, cmd, NULL, 4 + len);
    if (ret < 0) return ret;

    ret = w25q64jv_poll_busy(dev);
    if (ret < 0) return ret;

    return 0;
}
