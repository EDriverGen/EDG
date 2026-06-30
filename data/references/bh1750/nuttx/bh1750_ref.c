/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference driver
 */
#include "bh1750_ref.h"
#include <errno.h>
#include <unistd.h>

#define BH1750_CMD_POWER_DOWN  0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

/* ---- Internal helpers ---- */

static bool bh1750_is_valid_mode(uint8_t mode)
{
  switch (mode)
    {
    case BH1750_CONT_H_RES_MODE:
    case BH1750_CONT_H_RES_MODE2:
    case BH1750_CONT_L_RES_MODE:
    case BH1750_ONE_H_RES_MODE:
    case BH1750_ONE_H_RES_MODE2:
    case BH1750_ONE_L_RES_MODE:
      return true;
    default:
      return false;
    }
}

static int bh1750_write_cmd(FAR struct bh1750_device *dev, uint8_t cmd)
{
  return i2c_write(dev->i2c, &dev->config, &cmd, 1);
}

static int bh1750_read_bytes(FAR struct bh1750_device *dev,
                             FAR uint8_t *buf, int len)
{
  return i2c_read(dev->i2c, &dev->config, buf, len);
}

static int bh1750_get_wait_time_us(uint8_t mode)
{
  switch (mode)
    {
    case BH1750_CONT_H_RES_MODE:
    case BH1750_CONT_H_RES_MODE2:
    case BH1750_ONE_H_RES_MODE:
    case BH1750_ONE_H_RES_MODE2:
      return 180000;  /* 180 ms */
    case BH1750_CONT_L_RES_MODE:
    case BH1750_ONE_L_RES_MODE:
      return 24000;   /* 24 ms */
    default:
      return 180000;
    }
}

/* ---- Public API ---- */

int bh1750_init(FAR struct bh1750_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;
  dev->config.frequency = BH1750_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;
  dev->mode = BH1750_ONE_H_RES_MODE;

  return 0;
}

int bh1750_set_mode(FAR struct bh1750_device *dev, uint8_t mode)
{
  if (dev == NULL || !bh1750_is_valid_mode(mode))
    {
      return -EINVAL;
    }

  dev->mode = mode;
  return 0;
}

int bh1750_probe(FAR struct bh1750_device *dev)
{
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
  if (ret < 0)
    {
      return ret;
    }

  return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

int bh1750_read_raw(FAR struct bh1750_device *dev, FAR uint16_t *raw)
{
  uint8_t data[2];
  int ret;

  if (dev == NULL || raw == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
  if (ret < 0) return ret;

  ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
  if (ret < 0) return ret;

  ret = bh1750_write_cmd(dev, dev->mode);
  if (ret < 0) return ret;

  usleep(bh1750_get_wait_time_us(dev->mode));

  ret = bh1750_read_bytes(dev, data, 2);
  if (ret < 0) return ret;

  *raw = ((uint16_t)data[0] << 8) | data[1];
  return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw)
{
  return ((uint32_t)raw * 1000U) / 12U;
}

int bh1750_read_lux_x100(FAR struct bh1750_device *dev, FAR uint32_t *lux_x100)
{
  uint16_t raw;
  int ret;

  if (dev == NULL || lux_x100 == NULL)
    {
      return -EINVAL;
    }

  ret = bh1750_read_raw(dev, &raw);
  if (ret < 0) return ret;

  *lux_x100 = bh1750_raw_to_lux_x100(raw);
  return 0;
}
