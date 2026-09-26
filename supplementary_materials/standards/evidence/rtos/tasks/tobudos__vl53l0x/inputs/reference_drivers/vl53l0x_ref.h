#ifndef __VL53L0X_REF_H
#define __VL53L0X_REF_H
#include "tos_k.h"
#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT  0x29
#define VL53L0X_MODEL_ID      0xEE

struct vl53l0x_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);
#endif
