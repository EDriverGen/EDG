#include "pca9685.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <nuttx/i2c/i2c_master.h>
#include <arch.h>

#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE1_SLEEP (1 << 4)

static int pca9685_i2c_write(struct pca9685_dev_s *dev, uint8_t reg, uint8_t *data, int len)
{
    struct i2c_msg_s msg[2];
    uint8_t reg_buf = reg;
    int ret;

    msg[0].frequency = 100000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = &reg_buf;
    msg[0].length = 1;

    msg[1].frequency = 100000;
    msg[1].addr = dev->addr;
    msg[1].flags = I2C_M_NOSTART;
    msg[1].buffer = data;
    msg[1].length = len;

    ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return OK;
}

static int pca9685_i2c_read(struct pca9685_dev_s *dev, uint8_t reg, uint8_t *data, int len)
{
    struct i2c_msg_s msg[2];
    uint8_t reg_buf = reg;
    int ret;

    msg[0].frequency = 100000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = &reg_buf;
    msg[0].length = 1;

    msg[1].frequency = 100000;
    msg[1].addr = dev->addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = data;
    msg[1].length = len;

    ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return OK;
}

int pca9685_init(struct pca9685_dev_s *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = PCA9685_I2C_ADDR;
    return OK;
}

int pca9685_read_pwm_channel(struct pca9685_dev_s *dev, uint8_t channel, uint16_t *duty)
{
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    int ret;
    uint16_t off_l, off_h;

    ret = pca9685_i2c_read(dev, reg, buf, 4);
    if (ret < 0) {
        return ret;
    }

    off_l = buf[0];
    off_h = buf[1];
    *duty = ((off_h & 0x0F) * 256) + off_l;
    return OK;
}