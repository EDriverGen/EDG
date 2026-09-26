#include "ssd1306.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
static int ssd1306_write_cmd(const struct device *dev, uint8_t cmd)
{
	const struct i2c_dt_spec spec = { .bus = dev, .addr = SSD1306_I2C_ADDR };
	return i2c_write_dt(&spec, &cmd, 1);
}

static int ssd1306_write_cmd2(const struct device *dev, uint8_t cmd1, uint8_t cmd2)
{
	uint8_t buf[2] = { cmd1, cmd2 };
	const struct i2c_dt_spec spec = { .bus = dev, .addr = SSD1306_I2C_ADDR };
	return i2c_write_dt(&spec, buf, 2);
}

static int ssd1306_write_cmd3(const struct device *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
	uint8_t buf[3] = { cmd1, cmd2, cmd3 };
	const struct i2c_dt_spec spec = { .bus = dev, .addr = SSD1306_I2C_ADDR };
	return i2c_write_dt(&spec, buf, 3);
}

int ssd1306_init(const struct device *dev)
{
	int ret;

	ret = ssd1306_write_cmd(dev, 0xAE);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0xD5, 0x80);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0xA8, 0x3F);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0xD3, 0x00);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0x40);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0xA1);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0xC8);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0xDA, 0x12);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0x81, 0x7F);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0xA4);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0xA6);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0x2E);
	if (ret) return ret;
	ret = ssd1306_write_cmd2(dev, 0x20, 0x00);
	if (ret) return ret;
	ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
	if (ret) return ret;
	ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
	if (ret) return ret;
	ret = ssd1306_write_cmd(dev, 0xAF);
	if (ret) return ret;

	return 0;
}

int ssd1306_write_display_data(const struct device *dev, const uint8_t *data, size_t len)
{
	int ret;
	uint8_t cmd;
	const struct i2c_dt_spec spec = { .bus = dev, .addr = SSD1306_I2C_ADDR };

	cmd = 0x21;
	ret = i2c_write_dt(&spec, &cmd, 1);
	if (ret) return ret;

	cmd = 0x22;
	ret = i2c_write_dt(&spec, &cmd, 1);
	if (ret) return ret;

	return i2c_write_dt(&spec, data, len);
}
