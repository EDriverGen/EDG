#include "vl53l0x.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#include "stm32f1xx_hal.h"
#define VL53L0X_I2C_ADDR 0x52
#define VL53L0X_READ_TIMEOUT 100

int32_t vl53l0x_init(vl53l0x_t *dev, void *bus_handle) {
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    HAL_Delay(2);
    return 0;
}

int32_t vl53l0x_read_distance(vl53l0x_t *dev, int32_t *raw) {
    if (dev == NULL || raw == NULL) return -1;
    uint8_t reg = 0x00;
    uint8_t buf[2];
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(dev->bus_handle, dev->i2c_addr << 1, &reg, 1, VL53L0X_READ_TIMEOUT);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, dev->i2c_addr << 1, buf, 2, VL53L0X_READ_TIMEOUT);
    if (ret != HAL_OK) return -1;
    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
