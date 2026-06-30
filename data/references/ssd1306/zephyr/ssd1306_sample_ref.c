/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SSD1306 OLED Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdio.h>
#include "ssd1306_ref.h"

int ssd1306_sample(void)
{
  struct ssd1306_device dev;
  const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
  int x, y, ret;

  if (!device_is_ready(i2c_dev)) { printf("ERROR: I2C not ready\n"); return -1; }

  ret = ssd1306_init(&dev, i2c_dev, SSD1306_I2C_ADDR);
  if (ret < 0) { printf("ERROR: ssd1306 init failed\n"); return -1; }

  ret = ssd1306_probe(&dev);
  if (ret < 0) { printf("ERROR: ssd1306 probe failed\n"); ssd1306_deinit(&dev); return -1; }

  printf("[SSD1306] addr=0x%02X probe OK\n", SSD1306_I2C_ADDR);

  ssd1306_clear(&dev);

  for (x = 0; x < SSD1306_WIDTH; x++)
    { ssd1306_set_pixel(&dev, x, 0, 1); ssd1306_set_pixel(&dev, x, SSD1306_HEIGHT-1, 1); }
  for (y = 0; y < SSD1306_HEIGHT; y++)
    { ssd1306_set_pixel(&dev, 0, y, 1); ssd1306_set_pixel(&dev, SSD1306_WIDTH-1, y, 1); }
  for (x = 0; x < SSD1306_HEIGHT; x++)
    ssd1306_set_pixel(&dev, x, x, 1);

  ret = ssd1306_update(&dev);
  if (ret < 0) { printf("ERROR: display update failed\n"); ssd1306_deinit(&dev); return -1; }

  printf("[SSD1306] display updated: border + diagonal pattern\n");
  ssd1306_set_contrast(&dev, 0xFF);
  printf("[SSD1306] contrast set to max\n");

  ssd1306_deinit(&dev);
  return 0;
}
