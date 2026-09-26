#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#include "threadx.h"
typedef struct {
    I2C_HandleTypeDef *bus_handle;
    uint8_t i2c_addr;
} vl53l0x_t;

int32_t vl53l0x_init(vl53l0x_t *dev, void *bus_handle);
int32_t vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw);

#endif /* VL53L0X_H */
