#ifndef MAX31855_CMSIS_RTX_REF_H
#define MAX31855_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define MAX31855_SPI_MAX_HZ  5000000U
#define MAX31855_FAULT_BIT   (1UL << 16)
#define MAX31855_FAULT_SCV   (1UL << 2)
#define MAX31855_FAULT_SCG   (1UL << 1)
#define MAX31855_FAULT_OC    (1UL << 0)

struct max31855_device {
    SPI_HandleTypeDef *spi;
};

int max31855_init(struct max31855_device *dev, SPI_HandleTypeDef *spi);
int max31855_read_raw(struct max31855_device *dev, uint32_t *raw);
int max31855_has_fault(uint32_t raw);
uint8_t max31855_get_fault(uint32_t raw);
int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc);
int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc);
int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc);
int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc);

#endif
