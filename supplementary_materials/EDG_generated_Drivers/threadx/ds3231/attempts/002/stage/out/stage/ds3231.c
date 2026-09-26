#include "ds3231.h"
#include <string.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "tx_api.h"

#include "threadx.h"
#define DS3231_ADDR 0x68
#define DS3231_ADDR_WRITE (DS3231_ADDR << 1)
#define DS3231_ADDR_READ (DS3231_ADDR << 1 | 1)

static int ds3231_i2c_write(struct ds3231_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[256];
    buf[0] = reg;
    memcpy(buf + 1, data, len);
    if (HAL_I2C_Master_Transmit(hi2c, DS3231_ADDR_WRITE, buf, len + 1, 100) != HAL_OK)
        return -1;
    return 0;
}

static int ds3231_i2c_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Master_Transmit(hi2c, DS3231_ADDR_WRITE, &reg, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive(hi2c, DS3231_ADDR_READ, data, len, 100) != HAL_OK)
        return -1;
    return 0;
}

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
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
    if (ds3231_i2c_read(dev, 0x00, buf, 7) != 0)
        return -1;
    t->seconds = bcd_to_dec(buf[0]);
    t->minutes = bcd_to_dec(buf[1]);
    t->hours = bcd_to_dec(buf[2] & 0x3F);
    t->day = buf[3];
    t->date = bcd_to_dec(buf[4]);
    t->month = bcd_to_dec(buf[5] & 0x1F);
    t->year = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    buf[0] = dec_to_bcd(t->seconds);
    buf[1] = dec_to_bcd(t->minutes);
    buf[2] = dec_to_bcd(t->hours);
    buf[3] = t->day;
    buf[4] = dec_to_bcd(t->date);
    buf[5] = dec_to_bcd(t->month);
    buf[6] = dec_to_bcd(t->year);
    if (ds3231_i2c_write(dev, 0x00, buf, 7) != 0)
        return -1;
    return 0;
}
