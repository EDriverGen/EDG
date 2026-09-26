#include "bh1750.h"
#include <rtthread.h>

#define BH1750_POWER_ON 0x01
#define BH1750_RESET 0x07
#define BH1750_CONT_H_RES_MODE 0x10
#define BH1750_CONT_H_RES_MODE2 0x11
#define BH1750_CONT_L_RES_MODE 0x13
#define BH1750_ONE_TIME_H_RES_MODE 0x20
#define BH1750_ONE_TIME_H_RES_MODE2 0x21
#define BH1750_ONE_TIME_L_RES_MODE 0x23

static int bh1750_write_cmd(bh1750_device_t *dev, uint8_t cmd)
{
    struct rt_i2c_msg msg;
    uint8_t buf[1] = {cmd};
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_WR;
    msg.len = 1;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int bh1750_read_raw(bh1750_device_t *dev, uint16_t *raw)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2];
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_RD;
    msg.len = 2;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    return 0;
}

int bh1750_init(bh1750_device_t *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = BH1750_I2C_ADDR;
    dev->mt_reg = 69;
    return 0;
}

int bh1750_read_illuminance(bh1750_device_t *dev, uint16_t *raw)
{
    int ret;
    ret = bh1750_write_cmd(dev, BH1750_CONT_H_RES_MODE);
    if (ret != 0)
        return -1;
    rt_thread_mdelay(180);
    ret = bh1750_read_raw(dev, raw);
    if (ret != 0)
        return -1;
    return 0;
}
