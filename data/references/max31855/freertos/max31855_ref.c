/*
 * MAX31855 thermocouple driver for FreeRTOS + STM32 HAL
 */
#include "max31855_ref.h"

int max31855_init(struct max31855_device *dev, SPI_HandleTypeDef *hspi,
                  GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    if (dev == NULL || hspi == NULL || cs_port == NULL) return -1;
    dev->hspi = hspi; dev->cs_port = cs_port; dev->cs_pin = cs_pin;
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t buf[4] = {0};
    if (dev == NULL || dev->hspi == NULL || raw == NULL) return -1;

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Receive(dev->hspi, buf, 4, 100);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    if (st != HAL_OK) return -1;
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
    uint32_t raw; if (max31855_read_raw(dev, &raw) != 0) return -1;
    return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; if (max31855_read_raw(dev, &raw) != 0) return -1;
    return max31855_get_internal_temp(raw, temp_mc);
}
