#include "mcp3008_ref.h"

#include <dev/spi/spi.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int mcp3008_init(struct mcp3008_device *dev, const char *spi_path, uint16_t vref_mv)
{
    int fd;
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = MCP3008_SPI_MAX_HZ;

    if (dev == 0 || spi_path == 0) {
        return -1;
    }
    fd = open(spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    (void)ioctl(fd, SPI_IOC_WR_MODE, &mode);
    (void)ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    (void)ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    (void)close(fd);
    dev->spi_path = spi_path;
    dev->vref_mv = vref_mv;
    return 0;
}

int mcp3008_read_raw(struct mcp3008_device *dev, uint8_t channel, uint8_t single, uint16_t *raw)
{
    struct spi_ioc_transfer msg;
    uint8_t tx[3];
    uint8_t rx[3];
    int fd;

    if (dev == 0 || dev->spi_path == 0 || raw == 0) {
        return -1;
    }
    if (channel >= MCP3008_CHANNELS) {
        return -1;
    }

    tx[0] = 0x01;
    tx[1] = (uint8_t)((single ? 0x80U : 0x00U) | ((channel & 0x07U) << 4));
    tx[2] = 0x00;
    fd = open(dev->spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }

    msg.tx_buf = (uint64_t)(uintptr_t)tx;
    msg.rx_buf = (uint64_t)(uintptr_t)rx;
    msg.len = 3;
    msg.speed_hz = MCP3008_SPI_MAX_HZ;
    msg.delay_usecs = 0;
    msg.bits_per_word = 8;
    msg.cs_change = 0;
    msg.mode = SPI_MODE_0;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &msg) != 0) {
        (void)close(fd);
        return -1;
    }
    (void)close(fd);
    *raw = (uint16_t)(((uint16_t)(rx[1] & 0x03U) << 8) | rx[2]);
    return 0;
}

int mcp3008_read_voltage(struct mcp3008_device *dev, uint8_t channel, uint16_t *mv)
{
    uint16_t raw = 0;
    if (dev == 0 || mv == 0) {
        return -1;
    }
    if (mcp3008_read_raw(dev, channel, MCP3008_SINGLE, &raw) != 0) {
        return -1;
    }
    return mcp3008_to_millivolts(raw, dev->vref_mv, mv);
}

int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv)
{
    if (mv == 0) {
        return -1;
    }
    *mv = (uint16_t)(((uint32_t)raw * vref_mv) / MCP3008_MAX_VALUE);
    return 0;
}
