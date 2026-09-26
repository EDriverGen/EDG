#include "bh1750.h"
#include <stdint.h>

#include "tobudos.h"
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07

int bh1750_init(struct bh1750_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    uint8_t cmd = BH1750_CMD_POWER_ON;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     &cmd, 1, 100);
    if (ret != HAL_OK)
        return -1;

    return 0;
}

int bh1750_read(struct bh1750_device *dev, int32_t *raw)
{
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle,
                                                     (uint16_t)(dev->i2c_addr << 1),
                                                     &cmd, 1, 100);
    if (ret != HAL_OK)
        return -1;

    tos_sleep_ms(180);

    uint8_t buf[2];
    ret = HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle,
                                  (uint16_t)(dev->i2c_addr << 1),
                                  buf, 2, 100);
    if (ret != HAL_OK)
        return -1;

    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
