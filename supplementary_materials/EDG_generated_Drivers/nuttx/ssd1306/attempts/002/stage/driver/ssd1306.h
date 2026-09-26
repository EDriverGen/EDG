#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

struct i2c_master_s;

struct ssd1306_dev_s {
    struct i2c_master_s *bus;
    uint8_t addr;
};

int ssd1306_init(struct ssd1306_dev_s *dev, struct i2c_master_s *bus);
int ssd1306_write_display_data(struct ssd1306_dev_s *dev, const uint8_t *data, uint16_t len);

#endif /* SSD1306_H */