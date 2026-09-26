#include "max31855.h"
#include <arch.h>
#include <errno.h>
#include <stdint.h>

#include <nuttx/spi/spi.h>
#include "nuttx.h"
#define MAX31855_DEV_ID 0

int max31855_init(struct max31855_dev *dev, struct spi_dev_s *spi)
{
    if (!dev || !spi)
        return -EINVAL;
    dev->spi = spi;
    dev->devid = MAX31855_DEV_ID;
    up_mdelay(200);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc, int32_t *local)
{
    uint8_t buf[4];
    uint32_t raw32;
    int16_t raw_14, raw_12;
    int32_t tc_val, local_val;
    int ret;

    if (!dev || !dev->spi || !tc || !local)
        return -EINVAL;

    SPI_SELECT(dev->spi, dev->devid, true);
    ret = SPI_EXCHANGE(dev->spi, NULL, buf, 4);
    SPI_SELECT(dev->spi, dev->devid, false);

    if (ret < 0)
        return ret;

    raw32 = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8) | buf[3];

    if (raw32 & 0x00010000) {
        return -EIO;
    }

    raw_14 = (int16_t)((raw32 >> 18) & 0x3FFF);
    if (raw_14 & 0x2000)
        raw_14 |= 0xC000;

    raw_12 = (int16_t)((raw32 >> 4) & 0x0FFF);
    if (raw_12 & 0x0800)
        raw_12 |= 0xF000;

    tc_val = (((raw_14 >> 13) & 1) * (-8192) + ((raw_14 & 0x1FFF) ^ ((raw_14 >> 13) & 1) * 0x2000)) * 250 / 1000;
    local_val = (((raw_12 >> 11) & 1) * (-2048) + ((raw_12 & 0x7FF) ^ ((raw_12 >> 11) & 1) * 0x800)) * 625 / 10000;

    *tc = tc_val;
    *local = local_val;

    return 0;
}
