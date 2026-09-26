#include "max31855_ref.h"

int max31855_init(struct max31855_device *dev, SPI_HandleTypeDef *spi)
{
    if (dev == 0 || spi == 0) {
        return -1;
    }
    if (HAL_SPI_Init(spi) != HAL_OK) {
        return -1;
    }
    dev->spi = spi;
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t rx[4];
    if (dev == 0 || dev->spi == 0 || raw == 0) {
        return -1;
    }
    if (HAL_SPI_Receive(dev->spi, rx, 4, 100) != HAL_OK) {
        return -1;
    }
    *raw = ((uint32_t)rx[0] << 24) |
           ((uint32_t)rx[1] << 16) |
           ((uint32_t)rx[2] << 8) |
           rx[3];
    return 0;
}

int max31855_has_fault(uint32_t raw)
{
    return (raw & MAX31855_FAULT_BIT) != 0;
}

uint8_t max31855_get_fault(uint32_t raw)
{
    return (uint8_t)(raw & (MAX31855_FAULT_SCV | MAX31855_FAULT_SCG | MAX31855_FAULT_OC));
}

int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == 0 || max31855_has_fault(raw)) {
        return -1;
    }
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) {
        val |= ~((int32_t)0x3FFF);
    }
    *temp_mc = val * 250;
    return 0;
}

int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == 0) {
        return -1;
    }
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) {
        val |= ~((int32_t)0x0FFF);
    }
    *temp_mc = (val * 625) / 10;
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw = 0;
    if (max31855_read_raw(dev, &raw) != 0) {
        return -1;
    }
    return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw = 0;
    if (max31855_read_raw(dev, &raw) != 0) {
        return -1;
    }
    return max31855_get_internal_temp(raw, temp_mc);
}
