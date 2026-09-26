#include "ds3231_ref.h"

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}

static int ds3231_read_reg(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev == 0 || dev->bus == 0 || buf == 0 || len == 0) return -1;
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1),
                            reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK ? 0 : -1;
}

int ds3231_init(struct ds3231_device *dev, I2C_HandleTypeDef *bus, uint16_t addr)
{
    if (dev == 0 || bus == 0) return -1;
    if (HAL_I2C_Init(bus) != HAL_OK) return -1;
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int ds3231_probe(struct ds3231_device *dev)
{
    uint8_t val = 0;
    return ds3231_read_reg(dev, 0x00, &val, 1);
}

int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t)
{
    uint8_t buf[7];
    if (dev == 0 || t == 0) return -1;
    if (ds3231_read_reg(dev, 0x00, buf, 7) != 0) return -1;
    t->seconds = bcd_to_dec(buf[0] & 0x7FU);
    t->minutes = bcd_to_dec(buf[1] & 0x7FU);
    t->hours = bcd_to_dec(buf[2] & 0x3FU);
    t->day = bcd_to_dec(buf[3] & 0x07U);
    t->date = bcd_to_dec(buf[4] & 0x3FU);
    t->month = bcd_to_dec(buf[5] & 0x1FU);
    t->year = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius)
{
    uint8_t buf[2];
    int16_t raw;
    if (temp_mcelsius == 0) return -1;
    if (ds3231_read_reg(dev, 0x11, buf, 2) != 0) return -1;
    raw = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    *temp_mcelsius = ((int32_t)(raw >> 6) * 250);
    return 0;
}
