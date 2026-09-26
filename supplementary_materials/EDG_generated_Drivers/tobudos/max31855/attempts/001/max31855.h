#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "tobudos.h"
struct max31855_dev {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
};

int max31855_init(struct max31855_dev *dev, void *bus_handle);
int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *temp_local_val);

#endif
