#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
struct bme280_dev {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
};

int bme280_init(struct bme280_dev *dev, void *bus_handle);
int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw);

#endif
