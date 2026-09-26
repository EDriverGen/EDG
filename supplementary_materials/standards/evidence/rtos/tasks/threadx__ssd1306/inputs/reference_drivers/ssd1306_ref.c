/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SSD1306 OLED I2C Driver for ThreadX
 */
#include "ssd1306_ref.h"
#include <string.h>


static int ssd1306_threadx_i2c_write(struct ssd1306_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int ssd1306_threadx_i2c_read(struct ssd1306_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int ssd1306_threadx_i2c_write_read(struct ssd1306_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define SSD1306_I2C_WRITE(_bus, _addr, _data, _len) \
    ssd1306_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define SSD1306_I2C_READ(_bus, _addr, _data, _len) \
    ssd1306_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define SSD1306_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    ssd1306_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))


static const uint8_t ssd1306_init_cmds[] =
{
  0xAE,       /* Display OFF */
  0xD5, 0x80, /* Set display clock div */
  0xA8, 0x3F, /* Set multiplex ratio (64-1) */
  0xD3, 0x00, /* Set display offset */
  0x40,       /* Set start line 0 */
  0x8D, 0x14, /* Enable charge pump */
  0x20, 0x00, /* Horizontal addressing mode */
  0xA1,       /* Segment remap (col127=SEG0) */
  0xC8,       /* COM scan remapped */
  0xDA, 0x12, /* Set COM pins */
  0x81, 0xCF, /* Set contrast */
  0xD9, 0xF1, /* Set precharge period */
  0xDB, 0x40, /* Set VCOMH deselect */
  0xA4,       /* Display follows RAM */
  0xA6,       /* Normal display */
  0xAF,       /* Display ON */
};


static int ssd1306_write_cmd(struct ssd1306_device *dev, uint8_t cmd)
{
  uint8_t buf[2] = {0x00, cmd};
  return SSD1306_I2C_WRITE(dev->bus_context, dev->addr, buf, 2);
}

static int ssd1306_write_data(struct ssd1306_device *dev,
                              const uint8_t *data, int len)
{
  uint8_t buf[SSD1306_BUF_SIZE + 1];
  if (len > SSD1306_BUF_SIZE) return -1;
  buf[0] = 0x40;
  memcpy(&buf[1], data, len);
  return SSD1306_I2C_WRITE(dev->bus_context, dev->addr, buf, len + 1);
}

int ssd1306_init(struct ssd1306_device *dev, void *bus_context, const struct ssd1306_i2c_ops *ops, uint8_t addr)
{
  int i;
  if (dev == NULL || bus_context == NULL) return -1;

  dev->bus_context = bus_context;
    dev->ops = ops;
  dev->addr = addr;
  memset(dev->buffer, 0, SSD1306_BUF_SIZE);

  for (i = 0; i < (int)sizeof(ssd1306_init_cmds); i++)
    if (ssd1306_write_cmd(dev, ssd1306_init_cmds[i]) < 0) return -1;

  return 0;
}

void ssd1306_deinit(struct ssd1306_device *dev)
{
  if (dev != NULL) ssd1306_display_off(dev);
}

int ssd1306_probe(struct ssd1306_device *dev)
{
  if (dev == NULL) return -1;
  return ssd1306_write_cmd(dev, 0xAE);
}

int ssd1306_display_on(struct ssd1306_device *dev)  { return ssd1306_write_cmd(dev, 0xAF); }
int ssd1306_display_off(struct ssd1306_device *dev) { return ssd1306_write_cmd(dev, 0xAE); }

int ssd1306_clear(struct ssd1306_device *dev)
{
  if (dev == NULL) return -1;
  memset(dev->buffer, 0, SSD1306_BUF_SIZE);
  return ssd1306_update(dev);
}

void ssd1306_set_pixel(struct ssd1306_device *dev,
                       uint8_t x, uint8_t y, uint8_t on)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
  if (on) dev->buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
  else    dev->buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
}

int ssd1306_update(struct ssd1306_device *dev)
{
  if (dev == NULL) return -1;
  if (ssd1306_write_cmd(dev, 0x21) < 0) return -1;
  if (ssd1306_write_cmd(dev, 0x00) < 0) return -1;
  if (ssd1306_write_cmd(dev, 0x7F) < 0) return -1;
  if (ssd1306_write_cmd(dev, 0x22) < 0) return -1;
  if (ssd1306_write_cmd(dev, 0x00) < 0) return -1;
  if (ssd1306_write_cmd(dev, 0x07) < 0) return -1;
  return ssd1306_write_data(dev, dev->buffer, SSD1306_BUF_SIZE);
}

int ssd1306_set_contrast(struct ssd1306_device *dev, uint8_t contrast)
{
  if (ssd1306_write_cmd(dev, 0x81) < 0) return -1;
  return ssd1306_write_cmd(dev, contrast);
}
