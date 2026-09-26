#include "ds3231.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stddef.h>

#include "tobudos.h"
#define DS3231_I2C_ADDR 0x68
#define DS3231_ADDR_8BIT (DS3231_I2C_ADDR << 1)

#define REG_SECONDS 0x00
#define REG_MINUTES 0x01
#define REG_HOURS 0x02
#define REG_DAY 0x03
#define REG_DATE 0x04
#define REG_MONTH 0x05
#define REG_YEAR 0x06

static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DS3231_I2C_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_dev *dev, struct ds3231_time *t) {
    if (!dev || !t) return -1;
    uint8_t buf[7];
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = DS3231_ADDR_8BIT;
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Mem_Read(hi2c, addr, REG_SECONDS, I2C_MEMADD_SIZE_8BIT, buf, 7, 100);
    if (ret != HAL_OK) return -1;

    t->seconds = bcd_to_dec(buf[0]);
    t->minutes = bcd_to_dec(buf[1]);
    t->hours = bcd_to_dec(buf[2] & 0x3F); // mask 12/24 bit
    t->day = buf[3];
    t->date = bcd_to_dec(buf[4]);
    t->month = bcd_to_dec(buf[5] & 0x1F); // mask century bit
    t->year = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_set_time(struct ds3231_dev *dev, const struct ds3231_time *t) {
    if (!dev || !t) return -1;
    uint8_t buf[8];
    buf[0] = REG_SECONDS;
    buf[1] = dec_to_bcd(t->seconds);
    buf[2] = dec_to_bcd(t->minutes);
    buf[3] = dec_to_bcd(t->hours);
    buf[4] = t->day;
    buf[5] = dec_to_bcd(t->date);
    buf[6] = dec_to_bcd(t->month);
    buf[7] = dec_to_bcd(t->year);

    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = DS3231_ADDR_8BIT;
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(hi2c, addr, buf, 8, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}
