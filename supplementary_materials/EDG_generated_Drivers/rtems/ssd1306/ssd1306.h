#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>

struct ssd1306_device {
    int fd;
    uint8_t addr;
};

int ssd1306_init(struct ssd1306_device *dev, void *bus_handle);
int ssd1306_write_display_data(struct ssd1306_device *dev, const uint8_t *data, size_t len);

#endif /* SSD1306_H */