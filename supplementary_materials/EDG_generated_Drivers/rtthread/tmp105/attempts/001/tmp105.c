#include "tmp105.h"
#include <rtdevice.h>
#include <stdint.h>

#define TMP105_I2C_ADDR 0x48
#define TMP105_REG_TEMP 0x00

static int tmp105_write_pointer(struct tmp105_device *dev, uint8_t reg)
{
    struct rt_i2c_msg msg;
    uint8_t buf[1];
    buf[0] = reg;
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_WR;
    msg.len = 1;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int tmp105_read_bytes(struct tmp105_device *dev, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msg;
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_RD;
    msg.len = len;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

int tmp105_init(struct tmp105_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = TMP105_I2C_ADDR;
    rt_thread_mdelay(220);
    return 0;
}

int tmp105_read_temperature(struct tmp105_device *dev, int32_t *raw)
{
    uint8_t buf[2];
    int16_t raw12;
    if (tmp105_write_pointer(dev, TMP105_REG_TEMP) != 0)
        return -1;
    if (tmp105_read_bytes(dev, buf, 2) != 0)
        return -1;
    raw12 = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    raw12 >>= 4;
    if (raw12 & 0x0800)
        raw12 |= 0xF000;
    *raw = (int32_t)raw12 * 125 / 2;
    return 0;
}
