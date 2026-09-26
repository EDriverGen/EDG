#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
#define TMP421_I2C_ADDR 0x2A

#define REG_LOCAL_TEMP_HIGH 0x00
#define REG_REMOTE1_TEMP_HIGH 0x01
#define REG_STATUS 0x08
#define REG_LOCAL_TEMP_LOW 0x10
#define REG_REMOTE1_TEMP_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF

#define STATUS_BUSY_BIT 0x80

#define EXPECTED_MANUFACTURER_ID 0x55
#define EXPECTED_DEVICE_ID 0x21

static int tmp421_write_then_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint8_t len)
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
	int timeout = 130; /* ms per channel */

	while (timeout-- > 0) {
		ret = tmp421_read_reg(dev, REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}
		if (!(status & STATUS_BUSY_BIT)) {
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
	ret = tmp421_read_reg(dev, REG_MANUFACTURER_ID, &id);
	if (ret < 0) {
		return ret;
	}
	if (id != EXPECTED_MANUFACTURER_ID) {
		return -ENODEV;
	}

	/* Probe Device ID */
	ret = tmp421_read_reg(dev, REG_DEVICE_ID, &id);
	if (ret < 0) {
		return ret;
	}
	if (id != EXPECTED_DEVICE_ID) {
		return -ENODEV;
	}

	return 0;
}

static int tmp421_read_temperature(const struct device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
	uint8_t buf[2];
	uint8_t low_byte;
	int16_t raw;
	int ret;

	ret = tmp421_poll_busy(dev);
	if (ret < 0) {
		return ret;
	}

	/* Read high byte and low byte in one transaction from high_reg */
	ret = tmp421_write_then_read(dev, high_reg, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Read low byte from low_reg separately */
	ret = tmp421_read_reg(dev, low_reg, &low_byte);
	if (ret < 0) {
		return ret;
	}

	/* Combine: high byte is buf[0], low byte upper nibble from low_byte */
	int8_t high_signed = (int8_t)buf[0];
	uint8_t low_nibble = (low_byte >> 4) & 0x0F;
	raw = ((int16_t)high_signed << 4) | low_nibble;

	/* Convert to milli_degC: raw * 625 / 10 */
	*temp = ((int32_t)raw * 625) / 10;

	return 0;
}

int tmp421_read_temperature_local(const struct device *dev, int32_t *temp_local_val)
{
	return tmp421_read_temperature(dev, REG_LOCAL_TEMP_HIGH, REG_LOCAL_TEMP_LOW, temp_local_val);
}

int tmp421_read_temperature_remote1(const struct device *dev, int32_t *temp_remote_val)
{
	return tmp421_read_temperature(dev, REG_REMOTE1_TEMP_HIGH, REG_REMOTE1_TEMP_LOW, temp_remote_val);
}
