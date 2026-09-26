#include "max31855.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <linux/spi/spidev.h>
#define MAX31855_SPI_MODE SPI_MODE_0
#define MAX31855_SPI_BITS 8
#define MAX31855_SPI_SPEED 1000000

int max31855_init(struct max31855_device *dev, int bus_handle)
{
    (void)bus_handle;
    dev->fd = -1;
    return 0;
}

static int max31855_spi_transfer(struct max31855_device *dev, uint8_t *rx, size_t len)
{
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = 0;
    tr.rx_buf = (unsigned long)rx;
    tr.len = len;
    tr.speed_hz = MAX31855_SPI_SPEED;
    tr.delay_usecs = 0;
    tr.bits_per_word = MAX31855_SPI_BITS;
    tr.cs_change = 0;
    tr.mode = MAX31855_SPI_MODE;
    if (ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -1;
    return 0;
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint8_t buf[4];
    int ret;
    uint32_t raw;
    int16_t raw_14, raw_12;
    int32_t tc_temp, int_temp;

    ret = max31855_spi_transfer(dev, buf, 4);
    if (ret < 0)
        return -1;

    raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];

    if (raw & 0x00010000) {
        return 1;
    }

    raw_14 = (int16_t)((raw >> 18) & 0x3FFF);
    if (raw_14 & 0x2000)
        raw_14 |= 0xC000;
    tc_temp = (((raw_14 >> 13) & 1) * (-8192) + ((raw_14 & 0x1FFF) ^ ((raw_14 >> 13) & 1) * 0x2000)) * 250 / 1000;

    raw_12 = (int16_t)((raw >> 4) & 0xFFF);
    if (raw_12 & 0x800)
        raw_12 |= 0xF000;
    int_temp = (((raw_12 >> 11) & 1) * (-2048) + ((raw_12 & 0x7FF) ^ ((raw_12 >> 11) & 1) * 0x800)) * 625 / 10000;

    *thermocouple_val = tc_temp;
    *temp_local_val = int_temp;
    return 0;
}
