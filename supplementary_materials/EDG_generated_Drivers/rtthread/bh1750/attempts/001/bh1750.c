#include "bh1750.h"
#include <rtdevice.h>
#include <rtthread.h>

#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07
#define BH1750_CMD_CONT_H_RES_MODE 0x10
#define BH1750_CMD_CONT_H_RES_MODE2 0x11
#define BH1750_CMD_CONT_L_RES_MODE 0x13
#define BH1750_CMD_ONETIME_H_RES_MODE 0x20
#define BH1750_CMD_ONETIME_H_RES_MODE2 0x21
#define BH1750_CMD_ONETIME_L_RES_MODE 0x23

static int bh1750_write_cmd(bh1750_device_t *dev, uint8_t cmd)
{
    struct rt_i2c_msg msg;
    uint8_t buf[1] = {cmd};
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_WR;
    msg.buf = buf;
    msg.len = 1;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -RT_EIO;
    return RT_EOK;
}

static int bh1750_read_data(bh1750_device_t *dev, uint16_t *raw)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2];
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_RD;
    msg.buf = buf;
    msg.len = 2;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -RT_EIO;
    *raw = ((uint16_t)buf[0] << 8) | buf[1];
    return RT_EOK;
}

int bh1750_init(bh1750_device_t *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = BH1750_I2C_ADDR;
    dev->mt_reg = 69;
    return RT_EOK;
}

int bh1750_read_illuminance(bh1750_device_t *dev, uint16_t *raw)
{
    int ret;
    ret = bh1750_write_cmd(dev, BH1750_CMD_CONT_H_RES_MODE);
    if (ret != RT_EOK)
        return ret;
    rt_thread_mdelay(180);
    ret = bh1750_read_data(dev, raw);
    if (ret != RT_EOK)
        return ret;
    return RT_EOK;
}
