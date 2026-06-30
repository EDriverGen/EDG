/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Driver for Zephyr
 */
#include "emc1413_ref.h"
#include <errno.h>

static int32_t emc1413_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
  int16_t raw = (int16_t)((hi << 8) | lo);

  return (int32_t)(raw >> 5) * 125;
}

static int emc1413_read_temp(struct emc1413_device *dev,
                             uint8_t reg_hi, uint8_t reg_lo,
                             int32_t *temp_mcelsius)
{
  uint8_t hi, lo;
  int ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr, reg_hi, &hi);
  if (ret < 0) return ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr, reg_lo, &lo);
  if (ret < 0) return ret;

  *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
  return 0;
}

int emc1413_init(struct emc1413_device *dev,
                 const struct device *bus,
                 uint16_t addr)
{
  if (dev == NULL || bus == NULL) return -EINVAL;
  if (!device_is_ready(bus)) return -ENODEV;

  dev->bus  = bus;
  dev->addr = addr;
  return 0;
}

int emc1413_probe(struct emc1413_device *dev)
{
  uint8_t mid, pid;
  int ret;

  if (dev == NULL || dev->bus == NULL) return -EINVAL;

  ret = i2c_reg_read_byte(dev->bus, dev->addr,
                          EMC1413_REG_MANUFACTURER_ID, &mid);
  if (ret < 0) return ret;

  ret = i2c_reg_read_byte(dev->bus, dev->addr,
                          EMC1413_REG_PRODUCT_ID, &pid);
  if (ret < 0) return ret;

  if (mid != EMC1413_MANUFACTURER_SMSC || pid != EMC1413_PRODUCT_ID)
    {
      return -ENODEV;
    }

  return 0;
}

int emc1413_read_internal_temp(struct emc1413_device *dev,
                               int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;

  return emc1413_read_temp(dev,
                           EMC1413_REG_INTERNAL_TEMP_HI,
                           EMC1413_REG_INTERNAL_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext1_temp(struct emc1413_device *dev,
                           int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT1_TEMP_HI,
                           EMC1413_REG_EXT1_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext2_temp(struct emc1413_device *dev,
                           int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -EINVAL;

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT2_TEMP_HI,
                           EMC1413_REG_EXT2_TEMP_LO,
                           temp_mcelsius);
}
