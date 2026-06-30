/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SSD1306 OLED I2C Driver for RIOT
 */
#ifndef __SSD1306_REF_H
#define __SSD1306_REF_H

#include "periph/i2c.h"
#include "ztimer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_I2C_ADDR          0x3C
#define SSD1306_WIDTH             128
#define SSD1306_HEIGHT            64
#define SSD1306_BUF_SIZE          (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

struct ssd1306_device
{
  i2c_t bus;
  uint8_t addr;
  uint8_t buffer[SSD1306_BUF_SIZE];
};

int ssd1306_init(struct ssd1306_device *dev, i2c_t bus, uint8_t addr);
void ssd1306_deinit(struct ssd1306_device *dev);
int ssd1306_probe(struct ssd1306_device *dev);
int ssd1306_display_on(struct ssd1306_device *dev);
int ssd1306_display_off(struct ssd1306_device *dev);
int ssd1306_clear(struct ssd1306_device *dev);
void ssd1306_set_pixel(struct ssd1306_device *dev, uint8_t x, uint8_t y, uint8_t on);
int ssd1306_update(struct ssd1306_device *dev);
int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_REF_H */
