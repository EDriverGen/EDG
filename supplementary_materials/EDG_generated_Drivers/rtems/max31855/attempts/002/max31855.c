#include "max31855.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <linux/spi/spidev.h>
#define MAX31855_FAULT_BIT (1 << 16)

static int spi_transfer(int fd, uint8_t *tx, uint8_t *rx, uint32_t len)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = 1000000,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    return ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

int max31855_init(struct max31855_device *dev, int bus_handle)
{
    dev->fd = bus_handle;
    usleep(200000);
    return 0;
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint8_t tx[4] = {0};
    uint8_t rx[4] = {0};
    int ret = spi_transfer(dev->fd, tx, rx, 4);
    if (ret < 0) {
        return -1;
    }

    uint32_t raw = ((uint32_t)rx[0] << 24) | ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];

    if (raw & MAX31855_FAULT_BIT) {
        return 1;
    }

    uint16_t raw_14 = (raw >> 18) & 0x3FFF;
    int32_t sign_ext = (raw_14 & 0x2000) ? (raw_14 | 0xFFFFC000) : raw_14;
    *thermocouple_val = (sign_ext * 250) / 1000;

    uint16_t raw_12 = (raw >> 4) & 0xFFF;
    sign_ext = (raw_12 & 0x800) ? (raw_12 | 0xFFFFF000) : raw_12;
    *temp_local_val = (sign_ext * 625) / 10000;

    return 0;
}
