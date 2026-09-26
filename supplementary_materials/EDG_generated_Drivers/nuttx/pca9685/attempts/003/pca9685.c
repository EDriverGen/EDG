#include "pca9685.h"
#include <stddef.h>
#include <errno.h>
#include <arch.h>

#define PCA9685_I2C_ADDR 0x71
#define PCA9685_LED0_OFF_L 0x08

static int i2c_write_then_read(struct i2c_master_s *bus, uint8_t addr, uint8_t *wbuffer, int wlen, uint8_t *rbuffer, int rlen)
{
    struct i2c_msg_s msg[2];
    int ret;

    msg[0].frequency = 100000;
    msg[0].addr = addr;
    msg[0].flags = 0;
    msg[0].buffer = wbuffer;
    msg[0].length = wlen;

    msg[1].frequency = 100000;
    msg[1].addr = addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = rbuffer;
    msg[1].length = rlen;

    ret = I2C_TRANSFER(bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_write(struct i2c_master_s *bus, uint8_t addr, uint8_t *buffer, int len)
{
    struct i2c_msg_s msg;
    int ret;

    msg.frequency = 100000;
    msg.addr = addr;
    msg.flags = 0;
    msg.buffer = buffer;
    msg.length = len;

    ret = I2C_TRANSFER(bus, &msg, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int i2c_read(struct i2c_master_s *bus, uint8_t addr, uint8_t *buffer, int len)
{
    struct i2c_msg_s msg;
    int ret;

    msg.frequency = 100000;
    msg.addr = addr;
    msg.flags = I2C_M_READ;
    msg.buffer = buffer;
    msg.length = len;

    ret = I2C_TRANSFER(bus, &msg, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int pca9685_init(struct pca9685_dev_s *dev, struct i2c_master_s *bus)
{
    if (dev == NULL || bus == NULL) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev_s *dev, uint8_t channel, uint16_t *duty)
{
    uint8_t reg;
    uint8_t buf[4];
    int ret;
    uint16_t off_l, off_h;

    if (dev == NULL || duty == NULL) {
        return -EINVAL;
    }

    reg = PCA9685_LED0_OFF_L + (channel * 4);

    ret = i2c_write(dev->bus, dev->addr, &reg, 1);
    if (ret < 0) {
        return ret;
    }

    ret = i2c_read(dev->bus, dev->addr, buf, 4);
    if (ret < 0) {
        return ret;
    }

    off_l = buf[0];
    off_h = buf[1];
    *duty = ((off_h & 0x0F) * 256) + off_l;

    return 0;
}