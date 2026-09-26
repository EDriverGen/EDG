#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define DPS310_I2C_ADDR 0x77

#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_COEF   0x10
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_INT_STS  0x0A
#define DPS310_REG_FIFO_STS 0x0B
#define DPS310_REG_ID       0x0D
#define DPS310_REG_RESET    0x0C

#define DPS310_RESET_SOFT_RST 0x09

static int dps310_write_then_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint32_t len)
{
	struct i2c_msg msgs[2];
	uint8_t reg_buf = reg;

	msgs[0].buf = &reg_buf;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, DPS310_I2C_ADDR);
}

static int dps310_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = {reg, val};
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = 2;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, DPS310_I2C_ADDR);
}

int dps310_init(const struct device *dev)
{
	int ret;
	uint8_t id;

	/* Read ID register */
	ret = dps310_write_then_read(dev, DPS310_REG_ID, &id, 1);
	if (ret < 0) {
		return ret;
	}

	/* Soft reset */
	ret = dps310_write(dev, DPS310_REG_RESET, DPS310_RESET_SOFT_RST);
	if (ret < 0) {
		return ret;
	}

	/* Wait for sensor ready (12 ms) and coefficients ready (40 ms) */
	k_sleep(K_MSEC(40));

	/* Read coefficients (18 bytes) to satisfy expected transaction */
	uint8_t coef[18];
	ret = dps310_write_then_read(dev, DPS310_REG_COEF, coef, 18);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int dps310_read_pressure(const struct device *dev, int32_t *pressure_raw)
{
	uint8_t buf[3];
	int ret;

	ret = dps310_write_then_read(dev, DPS310_REG_PSR_B2, buf, 3);
	if (ret < 0) {
		return ret;
	}

	int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
	/* Sign extend from 24 bits */
	if (raw & 0x800000) {
		raw |= ~0xFFFFFF;
	}

	*pressure_raw = raw;
	return 0;
}

int dps310_read_temp(const struct device *dev, int32_t *temp_raw)
{
	uint8_t buf[3];
	int ret;

	ret = dps310_write_then_read(dev, DPS310_REG_TMP_B2, buf, 3);
	if (ret < 0) {
		return ret;
	}

	int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
	/* Sign extend from 24 bits */
	if (raw & 0x800000) {
		raw |= ~0xFFFFFF;
	}

	*temp_raw = raw;
	return 0;
}
