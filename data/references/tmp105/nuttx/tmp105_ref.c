/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference driver
 */
#include "tmp105_ref.h"
#include <errno.h>
#include <string.h>

/* ---- Internal helpers ---- */

static int tmp105_read_reg(FAR struct tmp105_device *dev,
                           uint8_t reg,
                           FAR uint8_t *buf, int len)
{
  return i2c_writeread(dev->i2c, &dev->config, &reg, 1, buf, len);
}

static int tmp105_write_reg(FAR struct tmp105_device *dev,
                            uint8_t reg,
                            FAR const uint8_t *buf, int len)
{
  uint8_t frame[3];

  if (len > 2)
    {
      return -EINVAL;
    }

  frame[0] = reg;
  memcpy(&frame[1], buf, len);
  return i2c_write(dev->i2c, &dev->config, frame, len + 1);
}

/*
 * TMP105 12-bit mode raw format:
 *   [15:4] = 12-bit signed, 0.0625 C per LSB
 *   [3:0]  = 0
 *
 * Lower resolutions use fewer bits. Shift amount depends on resolution.
 */
static int32_t tmp105_raw_to_mcelsius(int16_t raw, uint8_t resolution)
{
  int shift_bits;
  int32_t step_mc;

  switch (resolution)
    {
    case TMP105_RES_9BIT:
      shift_bits = 7; step_mc = 500; break;
    case TMP105_RES_10BIT:
      shift_bits = 6; step_mc = 250; break;
    case TMP105_RES_11BIT:
      shift_bits = 5; step_mc = 125; break;
    case TMP105_RES_12BIT:
    default:
      shift_bits = 4; step_mc = 625; break;
    }

  if (resolution >= TMP105_RES_12BIT)
    return (int32_t)(raw >> shift_bits) * step_mc / 10;
  return (int32_t)(raw >> shift_bits) * step_mc;
}

/* ---- Public API ---- */

int tmp105_init(FAR struct tmp105_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;
  dev->config.frequency = TMP105_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;
  dev->resolution = TMP105_RES_12BIT;

  return 0;
}

int tmp105_probe(FAR struct tmp105_device *dev)
{
  uint8_t conf;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1);
  if (ret < 0)
    {
      return ret;
    }

  return 0;
}

int tmp105_set_resolution(FAR struct tmp105_device *dev, uint8_t res)
{
  uint8_t conf;
  int ret;

  if (dev == NULL || res > TMP105_RES_12BIT)
    {
      return -EINVAL;
    }

  ret = tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1);
  if (ret < 0) return ret;

  conf &= ~(TMP105_CONF_R0 | TMP105_CONF_R1);
  conf |= (res << 5);

  ret = tmp105_write_reg(dev, TMP105_REG_CONF, &conf, 1);
  if (ret < 0) return ret;

  dev->resolution = res;
  return 0;
}

int tmp105_read_temperature(FAR struct tmp105_device *dev,
                            FAR int32_t *temp_mcelsius)
{
  uint8_t buf[2];
  int16_t raw;
  int ret;

  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  ret = tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2);
  if (ret < 0) return ret;

  raw = (int16_t)((buf[0] << 8) | buf[1]);
  *temp_mcelsius = tmp105_raw_to_mcelsius(raw, dev->resolution);

  return 0;
}

int tmp105_read_config(FAR struct tmp105_device *dev,
                       FAR uint8_t *config)
{
  if (dev == NULL || config == NULL)
    {
      return -EINVAL;
    }

  return tmp105_read_reg(dev, TMP105_REG_CONF, config, 1);
}

int tmp105_write_config(FAR struct tmp105_device *dev,
                        uint8_t config)
{
  return tmp105_write_reg(dev, TMP105_REG_CONF, &config, 1);
}
