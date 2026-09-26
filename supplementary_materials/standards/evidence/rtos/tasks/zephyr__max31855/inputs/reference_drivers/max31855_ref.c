/*
 * MAX31855 thermocouple driver for Zephyr (SPI)
 */
#include "max31855_ref.h"

int max31855_init(struct max31855_device *dev, const struct device *spi,
                  const struct gpio_dt_spec *cs_gpio)
{
    if (dev == NULL || spi == NULL) return -EINVAL;
    if (!device_is_ready(spi)) return -ENODEV;
    dev->spi_dev = spi;
    dev->spi_cfg.frequency = 5000000;
    dev->spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER;
    dev->spi_cfg.slave = 0;
    if (cs_gpio) { dev->cs_ctrl.gpio = *cs_gpio; dev->cs_ctrl.delay = 0;
                   dev->spi_cfg.cs = &dev->cs_ctrl; }
    else dev->spi_cfg.cs = NULL;
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t buf[4] = {0};
    struct spi_buf rx = { .buf = buf, .len = 4 };
    struct spi_buf_set rxs = { .buffers = &rx, .count = 1 };
    if (dev == NULL || dev->spi_dev == NULL || raw == NULL) return -EINVAL;
    int ret = spi_read(dev->spi_dev, &dev->spi_cfg, &rxs);
    if (ret < 0) return ret;
    *raw = ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
    return 0;
}

int max31855_has_fault(uint32_t raw) { return (raw & MAX31855_FAULT_BIT) ? 1 : 0; }
uint8_t max31855_get_fault(uint32_t raw) { return (uint8_t)(raw & 7); }

int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return -EINVAL;
    if (raw & MAX31855_FAULT_BIT) return -EINVAL;
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) val |= ~((uint32_t)0x3FFF);
    *temp_mc = val * 250;
    return 0;
}

int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return -EINVAL;
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) val |= ~((uint32_t)0x0FFF);
    *temp_mc = (val * 625) / 10;
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; int ret = max31855_read_raw(dev, &raw);
    if (ret) return ret; return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; int ret = max31855_read_raw(dev, &raw);
    if (ret) return ret; return max31855_get_internal_temp(raw, temp_mc);
}
