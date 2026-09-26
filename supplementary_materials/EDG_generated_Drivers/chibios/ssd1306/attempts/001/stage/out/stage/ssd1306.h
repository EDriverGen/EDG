#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include "hal.h"

#include "hal_i2c.h"
typedef struct {
    I2CDriver *i2c;
    uint8_t addr;
} SSD1306Driver;

void ssd1306_init(SSD1306Driver *dev, I2CDriver *bus_handle);
void ssd1306_write_display_data(SSD1306Driver *dev, const uint8_t *data, size_t len);

#endif /* SSD1306_H */
