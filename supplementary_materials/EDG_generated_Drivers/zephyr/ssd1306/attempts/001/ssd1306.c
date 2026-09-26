#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <stddef.h>
#include "ssd1306.h"

#include <zephyr/sys/byteorder.h>
#define SSD1306_CMD 0x00
#define SSD1306_DATA 0x40

static int ssd1306_write_cmd(const struct device *dev, uint8_t cmd)
{
	uint8_t buf[2] = {SSD1306_CMD, cmd};
	struct i2c_dt_spec spec = {.bus = dev, .addr = SSD1306_I2C_ADDR};
	return i2c_write_dt(&spec, buf, 2);
}

static int ssd1306_write_cmd2(const struct device *dev, uint8_t cmd, uint8_t arg)
{
	uint8_t buf[3] = {SSD1306_CMD, cmd, arg};
	struct i2c_dt_spec spec = {.bus = dev, .addr = SSD1306_I2C_ADDR};
	return i2c_write_dt(&spec, buf, 3);
}

static int ssd1306_write_cmd3(const struct device *dev, uint8_t cmd, uint8_t arg1, uint8_t arg2)
{
	uint8_t buf[4] = {SSD1306_CMD, cmd, arg1, arg2};
	struct i2c_dt_spec spec = {.bus = dev, .addr = SSD1306_I2C_ADDR};
	return i2c_write_dt(&spec, buf, 4);
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
	struct i2c_dt_spec spec = {.bus = dev, .addr = SSD1306_I2C_ADDR};

	ret = ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F);
	if (ret) return ret;
	ret = ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07);
	if (ret) return ret;

	uint8_t *buf = NULL;
	if (len > 0) {
		buf = malloc(len + 1);
		if (!buf) return -ENOMEM;
		buf[0] = SSD1306_DATA;
		for (size_t i = 0; i < len; i++) {
			buf[i + 1] = data[i];
		}
		ret = i2c_write_dt(&spec, buf, len + 1);
		free(buf);
	}
	return ret;
}
