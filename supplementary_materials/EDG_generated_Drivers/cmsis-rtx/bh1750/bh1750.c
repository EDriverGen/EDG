#include "bh1750.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07

static int i2c_write(struct bh1750_dev *dev, uint8_t *data, uint16_t size)
{
    if (HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, size, 100) != HAL_OK)
        return -1;
    return 0;
}

static int i2c_read(struct bh1750_dev *dev, uint8_t *buf, uint16_t size)
{
    if (HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, size, 100) != HAL_OK)
        return -1;
    return 0;
}

int bh1750_init(struct bh1750_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;
    uint8_t cmd = BH1750_CMD_POWER_ON;
    if (i2c_write(dev, &cmd, 1) != 0)
        return -1;
    return 0;
}

int bh1750_read_illuminance(struct bh1750_dev *dev, int32_t *raw)
{
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    if (i2c_write(dev, &cmd, 1) != 0)
        return -1;
    osDelay(180);
    uint8_t buf[2];
    if (i2c_read(dev, buf, 2) != 0)
        return -1;
    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
