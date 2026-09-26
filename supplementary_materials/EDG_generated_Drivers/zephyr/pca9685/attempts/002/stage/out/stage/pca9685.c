#include "pca9685.h"
#include <errno.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#define PCA9685_I2C_ADDR 0x71
#define PCA9685_LED0_OFF_L 0x08

int pca9685_init(struct pca9685_dev *dev)
{
	dev->addr = PCA9685_I2C_ADDR;
	return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *val)
{
	uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
	uint8_t buf[4];
	int ret;

	struct i2c_msg msgs[2];

	msgs[0].buf = &reg;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = 4;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(dev->bus, msgs, 2, dev->addr);
	if (ret < 0) {
		return -EIO;
	}

	uint8_t off_l = buf[0];
	uint8_t off_h = buf[1];
	*val = ((off_h & 0x0F) * 256) + off_l;

	return 0;
}
