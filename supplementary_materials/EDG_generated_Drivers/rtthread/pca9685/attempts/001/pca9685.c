#include "pca9685.h"
#include <drivers/dev_i2c.h>
#include <stdint.h>
#include <errno.h>

#include "rtthread.h"
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01

static int pca9685_write_then_read(struct pca9685_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t reg_buf = reg;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2) {
        return -EIO;
    }
    return 0;
}

static int pca9685_write(struct pca9685_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[1];
    uint8_t *data = (uint8_t *)&reg;
    (void)data;
    uint8_t *write_buf = (uint8_t *)&reg;
    (void)write_buf;
    uint8_t combined[256];
    combined[0] = reg;
    for (uint16_t i = 0; i < len; i++) {
        combined[1 + i] = buf[i];
    }

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1 + len;
    msgs[0].buf = combined;

    if (rt_i2c_transfer(dev->bus, msgs, 1) != 1) {
        return -EIO;
    }
    return 0;
}

int pca9685_init(struct pca9685_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm(struct pca9685_device *dev, uint8_t channel, uint16_t *pwm)
{
    uint8_t buf[4];
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    int ret;

    ret = pca9685_write_then_read(dev, reg, buf, 4);
    if (ret != 0) {
        return ret;
    }

    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *pwm = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
