/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for NuttX
 */
#include "tmp421_ref.h"
#include <errno.h>
#include <string.h>

/* ---- Internal helpers ---- */

static int tmp421_read_register(FAR struct tmp421_device *dev,
                                uint8_t reg, FAR uint8_t *value)
{
  return i2c_writeread(dev->i2c, &dev->config, &reg, 1, value, 1);
}

static int tmp421_write_register(FAR struct tmp421_device *dev,
                                 uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  return i2c_write(dev->i2c, &dev->config, frame, 2);
}

/*
 * TMP421 temperature: 12-bit signed, hi[7:0] = int, lo[7:4] = frac.
 * Step = 0.0625 C = 62.5 mC.
 */
static int32_t tmp421_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
  int16_t raw = (int16_t)((hi << 8) | lo);
  int32_t frac = (int32_t)(raw >> 4);

  return (frac * 1000) / 16;
}

static int tmp421_read_temp_pair(FAR struct tmp421_device *dev,
                                 uint8_t reg_hi, uint8_t reg_lo,
                                 FAR int32_t *temp_mcelsius)
{
  uint8_t hi, lo;
  int ret;

  ret = tmp421_read_register(dev, reg_hi, &hi);
  if (ret < 0) return ret;

  ret = tmp421_read_register(dev, reg_lo, &lo);
  if (ret < 0) return ret;

  *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
  return 0;
}

/* ---- Public API ---- */

int tmp421_init(FAR struct tmp421_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;
  dev->config.frequency = TMP421_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;

  return 0;
}

int tmp421_probe(FAR struct tmp421_device *dev)
{
  uint8_t mid;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mid);
  if (ret < 0) return ret;

  if (mid != TMP421_MANUFACTURER_ID_TI)
    {
      return -ENODEV;
    }

  return 0;
}

int tmp421_read_local_temp(FAR struct tmp421_device *dev,
                           FAR int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  return tmp421_read_temp_pair(dev,
                               TMP421_REG_LOCAL_TEMP_HI,
                               TMP421_REG_LOCAL_TEMP_LO,
                               temp_mcelsius);
}

int tmp421_read_remote_temp(FAR struct tmp421_device *dev,
                            FAR int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  return tmp421_read_temp_pair(dev,
                               TMP421_REG_REMOTE_TEMP_HI,
                               TMP421_REG_REMOTE_TEMP_LO,
                               temp_mcelsius);
}

int tmp421_set_extended_range(FAR struct tmp421_device *dev, bool enable)
{
  uint8_t conf;
  int ret;

  ret = tmp421_read_register(dev, TMP421_REG_CONFIG_1, &conf);
  if (ret < 0) return ret;

  if (enable)
    {
      conf |= TMP421_CONFIG1_RANGE;
    }
  else
    {
      conf &= (uint8_t)~TMP421_CONFIG1_RANGE;
    }

  return tmp421_write_register(dev, TMP421_REG_CONFIG_1, conf);
}
