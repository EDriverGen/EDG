/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SSD1306 OLED I2C Driver for XiUOS
 */
#include "ssd1306_ref.h"
#include <string.h>
#include <stdio.h>


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
  if (PrivWrite(dev->fd, buf, 2) < 0) return -1;
  return 0;
}

static int ssd1306_write_data(struct ssd1306_device *dev,
                              const uint8_t *data, int len)
{
  uint8_t buf[SSD1306_BUF_SIZE + 1];
  if (len > SSD1306_BUF_SIZE) return -1;
  buf[0] = 0x40;
  memcpy(&buf[1], data, len);
  if (PrivWrite(dev->fd, buf, len + 1) < 0) return -1;
  return 0;
}

int ssd1306_init(struct ssd1306_device *dev, const char *i2c_dev_path,
                 uint8_t addr)
{
  struct PrivIoctlCfg ioctl_cfg;
  uint16_t i2c_addr = addr;
  int i;

  if (dev == NULL || i2c_dev_path == NULL) return -1;

  dev->fd = PrivOpen(i2c_dev_path, O_RDWR);
  if (dev->fd < 0) return -1;

  ioctl_cfg.ioctl_driver_type = I2C_TYPE;
  ioctl_cfg.args = &i2c_addr;
  if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
      return -1;
    }

  dev->addr = addr;
  memset(dev->buffer, 0, SSD1306_BUF_SIZE);

  for (i = 0; i < (int)sizeof(ssd1306_init_cmds); i++)
    if (ssd1306_write_cmd(dev, ssd1306_init_cmds[i]) < 0) return -1;

  return 0;
}

void ssd1306_deinit(struct ssd1306_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      ssd1306_display_off(dev);
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int ssd1306_probe(struct ssd1306_device *dev)
{
  if (dev == NULL || dev->fd < 0) return -1;
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
