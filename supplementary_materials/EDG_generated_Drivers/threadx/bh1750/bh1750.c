#include "bh1750.h"
#include "threadx.h"
#include <stddef.h>

#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        (I2C_HandleTypeDef *)dev->bus_handle,
        (uint16_t)(dev->i2c_addr << 1),
        &cmd, 1, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int bh1750_read_data(struct bh1750_device *dev, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(
        (I2C_HandleTypeDef *)dev->bus_handle,
        (uint16_t)(dev->i2c_addr << 1),
        buf, len, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

int bh1750_init(struct bh1750_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
}

int bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw)
{
    int ret;
    uint8_t buf[2];
    
    ret = bh1750_write_cmd(dev, BH1750_CMD_CONT_HRES);
    if (ret != 0) return -1;
    
    tx_thread_sleep(180);
    
    ret = bh1750_read_data(dev, buf, 2);
    if (ret != 0) return -1;
    
    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
