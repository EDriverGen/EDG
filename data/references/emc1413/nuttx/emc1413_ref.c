/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Driver for NuttX
 */
#include "emc1413_ref.h"
#include <errno.h>

/* ---- Internal helpers ---- */

static int emc1413_read_register(FAR struct emc1413_device *dev,
                                 uint8_t reg, FAR uint8_t *value)
{
  return i2c_writeread(dev->i2c, &dev->config, &reg, 1, value, 1);
}

/*
 * EMC1413 temperature format: hi = signed integer, lo[7:5] = 0.125 C steps.
 * Total resolution = 0.125 C.
 */
static int32_t emc1413_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
  int16_t raw = (int16_t)((hi << 8) | lo);

  /* Only upper 11 bits are valid: raw >> 5 gives value in 0.125 C units */
  return (int32_t)(raw >> 5) * 125;
}

static int emc1413_read_temp(FAR struct emc1413_device *dev,
                             uint8_t reg_hi, uint8_t reg_lo,
                             FAR int32_t *temp_mcelsius)
{
  uint8_t hi, lo;
  int ret;

  ret = emc1413_read_register(dev, reg_hi, &hi);
  if (ret < 0) return ret;

  ret = emc1413_read_register(dev, reg_lo, &lo);
  if (ret < 0) return ret;

  *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
  return 0;
}

/* ---- Public API ---- */

int emc1413_init(FAR struct emc1413_device *dev,
                 FAR struct i2c_master_s *i2c,
                 uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;
  dev->config.frequency = EMC1413_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;

  return 0;
}

int emc1413_probe(FAR struct emc1413_device *dev)
{
  uint8_t mid, pid;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &mid);
  if (ret < 0) return ret;

  ret = emc1413_read_register(dev, EMC1413_REG_PRODUCT_ID, &pid);
  if (ret < 0) return ret;

  if (mid != EMC1413_MANUFACTURER_SMSC || pid != EMC1413_PRODUCT_ID)
    {
      return -ENODEV;
    }

  return 0;
}

int emc1413_read_internal_temp(FAR struct emc1413_device *dev,
                               FAR int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  return emc1413_read_temp(dev,
                           EMC1413_REG_INTERNAL_TEMP_HI,
                           EMC1413_REG_INTERNAL_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext1_temp(FAR struct emc1413_device *dev,
                           FAR int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT1_TEMP_HI,
                           EMC1413_REG_EXT1_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext2_temp(FAR struct emc1413_device *dev,
                           FAR int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT2_TEMP_HI,
                           EMC1413_REG_EXT2_TEMP_LO,
                           temp_mcelsius);
}
