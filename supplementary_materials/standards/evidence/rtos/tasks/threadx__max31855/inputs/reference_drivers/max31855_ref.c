#include "max31855_ref.h"
#include <stddef.h>

int max31855_init(struct max31855_device *dev, const struct max31855_spi_ops *ops, void *ctx)
{
    if (!dev || !ops) return -1;
    if (!ops->cs_select || !ops->cs_deselect || !ops->spi_recv) return -1;
    dev->ops = ops; dev->ctx = ctx;
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t buf[4] = {0};
    if (!dev || !dev->ops || !raw) return -1;

    dev->ops->cs_select(dev->ctx);
    int ret = dev->ops->spi_recv(dev->ctx, buf, 4);
    dev->ops->cs_deselect(dev->ctx);
    if (ret != 0) return -1;

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
    if (temp_mc == NULL) return -1;
    if (raw & MAX31855_FAULT_BIT) return -1;
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) val |= ~((uint32_t)0x3FFF);
    *temp_mc = val * 250;
    return 0;
}

int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return -1;
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) val |= ~((uint32_t)0x0FFF);
    *temp_mc = (val * 625) / 10;
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; if (max31855_read_raw(dev, &raw)) return -1;
    return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; if (max31855_read_raw(dev, &raw)) return -1;
    return max31855_get_internal_temp(raw, temp_mc);
}
