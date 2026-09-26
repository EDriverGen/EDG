#include "mcp23017.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define MCP23017_I2C_ADDR 0x20

#define MCP23017_REG_IODIRA 0x00
#define MCP23017_REG_IODIRB 0x01
#define MCP23017_REG_GPIOA 0x12
#define MCP23017_REG_GPIOB 0x13

int mcp23017_init(const struct device *dev)
{
	const struct i2c_dt_spec spec = { .bus = dev, .addr = MCP23017_I2C_ADDR };
	uint8_t buf[2];
	int ret;

	buf[0] = MCP23017_REG_IODIRA;
	buf[1] = 0x00;
	ret = i2c_write_dt(&spec, buf, 2);
	if (ret < 0) {
		return ret;
	}

	buf[0] = MCP23017_REG_IODIRB;
	buf[1] = 0x00;
	ret = i2c_write_dt(&spec, buf, 2);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int mcp23017_read_porta(const struct device *dev, uint8_t *porta_byte)
{
	const struct i2c_dt_spec spec = { .bus = dev, .addr = MCP23017_I2C_ADDR };
	uint8_t reg = MCP23017_REG_GPIOA;
	int ret;

	ret = i2c_write_dt(&spec, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&spec, porta_byte, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int mcp23017_read_portb(const struct device *dev, uint8_t *portb_byte)
{
	const struct i2c_dt_spec spec = { .bus = dev, .addr = MCP23017_I2C_ADDR };
	uint8_t reg = MCP23017_REG_GPIOB;
	int ret;

	ret = i2c_write_dt(&spec, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_read_dt(&spec, portb_byte, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
