#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "i2c_if.h"

#define SSD1306_I2C_ADDR 0x3C
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES 8

#include "openharmony_liteosm.h"
struct ssd1306_dev {
    DevHandle bus_handle;
    uint8_t i2c_addr;
};

int ssd1306_init(struct ssd1306_dev *dev, DevHandle bus_handle);
int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *buf, uint16_t len);

#endif
