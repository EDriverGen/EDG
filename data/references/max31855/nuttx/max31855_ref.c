#include "max31855_ref.h"
#include <errno.h>

int max31855_init(FAR struct max31855_device *dev, FAR struct spi_dev_s *spi, uint32_t devid)
{
    if (!dev || !spi) return -EINVAL;
    dev->spi = spi; dev->devid = devid;
    return 0;
}

int max31855_read_raw(FAR struct max31855_device *dev, FAR uint32_t *raw)
{
    uint8_t buf[4];
    if (!dev || !dev->spi || !raw) return -EINVAL;

    SPI_LOCK(dev->spi, true);
    SPI_SETFREQUENCY(dev->spi, 5000000);
    SPI_SETMODE(dev->spi, SPIDEV_MODE0);
    SPI_SETBITS(dev->spi, 8);
    SPI_SELECT(dev->spi, dev->devid, true);
    SPI_RECVBLOCK(dev->spi, buf, 4);
    SPI_SELECT(dev->spi, dev->devid, false);
    SPI_LOCK(dev->spi, false);

    *raw = ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
    return 0;
}

int max31855_has_fault(uint32_t raw) { return (raw & MAX31855_FAULT_BIT) ? 1 : 0; }
uint8_t max31855_get_fault(uint32_t raw) { return (uint8_t)(raw & 7); }

int max31855_get_thermocouple_temp(uint32_t raw, FAR int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return -EINVAL;
    if (raw & MAX31855_FAULT_BIT) return -EINVAL;
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) val |= ~((uint32_t)0x3FFF);
    *temp_mc = val * 250;
    return 0;
}

int max31855_get_internal_temp(uint32_t raw, FAR int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return -EINVAL;
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) val |= ~((uint32_t)0x0FFF);
    *temp_mc = (val * 625) / 10;
    return 0;
}

int max31855_read_thermocouple(FAR struct max31855_device *dev, FAR int32_t *temp_mc)
{
    uint32_t raw; int ret = max31855_read_raw(dev, &raw);
    if (ret) return ret; return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(FAR struct max31855_device *dev, FAR int32_t *temp_mc)
{
    uint32_t raw; int ret = max31855_read_raw(dev, &raw);
    if (ret) return ret; return max31855_get_internal_temp(raw, temp_mc);
}
