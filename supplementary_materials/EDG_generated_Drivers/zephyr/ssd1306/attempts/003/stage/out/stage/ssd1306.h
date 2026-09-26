#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/device.h>

#define SSD1306_I2C_ADDR 0x3C

#include <zephyr/drivers/i2c.h>
struct device;

int ssd1306_init(const struct device *dev);
int ssd1306_write_display_data(const struct device *dev, const uint8_t *data, size_t len);

#endif /* SSD1306_H */
