#include "ssd1306_ref.h"
#include <string.h>

static const uint8_t ssd1306_init_cmds[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
};

static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    if (dev == 0 || dev->bus == 0) return -1;
    return HAL_I2C_Master_Transmit(dev->bus, (uint16_t)(dev->addr << 1), buf, 2, 100) == HAL_OK ? 0 : -1;
}

int ssd1306_write_frame(struct ssd1306_device *dev, const uint8_t *data, uint16_t len)
{
    static uint8_t framed[SSD1306_BUF_SIZE + 1];
    if (dev == 0 || data == 0 || len == 0 || len > SSD1306_BUF_SIZE) return -1;
    framed[0] = 0x40;
    memcpy(&framed[1], data, len);
    return HAL_I2C_Master_Transmit(dev->bus, (uint16_t)(dev->addr << 1), framed, (uint16_t)(len + 1), 100) == HAL_OK ? 0 : -1;
}

int ssd1306_init(struct ssd1306_device *dev, I2C_HandleTypeDef *bus, uint8_t addr)
{
    if (dev == 0 || bus == 0) return -1;
    if (HAL_I2C_Init(bus) != HAL_OK) return -1;
    dev->bus = bus;
    dev->addr = addr;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    for (int i = 0; i < (int)sizeof(ssd1306_init_cmds); i++) {
        if (ssd1306_write_cmd(dev, ssd1306_init_cmds[i]) != 0) return -1;
    }
    return 0;
}

void ssd1306_deinit(struct ssd1306_device *dev)
{
    if (dev != 0) (void)ssd1306_display_off(dev);
}

int ssd1306_probe(struct ssd1306_device *dev) { return ssd1306_write_cmd(dev, 0xAE); }
int ssd1306_display_on(struct ssd1306_device *dev) { return ssd1306_write_cmd(dev, 0xAF); }
int ssd1306_display_off(struct ssd1306_device *dev) { return ssd1306_write_cmd(dev, 0xAE); }

int ssd1306_clear(struct ssd1306_device *dev)
{
    if (dev == 0) return -1;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    return ssd1306_update(dev);
}

void ssd1306_set_pixel(struct ssd1306_device *dev, uint8_t x, uint8_t y, uint8_t on)
{
    if (dev == 0 || x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    if (on) dev->buffer[x + (y / 8U) * SSD1306_WIDTH] |= (uint8_t)(1U << (y & 7U));
    else dev->buffer[x + (y / 8U) * SSD1306_WIDTH] &= (uint8_t)~(1U << (y & 7U));
}

int ssd1306_update(struct ssd1306_device *dev)
{
    if (dev == 0) return -1;
    if (ssd1306_write_cmd(dev, 0x21) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x00) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x22) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x00) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x07) != 0) return -1;
    return ssd1306_write_frame(dev, dev->buffer, SSD1306_BUF_SIZE);
}

int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast)
{
    if (ssd1306_write_cmd(dev, 0x81) != 0) return -1;
    return ssd1306_write_cmd(dev, contrast);
}
