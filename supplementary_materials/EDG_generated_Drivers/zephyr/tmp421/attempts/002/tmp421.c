#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#define TMP421_I2C_ADDR 0x2A

static int tmp421_write_then_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint32_t len)
{
	struct i2c_msg msgs[2];
	uint8_t reg_buf = reg;

	msgs[0].buf = &reg_buf;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, TMP421_I2C_ADDR);
}

static int tmp421_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(const struct device *dev)
{
	uint8_t status;
	int ret;
	int timeout = 130;

	while (timeout-- > 0) {
		ret = tmp421_read_reg(dev, 0x08, &status);
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

int tmp421_init(const struct device *dev)
{
	uint8_t id;
	int ret;

	/* Probe Manufacturer ID */
	ret = tmp421_read_reg(dev, 0xFE, &id);
	if (ret < 0) {
		return ret;
	}
	if (id != 0x55) {
		return -EIO;
	}

	/* Probe Device ID */
	ret = tmp421_read_reg(dev, 0xFF, &id);
	if (ret < 0) {
		return ret;
	}
	if (id != 0x21) {
		return -EIO;
	}

	return 0;
}

int tmp421_read_temperature_local(const struct device *dev, int32_t *temp_local_val)
{
	uint8_t buf[2];
	uint8_t low_byte;
	int16_t raw;
	int ret;

	ret = tmp421_poll_busy(dev);
	if (ret < 0) {
		return ret;
	}

	/* Read high and low bytes from pointer 0x00 */
	ret = tmp421_write_then_read(dev, 0x00, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Read low byte from pointer 0x10 */
	ret = tmp421_read_reg(dev, 0x10, &low_byte);
	if (ret < 0) {
		return ret;
	}

	/* Combine: high_byte is signed, low_byte upper nibble is fractional */
	int8_t high = (int8_t)buf[0];
	uint8_t low_nibble = (low_byte >> 4) & 0x0F;
	raw = ((int16_t)high << 4) | low_nibble;

	/* Convert to milli_degC: raw * 625 / 10 */
	*temp_local_val = ((int32_t)raw * 625) / 10;
	return 0;
}

int tmp421_read_temperature_remote1(const struct device *dev, int32_t *temp_remote_val)
{
	uint8_t buf[2];
	uint8_t low_byte;
	int16_t raw;
	int ret;

	ret = tmp421_poll_busy(dev);
	if (ret < 0) {
		return ret;
	}

	/* Read high and low bytes from pointer 0x01 */
	ret = tmp421_write_then_read(dev, 0x01, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Read low byte from pointer 0x11 */
	ret = tmp421_read_reg(dev, 0x11, &low_byte);
	if (ret < 0) {
		return ret;
	}

	/* Combine: high_byte is signed, low_byte upper nibble is fractional */
	int8_t high = (int8_t)buf[0];
	uint8_t low_nibble = (low_byte >> 4) & 0x0F;
	raw = ((int16_t)high << 4) | low_nibble;

	/* Convert to milli_degC: raw * 625 / 10 */
	*temp_remote_val = ((int32_t)raw * 625) / 10;
	return 0;
}
