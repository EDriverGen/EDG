#include "lm75a.h"
#include <stdint.h>

#include "FreeRTOS.h"
#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00

static int lm75a_write_then_read(struct lm75a_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
    if (ret != HAL_OK) {
        return -1;
    }
    return 0;
}

int lm75a_init(struct lm75a_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return 0;
}

int lm75a_read_temp(struct lm75a_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;
    ret = lm75a_write_then_read(dev, LM75A_TEMP_REG, buf, 2);
    if (ret != 0) {
        return ret;
    }
    int16_t raw16 = (int16_t)((buf[0] << 8) | buf[1]);
    *raw = (int32_t)(raw16 >> 5);
    return 0;
}
