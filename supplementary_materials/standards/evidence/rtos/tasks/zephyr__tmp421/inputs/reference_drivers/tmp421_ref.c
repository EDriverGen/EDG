/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for Zephyr
 */
#include "tmp421_ref.h"
#include <errno.h>

static int32_t tmp421_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
  int16_t raw = (int16_t)((hi << 8) | lo);
  int32_t frac = (int32_t)(raw >> 4);

  return (frac * 1000) / 16;
}

static int tmp421_read_temp_pair(struct tmp421_device *dev,
                                 uint8_t reg_hi, uint8_t reg_lo,
                                 int32_t *temp_mcelsius)
{
  uint8_t hi, lo;
  int ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr, reg_hi, &hi);
  if (ret < 0) return ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr, reg_lo, &lo);
  if (ret < 0) return ret;

  *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
  return 0;
}

int tmp421_init(struct tmp421_device *dev,
                const struct device *bus,
                uint16_t addr)
{
  if (dev == NULL || bus == NULL)
    {
      return -EINVAL;
    }

  if (!device_is_ready(bus))
    {
      return -ENODEV;
    }

  dev->bus  = bus;
  dev->addr = addr;

  return 0;
}

int tmp421_probe(struct tmp421_device *dev)
{
  uint8_t mid;
  int ret;

  if (dev == NULL || dev->bus == NULL)
    {
      return -EINVAL;
    }

  ret = i2c_reg_read_byte(dev->bus, dev->addr,
                          TMP421_REG_MANUFACTURER_ID, &mid);
  if (ret < 0) return ret;

  if (mid != TMP421_MANUFACTURER_ID_TI)
    {
      return -ENODEV;
    }

  return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev,
                           int32_t *temp_mcelsius)
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

int tmp421_read_remote_temp(struct tmp421_device *dev,
                            int32_t *temp_mcelsius)
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

int tmp421_set_extended_range(struct tmp421_device *dev, bool enable)
{
  uint8_t conf;
  int ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr,
                          TMP421_REG_CONFIG_1, &conf);
  if (ret < 0) return ret;

  if (enable)
    {
      conf |= TMP421_CONFIG1_RANGE;
    }
  else
    {
      conf &= (uint8_t)~TMP421_CONFIG1_RANGE;
    }

  return i2c_reg_write_byte(dev->bus, dev->addr,
                            TMP421_REG_CONFIG_1, conf);
}
