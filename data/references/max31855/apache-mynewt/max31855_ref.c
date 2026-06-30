#include "max31855_ref.h"

int max31855_init(struct max31855_device *dev, int spi_num)
{
    struct hal_spi_settings settings;
    if (dev == 0) {
        return -1;
    }
    settings.data_mode = HAL_SPI_MODE0;
    settings.data_order = HAL_SPI_MSB_FIRST;
    settings.word_size = HAL_SPI_WORD_SIZE_8BIT;
    settings.baudrate = MAX31855_SPI_MAX_HZ;
    if (hal_spi_init(spi_num, 0, HAL_SPI_TYPE_MASTER) != 0 ||
        hal_spi_config(spi_num, &settings) != 0 ||
        hal_spi_enable(spi_num) != 0) {
        return -1;
    }
    dev->spi_num = spi_num;
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t tx[4] = {0, 0, 0, 0};
    uint8_t rx[4];
    if (dev == 0 || raw == 0) {
        return -1;
    }
    if (hal_spi_txrx(dev->spi_num, tx, rx, 4) != 0) {
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
