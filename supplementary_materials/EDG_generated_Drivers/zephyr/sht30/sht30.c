#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define SHT30_I2C_ADDR 0x44

#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400

static int sht30_write_command(const struct device *dev, uint16_t cmd)
{
	uint8_t buf[2];
	buf[0] = (cmd >> 8) & 0xFF;
	buf[1] = cmd & 0xFF;
	struct i2c_dt_spec spec = { .bus = dev, .addr = SHT30_I2C_ADDR };
	return i2c_write_dt(&spec, buf, 2);
}

static int sht30_read_data(const struct device *dev, uint8_t *buf, uint32_t len)
{
	struct i2c_dt_spec spec = { .bus = dev, .addr = SHT30_I2C_ADDR };
	return i2c_read_dt(&spec, buf, len);
}

int sht30_init(const struct device *dev)
{
	int ret;
	/* Soft reset */
	ret = sht30_write_command(dev, SHT30_CMD_SOFT_RESET);
	if (ret < 0) {
		return ret;
	}
	k_msleep(2); /* wait max 1.5 ms */
	/* General call reset */
	ret = sht30_write_command(dev, SHT30_CMD_GENERAL_CALL_RESET);
	if (ret < 0) {
		return ret;
	}
	k_msleep(2);
	return 0;
}

int sht30_read_measurement(const struct device *dev, int32_t *temp_val, int32_t *hum_val)
{
	int ret;
	uint8_t buf[6];
	uint16_t st, srh;
	int32_t temp, hum;

	/* Send single shot measurement command */
	ret = sht30_write_command(dev, SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH);
	if (ret < 0) {
		return ret;
	}
	/* Wait for measurement (max 15 ms) */
	k_msleep(15);

	/* Read 6 bytes */
	ret = sht30_read_data(dev, buf, 6);
	if (ret < 0) {
		return ret;
	}

	/* Parse temperature (big-endian) */
	st = ((uint16_t)buf[0] << 8) | buf[1];
	/* Parse humidity (big-endian) */
	srh = ((uint16_t)buf[3] << 8) | buf[4];

	/* Convert to milli-degC: ((ST * 175000) // 65535) - 45000 */
	temp = ((int32_t)(((int64_t)st * 175000) / 65535)) - 45000;
	/* Convert to milli-percent: ((SRH * 100000) // 65535) */
	hum = (int32_t)(((int64_t)srh * 100000) / 65535);

	*temp_val = temp;
	*hum_val = hum;

	return 0;
}
