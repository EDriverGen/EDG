#include "max31855.h"
#include <arch.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/spi/spi.h>
#include "nuttx.h"
#define MAX31855_DEVICE_ID 0

static int32_t thermocouple_temp_milliC(uint16_t raw_14)
{
    int32_t sign = (raw_14 >> 13) & 1;
    int32_t magnitude = raw_14 & 0x1FFF;
    int32_t value = sign * (-8192) + (magnitude ^ (sign * 0x2000));
    return (value * 250) / 1000;
}

static int32_t internal_temp_milliC(uint16_t raw_12)
{
    int32_t sign = (raw_12 >> 11) & 1;
    int32_t magnitude = raw_12 & 0x7FF;
    int32_t value = sign * (-2048) + (magnitude ^ (sign * 0x800));
    return (value * 625) / 10000;
}

int max31855_init(struct max31855_dev *dev, struct spi_dev_s *bus)
{
    dev->bus = bus;
    up_mdelay(200);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc, int32_t *local)
{
    uint8_t buf[4];
    uint32_t raw32;
    uint16_t raw_14, raw_12;
    bool fault;

    SPI_SELECT(dev->bus, MAX31855_DEVICE_ID, true);
    SPI_EXCHANGE(dev->bus, NULL, buf, 4);
    SPI_SELECT(dev->bus, MAX31855_DEVICE_ID, false);

    raw32 = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];

    fault = (raw32 >> 16) & 1;
    if (fault) {
        return 1;
    }

    raw_14 = (raw32 >> 18) & 0x3FFF;
    raw_12 = (raw32 >> 4) & 0xFFF;

    *tc = thermocouple_temp_milliC(raw_14);
    *local = internal_temp_milliC(raw_12);

    return 0;
}
