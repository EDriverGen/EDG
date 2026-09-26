#include "w25q64jv.h"
#include "ztimer.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "riot.h"
#define CMD_READ_DATA 0x03
#define CMD_PAGE_PROGRAM 0x02
#define CMD_WRITE_ENABLE 0x06
#define CMD_JEDEC_ID 0x9F
#define CMD_READ_STATUS_1 0x05

#define STATUS_BUSY 0x01

#define SPI_MODE SPI_MODE_0
#define SPI_CLK SPI_CLK_10MHZ

static int w25q64jv_write_enable(w25q64jv_t *dev)
{
    uint8_t cmd = CMD_WRITE_ENABLE;
    spi_acquire(dev->bus, dev->cs, SPI_MODE, SPI_CLK);
    spi_transfer_bytes(dev->bus, dev->cs, false, &cmd, NULL, 1);
    spi_release(dev->bus);
    return 0;
}

static int w25q64jv_wait_busy(w25q64jv_t *dev)
{
    uint8_t cmd = CMD_READ_STATUS_1;
    uint8_t status;
    int timeout = 1000;
    do {
        spi_acquire(dev->bus, dev->cs, SPI_MODE, SPI_CLK);
        spi_transfer_bytes(dev->bus, dev->cs, false, &cmd, &status, 1);
        spi_release(dev->bus);
        if (!(status & STATUS_BUSY)) return 0;
        ztimer_sleep(ZTIMER_MSEC, 1);
    } while (--timeout > 0);
    return -ETIMEDOUT;
}

int w25q64jv_init(w25q64jv_t *dev, spi_t bus, spi_cs_t cs)
{
    dev->bus = bus;
    dev->cs = cs;

    spi_init(bus);
    spi_init_cs(bus, cs);

    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t buf[3];
    spi_acquire(bus, cs, SPI_MODE, SPI_CLK);
    spi_transfer_bytes(bus, cs, false, &cmd, NULL, 1);
    spi_transfer_bytes(bus, cs, false, NULL, buf, 3);
    spi_release(bus);

    if (buf[0] != 0xEF || buf[1] != 0x40 || buf[2] != 0x17) {
        return -ENODEV;
    }

    return 0;
}

int w25q64jv_read(w25q64jv_t *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;

    uint8_t header[4];
    header[0] = CMD_READ_DATA;
    header[1] = (addr >> 16) & 0xFF;
    header[2] = (addr >> 8) & 0xFF;
    header[3] = addr & 0xFF;

    spi_acquire(dev->bus, dev->cs, SPI_MODE, SPI_CLK);
    spi_transfer_bytes(dev->bus, dev->cs, false, header, NULL, 4);
    spi_transfer_bytes(dev->bus, dev->cs, false, NULL, buf, len);
    spi_release(dev->bus);

    return 0;
}

int w25q64jv_write(w25q64jv_t *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;
    if (len > W25Q64JV_PAGE_SIZE) return -EINVAL;

    int ret = w25q64jv_write_enable(dev);
    if (ret) return ret;

    uint8_t header[4];
    header[0] = CMD_PAGE_PROGRAM;
    header[1] = (addr >> 16) & 0xFF;
    header[2] = (addr >> 8) & 0xFF;
    header[3] = addr & 0xFF;

    spi_acquire(dev->bus, dev->cs, SPI_MODE, SPI_CLK);
    spi_transfer_bytes(dev->bus, dev->cs, false, header, NULL, 4);
    spi_transfer_bytes(dev->bus, dev->cs, false, buf, NULL, len);
    spi_release(dev->bus);

    ret = w25q64jv_wait_busy(dev);
    return ret;
}
