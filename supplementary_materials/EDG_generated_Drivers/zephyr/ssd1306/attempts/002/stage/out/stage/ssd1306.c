#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "ssd1306.h"

#include <zephyr/sys/byteorder.h>
#define SSD1306_I2C_ADDR 0x3C

static const struct device *g_eval_dev;

int ssd1306_init(void)
{
	int ret;
	uint8_t cmds[] = {
		0xAE,
		0xD5, 0x80,
		0xA8, 0x3F,
		0xD3, 0x00,
		0x40,
		0xA1,
		0xC8,
		0xDA, 0x12,
		0x81, 0x7F,
		0xA4,
		0xA6,
		0x2E,
		0x20, 0x00,
		0x21, 0x00, 0x7F,
		0x22, 0x00, 0x07
	};
	struct i2c_dt_spec spec = { .bus = g_eval_dev, .addr = SSD1306_I2C_ADDR };

	for (size_t i = 0; i < sizeof(cmds); ) {
		uint8_t cmd = cmds[i];
		uint8_t len;
		if (cmd == 0xAE || cmd == 0x40 || cmd == 0xA1 || cmd == 0xC8 || cmd == 0xA4 || cmd == 0xA6 || cmd == 0x2E) {
			len = 1;
		} else if (cmd == 0xD5 || cmd == 0xA8 || cmd == 0xD3 || cmd == 0xDA || cmd == 0x81 || cmd == 0x20 || cmd == 0x21 || cmd == 0x22) {
			len = (cmd == 0x21 || cmd == 0x22) ? 3 : 2;
		} else {
			len = 1;
		}
		ret = i2c_write_dt(&spec, &cmds[i], len);
		if (ret < 0) {
			return ret;
		}
		i += len;
	}

	return 0;
}

int ssd1306_write_display_data(const uint8_t *data, size_t len)
{
	int ret;
	struct i2c_dt_spec spec = { .bus = g_eval_dev, .addr = SSD1306_I2C_ADDR };
	uint8_t col_cmd[] = {0x21, 0x00, 0x7F};
	uint8_t page_cmd[] = {0x22, 0x00, 0x07};

	ret = i2c_write_dt(&spec, col_cmd, sizeof(col_cmd));
	if (ret < 0) {
		return ret;
	}

	ret = i2c_write_dt(&spec, page_cmd, sizeof(page_cmd));
	if (ret < 0) {
		return ret;
	}

	ret = i2c_write_dt(&spec, data, len);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
