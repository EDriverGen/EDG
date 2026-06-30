/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Driver for XiUOS
 */
#include "emc1413_ref.h"

static int emc1413_read_register(struct emc1413_device *dev,
                                 uint8_t reg, uint8_t *value)
{
  if (PrivWrite(dev->fd, &reg, 1) < 0) return -1;
  if (PrivRead(dev->fd, value, 1) < 0) return -1;
  return 0;
}

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

  if (emc1413_read_register(dev, reg_hi, &hi) < 0) return -1;
  if (emc1413_read_register(dev, reg_lo, &lo) < 0) return -1;

  *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
  return 0;
}

int emc1413_init(struct emc1413_device *dev,
                 const char *i2c_dev_path,
                 uint16_t addr)
{
  struct PrivIoctlCfg ioctl_cfg;
  uint16_t i2c_addr = addr;

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
  return 0;
}

void emc1413_deinit(struct emc1413_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int emc1413_probe(struct emc1413_device *dev)
{
  uint8_t mid, pid;

  if (dev == NULL || dev->fd < 0) return -1;

  if (emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &mid) < 0)
    return -1;
  if (emc1413_read_register(dev, EMC1413_REG_PRODUCT_ID, &pid) < 0)
    return -1;

  if (mid != EMC1413_MANUFACTURER_SMSC || pid != EMC1413_PRODUCT_ID)
    return -1;

  return 0;
}

int emc1413_read_internal_temp(struct emc1413_device *dev,
                               int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -1;

  return emc1413_read_temp(dev,
                           EMC1413_REG_INTERNAL_TEMP_HI,
                           EMC1413_REG_INTERNAL_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext1_temp(struct emc1413_device *dev,
                           int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -1;

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT1_TEMP_HI,
                           EMC1413_REG_EXT1_TEMP_LO,
                           temp_mcelsius);
}

int emc1413_read_ext2_temp(struct emc1413_device *dev,
                           int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -1;

  return emc1413_read_temp(dev,
                           EMC1413_REG_EXT2_TEMP_HI,
                           EMC1413_REG_EXT2_TEMP_LO,
                           temp_mcelsius);
}
