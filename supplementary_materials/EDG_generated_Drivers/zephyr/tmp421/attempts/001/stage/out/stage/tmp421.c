#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define TMP421_I2C_ADDR 0x2A

static const struct device *g_eval_dev;

int tmp421_init(void)
{
	return 0;
}

static int tmp421_read_reg(const struct device *dev, uint8_t reg, uint8_t *buf, uint32_t len)
{
	struct i2c_msg msgs[2];
	uint8_t wbuf[1] = {reg};

	msgs[0].buf = wbuf;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, TMP421_I2C_ADDR);
}

static int tmp421_poll_busy(const struct device *dev)
{
	uint8_t status;
	int ret;
	int timeout = 130;

	while (timeout-- > 0) {
		ret = tmp421_read_reg(dev, 0x08, &status, 1);
		if (ret < 0) {
			return ret;
		}
		if (!(status & 0x80)) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	}
	return -ETIMEDOUT;
}

static int tmp421_read_temperature(const struct device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
	uint8_t high_byte, low_byte;
	int16_t raw;
	int ret;

	ret = tmp421_poll_busy(dev);
	if (ret < 0) {
		return ret;
	}

	ret = tmp421_read_reg(dev, high_reg, &high_byte, 1);
	if (ret < 0) {
		return ret;
	}

	ret = tmp421_read_reg(dev, low_reg, &low_byte, 1);
	if (ret < 0) {
		return ret;
	}

	raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
	*temp_milli = ((int32_t)raw * 625) / 10;
	return 0;
}

int tmp421_read_temperature_local(void *dev, int32_t *val)
{
	return tmp421_read_temperature((const struct device *)dev, 0x00, 0x10, val);
}

int tmp421_read_temperature_remote1(void *dev, int32_t *val)
{
	return tmp421_read_temperature((const struct device *)dev, 0x01, 0x11, val);
}
