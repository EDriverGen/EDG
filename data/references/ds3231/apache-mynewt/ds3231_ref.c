#include "ds3231_ref.h"

static uint8_t bcd_to_dec(uint8_t bcd)
{
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}

static int ds3231_read_reg(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || buf == 0 || len == 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 1;
    xfer.buffer = &reg;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = len;
    xfer.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

int ds3231_init(struct ds3231_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0) return -1;
    if (hal_i2c_init(i2c_num, 0) != 0) return -1;
    dev->i2c_num = i2c_num;
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
