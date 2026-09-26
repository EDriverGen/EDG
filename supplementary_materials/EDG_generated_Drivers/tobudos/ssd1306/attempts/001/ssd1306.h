#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>

struct ssd1306_dev {
    void *bus_handle;
    uint8_t i2c_addr;
};

int ssd1306_init(struct ssd1306_dev *dev, void *bus_handle);
int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *data, uint16_t len);

#endif /* SSD1306_H */