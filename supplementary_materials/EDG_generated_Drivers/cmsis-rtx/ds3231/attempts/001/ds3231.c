#include "ds3231.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include <string.h>

#include "cmsis_rtx.h"
#define DS3231_I2C_ADDR 0x68
#define DS3231_I2C_ADDR_SHIFTED (DS3231_I2C_ADDR << 1)

#define REG_SECONDS 0x00
#define REG_MINUTES 0x01
#define REG_HOURS 0x02
#define REG_DAY 0x03
#define REG_DATE 0x04
#define REG_MONTH 0x05
#define REG_YEAR 0x06

static int ds3231_write_then_read(struct ds3231_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    if (HAL_I2C_Mem_Read(hi2c, DS3231_I2C_ADDR_SHIFTED, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) != HAL_OK)
        return -1;
    return 0;
}

static int ds3231_write_bytes(struct ds3231_dev *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint8_t buf[16];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    if (HAL_I2C_Master_Transmit(hi2c, DS3231_I2C_ADDR_SHIFTED, buf, len + 1, 100) != HAL_OK)
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
    dev->i2c_addr = DS3231_I2C_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    if (ds3231_write_then_read(dev, REG_SECONDS, buf, 7) != 0)
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

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t)
{
    uint8_t data[7];
    data[0] = dec_to_bcd(t->seconds);
    data[1] = dec_to_bcd(t->minutes);
    data[2] = dec_to_bcd(t->hours);
    data[3] = t->day;
    data[4] = dec_to_bcd(t->date);
    data[5] = dec_to_bcd(t->month);
    data[6] = dec_to_bcd(t->year);
    if (ds3231_write_bytes(dev, REG_SECONDS, data, 7) != 0)
        return -1;
    return 0;
}
