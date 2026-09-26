#include "lm75a.h"
#include <stdint.h>

#include "FreeRTOS.h"
#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00
#define I2C_TIMEOUT 100

int lm75a_init(struct lm75a_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    vTaskDelay(pdMS_TO_TICKS(100));
    return 0;
}

int lm75a_read_temp(struct lm75a_dev *dev, int32_t *raw) {
    uint8_t cmd = LM75A_TEMP_REG;
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, &cmd, 1, I2C_TIMEOUT);
    if (ret != HAL_OK) return -1;

    ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, dev->i2c_addr << 1, buf, 2, I2C_TIMEOUT);
    if (ret != HAL_OK) return -1;

    int32_t raw_val = ((int32_t)buf[0] << 8) | buf[1];
    raw_val >>= 5;
    if (raw_val & 0x400) {
        raw_val |= ~0x7FF;
    }
    *raw = raw_val;
    return 0;
}
