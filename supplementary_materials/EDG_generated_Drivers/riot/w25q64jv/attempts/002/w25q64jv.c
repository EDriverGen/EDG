#include "w25q64jv.h"
#include "ztimer.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "riot.h"
#define CMD_READ_DATA      0x03
#define CMD_PAGE_PROGRAM   0x02
#define CMD_WRITE_ENABLE   0x06
#define CMD_JEDEC_ID       0x9F

#define SPI_MODE SPI_MODE_0
#define SPI_CLK  SPI_CLK_10MHZ

static int spi_write_then_read(spi_t bus, spi_cs_t cs, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    spi_acquire(bus, cs, SPI_MODE, SPI_CLK);
    int ret = spi_transfer_bytes(bus, cs, false, tx, rx, tx_len + rx_len);
    spi_release(bus);
    return ret;
}

static int spi_write(spi_t bus, spi_cs_t cs, const uint8_t *data, size_t len)
{
    spi_acquire(bus, cs, SPI_MODE, SPI_CLK);
    int ret = spi_transfer_bytes(bus, cs, false, data, NULL, len);
    spi_release(bus);
    return ret;
}

int w25q64jv_init(w25q64jv_t *dev, spi_t bus, spi_cs_t cs)
{
    dev->bus = bus;
    dev->cs = cs;

    spi_init(bus);
    spi_init_cs(bus, cs);

    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t rx[3];
    int ret = spi_write_then_read(bus, cs, &cmd, 1, rx, 3);
    if (ret < 0) {
        return ret;
    }
    if (rx[0] != 0xEF || rx[1] != 0x40 || rx[2] != 0x17) {
        return -ENODEV;
    }
    return 0;
}

int w25q64jv_read(w25q64jv_t *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) {
        return -EINVAL;
    }
    uint8_t tx[4];
    tx[0] = CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    return spi_write_then_read(dev->bus, dev->cs, tx, 4, buf, len);
}

int w25q64jv_write(w25q64jv_t *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) {
        return -EINVAL;
    }
    if (len > W25Q64JV_PAGE_SIZE) {
        return -EINVAL;
    }
    uint8_t cmd = CMD_WRITE_ENABLE;
    int ret = spi_write(dev->bus, dev->cs, &cmd, 1);
    if (ret < 0) {
        return ret;
    }
    uint8_t tx[4 + W25Q64JV_PAGE_SIZE];
    tx[0] = CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(tx + 4, buf, len);
    return spi_write(dev->bus, dev->cs, tx, 4 + len);
}
