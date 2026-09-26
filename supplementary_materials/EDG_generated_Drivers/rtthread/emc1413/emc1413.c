#include "emc1413.h"
#include <rtthread.h>
#include <rtdevice.h>

#define EMC1413_I2C_ADDR 0x4C

static int emc1413_write_reg(struct emc1413_device *dev, uint8_t reg, uint8_t data)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, data};
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    if (rt_i2c_transfer(dev->bus, msgs, 1) != 1)
        return -1;
    return 0;
}

static int emc1413_read_reg(struct emc1413_device *dev, uint8_t reg, uint8_t *data)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = 1;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int emc1413_read_regs(struct emc1413_device *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data;
    msgs[1].len = len;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

int emc1413_init(struct emc1413_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    rt_thread_mdelay(15);

    if (emc1413_write_reg(dev, 0x03, 0x00) != 0)
        return -1;
    if (emc1413_write_reg(dev, 0x04, 0x06) != 0)
        return -1;

    return 0;
}

int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_read_reg(dev, 0x00, &high) != 0)
        return -1;
    if (emc1413_read_reg(dev, 0x29, &low) != 0)
        return -1;
    uint16_t raw = ((uint16_t)high << 3) | (low >> 5);
    *temp = (int32_t)raw * 125;
    return 0;
}

int emc1413_read_external_diode_1_temp(struct emc1413_device *dev, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_read_reg(dev, 0x01, &high) != 0)
        return -1;
    if (emc1413_read_reg(dev, 0x10, &low) != 0)
        return -1;
    uint16_t raw = ((uint16_t)high << 3) | (low >> 5);
    *temp = (int32_t)raw * 125;
    return 0;
}

int emc1413_read_external_diode_2_temp(struct emc1413_device *dev, int32_t *temp)
{
    uint8_t buf[2];
    if (emc1413_read_regs(dev, 0x23, buf, 2) != 0)
        return -1;
    uint16_t raw = ((uint16_t)buf[0] << 3) | (buf[1] >> 5);
    *temp = (int32_t)raw * 125;
    return 0;
}
