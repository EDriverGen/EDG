#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    void *bus_handle;
    uint8_t i2c_addr;
} SSD1306Driver;

void ssd1306_init(SSD1306Driver *dev, void *bus_handle);
void ssd1306_write_display_data(SSD1306Driver *dev, const uint8_t *data, size_t len);

#endif