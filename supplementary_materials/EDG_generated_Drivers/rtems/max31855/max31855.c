#include "max31855.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <linux/spi/spidev.h>
#define MAX31855_FAULT_BIT (1 << 16)

static int max31855_spi_read(struct max31855_device *dev, uint8_t *buf, size_t len)
{
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)NULL;
    tr.rx_buf = (unsigned long)buf;
    tr.len = len;
    tr.speed_hz = 1000000;
    tr.delay_usecs = 0;
    tr.bits_per_word = 8;
    tr.cs_change = 0;
    tr.mode = 0;
    int ret = spi_bus_ioctl(dev->bus, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) {
        return -errno;
    }
    return 0;
}

int max31855_init(struct max31855_device *dev, spi_bus bus)
{
    dev->bus = bus;
    return 0;
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint8_t buf[4];
    int ret = max31855_spi_read(dev, buf, 4);
    if (ret != 0) {
        return ret;
    }
    uint32_t raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    if (raw & MAX31855_FAULT_BIT) {
        return 1;
    }
    int16_t raw_14 = (int16_t)((raw >> 18) & 0x3FFF);
    if (raw_14 & 0x2000) {
        raw_14 |= 0xC000;
    }
    int32_t thermocouple_temp = (((raw_14 >> 13) & 1) * (-8192) + ((raw_14 & 0x1FFF) ^ ((raw_14 >> 13) & 1) * 0x2000)) * 250 / 1000;
    int16_t raw_12 = (int16_t)((raw >> 4) & 0xFFF);
    if (raw_12 & 0x800) {
        raw_12 |= 0xF000;
    }
    int32_t internal_temp = (((raw_12 >> 11) & 1) * (-2048) + ((raw_12 & 0x7FF) ^ ((raw_12 >> 11) & 1) * 0x800)) * 625 / 10000;
    *thermocouple_val = thermocouple_temp;
    *temp_local_val = internal_temp;
    return 0;
}
