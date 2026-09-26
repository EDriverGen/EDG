#include "ds3231.h"
#include <string.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "tx_api.h"

#include "threadx.h"
#define DS3231_ADDR 0x68
#define DS3231_ADDR_SHIFTED (DS3231_ADDR << 1)

static int ds3231_write_reg(struct ds3231_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Write((I2C_HandleTypeDef *)dev->bus_handle, DS3231_ADDR_SHIFTED, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

static int ds3231_read_reg(struct ds3231_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle, DS3231_ADDR_SHIFTED, reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    int ret;
    ret = ds3231_read_reg(dev, 0x00, buf, 7);
    if (ret != 0) return ret;
    t->seconds = ((buf[0] >> 4) & 0x07) * 10 + (buf[0] & 0x0F);
    t->minutes = ((buf[1] >> 4) & 0x07) * 10 + (buf[1] & 0x0F);
    t->hours = ((buf[2] >> 4) & 0x03) * 10 + (buf[2] & 0x0F);
    t->day = buf[3] & 0x07;
    t->date = ((buf[4] >> 4) & 0x03) * 10 + (buf[4] & 0x0F);
    t->month = ((buf[5] >> 4) & 0x01) * 10 + (buf[5] & 0x0F);
    t->year = ((buf[6] >> 4) & 0x0F) * 10 + (buf[6] & 0x0F);
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t buf[8];
    buf[0] = 0x00;
    buf[1] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    buf[2] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    buf[3] = ((t->hours / 10) << 4) | (t->hours % 10);
    buf[4] = t->day & 0x07;
    buf[5] = ((t->date / 10) << 4) | (t->date % 10);
    buf[6] = ((t->month / 10) << 4) | (t->month % 10);
    buf[7] = ((t->year / 10) << 4) | (t->year % 10);
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, DS3231_ADDR_SHIFTED, buf, 8, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}
