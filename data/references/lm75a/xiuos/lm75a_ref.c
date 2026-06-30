/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Driver for XiUOS
 *
 * Uses XiUOS POSIX-like I2C interface:
 *   PrivOpen() -> PrivIoctl(OPE_INT) to set address -> PrivWrite/PrivRead
 */
#include "lm75a_ref.h"
#include <string.h>
#include <stdio.h>

/* ---- Internal helpers ---- */

static int lm75a_write_then_read(struct lm75a_device *dev,
                                 const uint8_t *wbuf, int wlen,
                                 uint8_t *rbuf, int rlen)
{
  if (PrivWrite(dev->fd, wbuf, wlen) < 0)
    {
      return -1;
    }

  if (rlen > 0)
    {
      if (PrivRead(dev->fd, rbuf, rlen) < 0)
        {
          return -1;
        }
    }

  return 0;
}

static int lm75a_read_reg(struct lm75a_device *dev,
                          uint8_t reg, uint8_t *buf, int len)
{
  return lm75a_write_then_read(dev, &reg, 1, buf, len);
}

static int lm75a_write_reg(struct lm75a_device *dev,
                           uint8_t reg, const uint8_t *buf, int len)
{
  uint8_t frame[3]; /* 1 reg + max 2 data */

  if (len > 2) return -1;

  frame[0] = reg;
  memcpy(&frame[1], buf, len);

  if (PrivWrite(dev->fd, frame, len + 1) < 0)
    {
      return -1;
    }

  return 0;
}

static int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
  int32_t value = (int32_t)(raw >> 5);

  return value * LM75A_TEMP_STEP_MC;
}

/* ---- Public API ---- */

int lm75a_init(struct lm75a_device *dev,
               const char *i2c_dev_path,
               uint16_t addr)
{
  struct PrivIoctlCfg ioctl_cfg;
  uint16_t i2c_addr = addr;

  if (dev == NULL || i2c_dev_path == NULL) return -1;

  dev->fd = PrivOpen(i2c_dev_path, O_RDWR);
  if (dev->fd < 0)
    {
      printf("lm75a: open %s failed\n", i2c_dev_path);
      return -1;
    }

  /* Set I2C slave address via ioctl */
  ioctl_cfg.ioctl_driver_type = I2C_TYPE;
  ioctl_cfg.args = &i2c_addr;

  if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0)
    {
      printf("lm75a: ioctl set addr 0x%02X failed\n", addr);
      PrivClose(dev->fd);
      dev->fd = -1;
      return -1;
    }

  dev->addr = addr;
  return 0;
}

void lm75a_deinit(struct lm75a_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int lm75a_probe(struct lm75a_device *dev)
{
  uint8_t conf;

  if (dev == NULL || dev->fd < 0) return -1;

  if (lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1) < 0)
    {
      return -1;
    }

  if ((conf & 0xE0) != 0)
    {
      return -1;
    }

  return 0;
}

int lm75a_read_temperature(struct lm75a_device *dev,
                           int32_t *temp_mcelsius)
{
  uint8_t buf[2];
  int16_t raw;

  if (dev == NULL || temp_mcelsius == NULL) return -1;

  if (lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2) < 0)
    {
      return -1;
    }

  raw = (int16_t)((buf[0] << 8) | buf[1]);
  *temp_mcelsius = lm75a_raw_to_mcelsius(raw);

  return 0;
}

int lm75a_read_config(struct lm75a_device *dev, uint8_t *config)
{
  if (dev == NULL || config == NULL) return -1;

  return lm75a_read_reg(dev, LM75A_REG_CONF, config, 1);
}

int lm75a_write_config(struct lm75a_device *dev, uint8_t config)
{
  return lm75a_write_reg(dev, LM75A_REG_CONF, &config, 1);
}
