#include "w25q64jv.h"
#include <errno.h>
#include <string.h>

#define CMD_READ_DATA 0x03
#define CMD_PAGE_PROGRAM 0x02
#define CMD_WRITE_ENABLE 0x06
#define CMD_JEDEC_ID 0x9F

static int spi_write_then_read(spi_bus bus, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    /* Use full-duplex transfer: send tx, then read rx in same transaction */
    size_t total_len = tx_len + rx_len;
    uint8_t tx_buf[total_len];
    uint8_t rx_buf[total_len];
    memcpy(tx_buf, tx, tx_len);
    memset(tx_buf + tx_len, 0, rx_len);
    /* Perform transfer (simplified: assume spi_bus_transfer exists) */
    /* For RTEMS, we might use ioctl; here we assume a helper */
    /* Placeholder: actual implementation would use SPI_IOC_MESSAGE */
    (void)bus;
    (void)tx_buf;
    (void)rx_buf;
    if (rx) {
        memcpy(rx, rx_buf + tx_len, rx_len);
    }
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, spi_bus bus)
{
    dev->bus = bus;
    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t rx[3];
    int ret = spi_write_then_read(dev->bus, &cmd, 1, rx, 3);
    if (ret != 0) return ret;
    /* Optionally verify JEDEC ID: 0xEF 0x40 0x17 */
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;
    uint8_t tx[4];
    tx[0] = CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    return spi_write_then_read(dev->bus, tx, 4, buf, len);
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;
    if (len > W25Q64JV_PAGE_SIZE) return -EINVAL;
    /* Write enable */
    uint8_t we_cmd = CMD_WRITE_ENABLE;
    int ret = spi_write_then_read(dev->bus, &we_cmd, 1, NULL, 0);
    if (ret != 0) return ret;
    /* Page program */
    size_t total_len = 4 + len;
    uint8_t tx[total_len];
    tx[0] = CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(tx + 4, buf, len);
    return spi_write_then_read(dev->bus, tx, total_len, NULL, 0);
}