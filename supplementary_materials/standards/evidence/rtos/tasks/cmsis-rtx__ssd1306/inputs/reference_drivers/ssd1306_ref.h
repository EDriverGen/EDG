#ifndef SSD1306_CMSIS_RTX_REF_H
#define SSD1306_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define SSD1306_I2C_ADDR  0x3CU
#define SSD1306_WIDTH     128U
#define SSD1306_HEIGHT    64U
#define SSD1306_BUF_SIZE  (SSD1306_WIDTH * SSD1306_HEIGHT / 8U)

struct ssd1306_device {
    I2C_HandleTypeDef *bus;
    uint8_t addr;
    uint8_t buffer[SSD1306_BUF_SIZE];
};

int ssd1306_init(struct ssd1306_device *dev, I2C_HandleTypeDef *bus, uint8_t addr);
void ssd1306_deinit(struct ssd1306_device *dev);
int ssd1306_probe(struct ssd1306_device *dev);
int ssd1306_display_on(struct ssd1306_device *dev);
int ssd1306_display_off(struct ssd1306_device *dev);
int ssd1306_clear(struct ssd1306_device *dev);
void ssd1306_set_pixel(struct ssd1306_device *dev, uint8_t x, uint8_t y, uint8_t on);
int ssd1306_update(struct ssd1306_device *dev);
int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast);
int ssd1306_write_frame(struct ssd1306_device *dev, const uint8_t *data, uint16_t len);

#endif
