#include "vl53l0x.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#include "stm32f1xx_hal.h"
#define VL53L0X_I2C_ADDR 0x52

static int vl53l0x_read_reg(struct vl53l0x_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(VL53L0X_I2C_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

int vl53l0x_init(struct vl53l0x_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = VL53L0X_I2C_ADDR;
    HAL_Delay(2);
    return 0;
}

int vl53l0x_read_distance(struct vl53l0x_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;
    ret = vl53l0x_read_reg(dev, 0x51, buf, 2);
    if (ret != 0) {
        return -1;
    }
    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
