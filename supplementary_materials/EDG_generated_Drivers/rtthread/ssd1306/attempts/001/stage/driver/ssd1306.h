#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <drivers/dev_i2c.h>

#define SSD1306_I2C_ADDR 0x3C

struct rt_i2c_bus_device;

struct ssd1306_device {
    struct rt_i2c_bus_device *bus;
    uint8_t i2c_addr;
};

int ssd1306_init(struct ssd1306_device *dev, struct rt_i2c_bus_device *bus);
int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, uint16_t len);

#endif /* SSD1306_H */