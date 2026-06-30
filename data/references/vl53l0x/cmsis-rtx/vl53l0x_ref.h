#ifndef VL53L0X_CMSIS_RTX_REF_H
#define VL53L0X_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT 0x29
#define VL53L0X_MODEL_ID     0xEE

struct vl53l0x_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);

#endif
