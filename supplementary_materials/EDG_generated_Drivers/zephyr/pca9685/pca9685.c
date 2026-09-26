#include "pca9685.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <errno.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>
#define PCA9685_REG_MODE1 0x00
#define PCA9685_REG_MODE2 0x01
#define PCA9685_REG_LED0_OFF_L 0x08
#define PCA9685_REG_PRE_SCALE 0xFE

int pca9685_init(struct pca9685_dev *dev)
{
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out)
{
    if (channel > 1) {
        return -EINVAL;
    }

    uint8_t reg = PCA9685_REG_LED0_OFF_L + (channel * 4);
    uint8_t tx_buf[1] = { reg };
    uint8_t rx_buf[4];
    int ret;

    struct i2c_msg msgs[2];

    msgs[0].buf = tx_buf;
    msgs[0].len = 1;
    msgs[0].flags = I2C_MSG_WRITE;

    msgs[1].buf = rx_buf;
    msgs[1].len = 4;
    msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

    ret = i2c_transfer(dev->bus, msgs, 2, PCA9685_ALL_CALL_ADDR);
    if (ret < 0) {
        return -EIO;
    }

    uint8_t off_l = rx_buf[0];
    uint8_t off_h = rx_buf[1];
    uint16_t duty = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    *out = duty;

    return 0;
}
