#include "bh1750.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_MEAS_DELAY_MS 180

int bh1750_init(struct bh1750_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    uint8_t cmd = BH1750_CMD_POWER_ON;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, dev->i2c_addr << 1, &cmd, 1, 100);
    if (ret != HAL_OK)
        return -1;

    return 0;
}

int bh1750_read(struct bh1750_dev *dev, int32_t *raw)
{
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, dev->i2c_addr << 1, &cmd, 1, 100);
    if (ret != HAL_OK)
        return -1;

    vTaskDelay(pdMS_TO_TICKS(BH1750_MEAS_DELAY_MS));

    uint8_t buf[2];
    ret = HAL_I2C_Master_Receive(dev->bus_handle, dev->i2c_addr << 1, buf, 2, 100);
    if (ret != HAL_OK)
        return -1;

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
