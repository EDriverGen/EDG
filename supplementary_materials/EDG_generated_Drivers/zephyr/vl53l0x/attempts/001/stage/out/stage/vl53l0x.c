#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define VL53L0X_I2C_ADDR 0x52

static int vl53l0x_write_reg(const struct device *dev, uint8_t reg)
{
	uint8_t buf[1] = { reg };
	struct i2c_msg msg;
	msg.buf = buf;
	msg.len = 1;
	msg.flags = I2C_MSG_WRITE;
	return i2c_transfer(dev, &msg, 1, VL53L0X_I2C_ADDR);
}

static int vl53l0x_read_reg16(const struct device *dev, uint8_t reg, uint16_t *val)
{
	uint8_t buf[2];
	int ret;
	struct i2c_msg msgs[2];

	msgs[0].buf = &reg;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = 2;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer(dev, msgs, 2, VL53L0X_I2C_ADDR);
	if (ret < 0)
		return ret;

	*val = ((uint16_t)buf[0] << 8) | buf[1];
	return 0;
}

int vl53l0x_init(const struct device *dev)
{
	(void)dev;
	return 0;
}

int vl53l0x_read_distance(const struct device *dev, int32_t *raw)
{
	uint16_t distance;
	int ret;

	ret = vl53l0x_write_reg(dev, 0x51);
	if (ret < 0)
		return ret;

	ret = vl53l0x_read_reg16(dev, 0x51, &distance);
	if (ret < 0)
		return ret;

	*raw = (int32_t)distance;
	return 0;
}
