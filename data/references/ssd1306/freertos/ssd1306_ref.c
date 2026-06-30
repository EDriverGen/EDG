#include "ssd1306_ref.h"


static int freertos_i2c_write(I2C_HandleTypeDef *bus, uint16_t addr,
                              const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Transmit(bus, (uint16_t)(addr << 1), (uint8_t *)data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int freertos_i2c_read(I2C_HandleTypeDef *bus, uint16_t addr,
                             uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Receive(bus, (uint16_t)(addr << 1), data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int freertos_i2c_write_read(I2C_HandleTypeDef *bus, uint16_t addr,
                                   const uint8_t *wdata, uint16_t wlen,
                                   uint8_t *rdata, uint16_t rlen)
{
    HAL_StatusTypeDef status;
    uint16_t mem_addr;

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;

    if (wlen == 1) {
        status = HAL_I2C_Mem_Read(bus, (uint16_t)(addr << 1), wdata[0],
                                  I2C_MEMADD_SIZE_8BIT, rdata, rlen, 100);
        return (status == HAL_OK) ? 0 : -1;
    }

    if (wlen == 2) {
        mem_addr = (uint16_t)(((uint16_t)wdata[0] << 8) | wdata[1]);
        status = HAL_I2C_Mem_Read(bus, (uint16_t)(addr << 1), mem_addr,
                                  I2C_MEMADD_SIZE_16BIT, rdata, rlen, 100);
        return (status == HAL_OK) ? 0 : -1;
    }

    status = HAL_I2C_Master_Transmit(bus, (uint16_t)(addr << 1), (uint8_t *)wdata, wlen, 100);
    if (status != HAL_OK) return -1;
    status = HAL_I2C_Master_Receive(bus, (uint16_t)(addr << 1), rdata, rlen, 100);
    return (status == HAL_OK) ? 0 : -1;
}

#include <string.h>

static const uint8_t ssd1306_init_cmds[] = {
    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
    0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
    0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
};

static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    return freertos_i2c_write(dev->bus, dev->addr, buf, 2);
}

static int ssd1306_write_data(struct ssd1306_device *dev, const uint8_t *data, int len) {
    uint8_t buf[SSD1306_BUF_SIZE + 1];
    if (len > SSD1306_BUF_SIZE) return -1;
    buf[0] = 0x40;
    memcpy(&buf[1], data, len);
    return freertos_i2c_write(dev->bus, dev->addr, buf, len + 1);
}

int ssd1306_init(struct ssd1306_device *dev, I2C_HandleTypeDef *bus, uint8_t addr) {
    if (!dev || !bus) return -1;
    dev->bus = bus; dev->addr = addr;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    for (int i = 0; i < (int)sizeof(ssd1306_init_cmds); i++)
        if (ssd1306_write_cmd(dev, ssd1306_init_cmds[i]) < 0) return -1;
    return 0;
}

void ssd1306_deinit(struct ssd1306_device *dev) {
    if (dev) ssd1306_write_cmd(dev, 0xAE);
}

int ssd1306_probe(struct ssd1306_device *dev) {
    if (!dev) return -1;
    return ssd1306_write_cmd(dev, 0xAE);
}

int ssd1306_clear(struct ssd1306_device *dev) {
    if (!dev) return -1;
    memset(dev->buffer, 0, SSD1306_BUF_SIZE);
    return ssd1306_update(dev);
}

void ssd1306_set_pixel(struct ssd1306_device *dev, uint8_t x, uint8_t y, uint8_t on) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    if (on) dev->buffer[x + (y/8)*SSD1306_WIDTH] |= (1 << (y & 7));
    else    dev->buffer[x + (y/8)*SSD1306_WIDTH] &= ~(1 << (y & 7));
}

int ssd1306_update(struct ssd1306_device *dev) {
    if (!dev) return -1;
    ssd1306_write_cmd(dev, 0x21); ssd1306_write_cmd(dev, 0x00); ssd1306_write_cmd(dev, 0x7F);
    ssd1306_write_cmd(dev, 0x22); ssd1306_write_cmd(dev, 0x00); ssd1306_write_cmd(dev, 0x07);
    return ssd1306_write_data(dev, dev->buffer, SSD1306_BUF_SIZE);
}

int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast) {
    if (ssd1306_write_cmd(dev, 0x81) < 0) return -1;
    return ssd1306_write_cmd(dev, contrast);
}
