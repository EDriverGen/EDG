#include "w25q64jv.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "ztimer.h"

#include "riot.h"
#define CMD_READ_DATA      0x03
#define CMD_PAGE_PROGRAM   0x02
#define CMD_WRITE_ENABLE   0x06
#define CMD_JEDEC_ID       0x9F

static int w25q64jv_transfer(w25q64jv_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx, rx, len);
    spi_release(dev->bus);
    return 0;
}

int w25q64jv_init(w25q64jv_t *dev, spi_t bus, spi_cs_t cs)
{
    dev->bus = bus;
    dev->cs = cs;

    spi_init(bus);
    spi_init_cs(bus, cs);

    /* Wait for power-up */
    ztimer_sleep(ZTIMER_MSEC, 5);

    /* Send JEDEC ID command and read 3 bytes */
    uint8_t tx_buf[4] = {CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx_buf[4] = {0};
    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx_buf, rx_buf, 4);
    spi_release(dev->bus);

    /* Check JEDEC ID: expected 0xEF 0x40 0x17 for W25Q64JV */
    if (rx_buf[1] != 0xEF || rx_buf[2] != 0x40 || rx_buf[3] != 0x17) {
        return -ENODEV;
    }

    return 0;
}

int w25q64jv_read(w25q64jv_t *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEM_SIZE) {
        return -EINVAL;
    }

    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, cmd, NULL, 4);
    spi_transfer_bytes(dev->bus, dev->cs, false, NULL, buf, len);
    spi_release(dev->bus);

    return 0;
}

int w25q64jv_write(w25q64jv_t *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEM_SIZE) {
        return -EINVAL;
    }
    if (len > W25Q64JV_PAGE_SIZE) {
        return -EINVAL;
    }

    /* Write enable */
    uint8_t we_cmd = CMD_WRITE_ENABLE;
    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, &we_cmd, NULL, 1);
    spi_release(dev->bus);

    /* Page program */
    uint8_t cmd[4];
    cmd[0] = CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, cmd, NULL, 4);
    spi_transfer_bytes(dev->bus, dev->cs, false, buf, NULL, len);
    spi_release(dev->bus);

    /* Wait for program to complete (poll BUSY) */
    uint8_t status_cmd = 0x05;
    uint8_t status;
    do {
        spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_10MHZ);
        spi_transfer_bytes(dev->bus, dev->cs, false, &status_cmd, NULL, 1);
        spi_transfer_bytes(dev->bus, dev->cs, false, NULL, &status, 1);
        spi_release(dev->bus);
    } while (status & 0x01);

    return 0;
}
