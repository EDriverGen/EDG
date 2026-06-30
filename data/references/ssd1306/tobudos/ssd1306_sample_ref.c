#include "ssd1306_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int ssd1306_tobudos_main(void) {
    struct ssd1306_device dev;
    if (ssd1306_init(&dev, &hi2c1, SSD1306_I2C_ADDR) < 0) {
        printf("[SSD1306] init FAILED\n"); return -1;
    }
    if (ssd1306_probe(&dev) < 0) { printf("[SSD1306] probe FAILED\n"); return -1; }
    printf("[SSD1306] addr=0x%02X probe OK\n", SSD1306_I2C_ADDR);
    ssd1306_clear(&dev);
    for (int x = 0; x < SSD1306_WIDTH; x++) {
        ssd1306_set_pixel(&dev, x, 0, 1);
        ssd1306_set_pixel(&dev, x, SSD1306_HEIGHT-1, 1);
    }
    for (int y = 0; y < SSD1306_HEIGHT; y++) {
        ssd1306_set_pixel(&dev, 0, y, 1);
        ssd1306_set_pixel(&dev, SSD1306_WIDTH-1, y, 1);
    }
    for (int d = 0; d < SSD1306_HEIGHT; d++)
        ssd1306_set_pixel(&dev, d, d, 1);
    ssd1306_update(&dev);
    printf("[SSD1306] display updated: border + diagonal pattern\n");
    ssd1306_set_contrast(&dev, 0xFF);
    printf("[SSD1306] contrast set to max\n");
    ssd1306_deinit(&dev);
    return 0;
}
