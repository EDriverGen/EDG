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

static int w25q64jv_wait_busy(struct w25q64jv_dev_s *dev)
{
    uint8_t cmd = 0x05;
    uint8_t status;
    int ret;
    unsigned int timeout = W25Q64JV_TIMEOUT_MS;

    do {
        SPI_SELECT(dev->spi, 0, true);
        SPI_SEND(dev->spi, cmd);
        status = SPI_SEND(dev->spi, 0);
        SPI_SELECT(dev->spi, 0, false);
        if (!(status & 0x01)) {
            return 0;
        }
        up_mdelay(1);
    } while (--timeout > 0);

    return -ETIMEDOUT;
}

int w25q64jv_init(struct w25q64jv_dev_s *dev, struct spi_dev_s *spi)
{
    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rxbuf[3];
    int ret;

    dev->spi = spi;

    SPI_SELECT(dev->spi, 0, true);
    SPI_SEND(dev->spi, cmd);
    rxbuf[0] = SPI_SEND(dev->spi, 0);
    rxbuf[1] = SPI_SEND(dev->spi, 0);
    rxbuf[2] = SPI_SEND(dev->spi, 0);
    SPI_SELECT(dev->spi, 0, false);

    if (rxbuf[0] != 0xEF || rxbuf[1] != 0x40 || rxbuf[2] != 0x17) {
        return -ENODEV;
    }

    return 0;
}

int w25q64jv_read(struct w25q64jv_dev_s *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    int ret;

    if (len == 0) {
        return 0;
    }

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    SPI_SELECT(dev->spi, 0, true);
    SPI_SEND(dev->spi, cmd[0]);
    SPI_SEND(dev->spi, cmd[1]);
    SPI_SEND(dev->spi, cmd[2]);
    SPI_SEND(dev->spi, cmd[3]);
    for (size_t i = 0; i < len; i++) {
        buf[i] = SPI_SEND(dev->spi, 0);
    }
    SPI_SELECT(dev->spi, 0, false);

    return 0;
}

int w25q64jv_write(struct w25q64jv_dev_s *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    size_t offset = 0;
    int ret;

    while (offset < len) {
        size_t page_offset = addr & (W25Q64JV_PAGE_SIZE - 1);
        size_t chunk = W25Q64JV_PAGE_SIZE - page_offset;
        if (chunk > len - offset) {
            chunk = len - offset;
        }

        SPI_SELECT(dev->spi, 0, true);
        SPI_SEND(dev->spi, W25Q64JV_CMD_WRITE_ENABLE);
        SPI_SELECT(dev->spi, 0, false);

        SPI_SELECT(dev->spi, 0, true);
        SPI_SEND(dev->spi, W25Q64JV_CMD_PAGE_PROGRAM);
        SPI_SEND(dev->spi, (addr >> 16) & 0xFF);
        SPI_SEND(dev->spi, (addr >> 8) & 0xFF);
        SPI_SEND(dev->spi, addr & 0xFF);
        for (size_t i = 0; i < chunk; i++) {
            SPI_SEND(dev->spi, buf[offset + i]);
        }
        SPI_SELECT(dev->spi, 0, false);

        ret = w25q64jv_wait_busy(dev);
        if (ret != 0) {
            return ret;
        }

        offset += chunk;
        addr += chunk;
    }

    return 0;
}
