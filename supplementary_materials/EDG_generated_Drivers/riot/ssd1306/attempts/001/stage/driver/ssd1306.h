#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include <periph/i2c.h>

#include "riot.h"
typedef struct {
    i2c_t bus;
    uint16_t addr;
} ssd1306_t;

int ssd1306_init(ssd1306_t *dev, i2c_t bus, uint16_t addr);
int ssd1306_write_display_data(ssd1306_t *dev, const uint8_t *data, size_t len);

#endif /* SSD1306_H */
