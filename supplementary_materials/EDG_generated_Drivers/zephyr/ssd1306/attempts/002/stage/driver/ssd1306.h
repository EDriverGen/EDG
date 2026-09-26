#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stddef.h>

int ssd1306_init(void);
int ssd1306_write_display_data(const uint8_t *data, size_t len);

#endif /* SSD1306_H */