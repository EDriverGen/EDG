/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for XiUOS
 */
#include "tmp421_ref.h"

static int tmp421_read_register(struct tmp421_device *dev,
                                uint8_t reg, uint8_t *value)
{
  if (PrivWrite(dev->fd, &reg, 1) < 0) return -1;
  if (PrivRead(dev->fd, value, 1) < 0) return -1;
  return 0;
}

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

  if (tmp421_read_register(dev, reg_hi, &hi) < 0) return -1;
  if (tmp421_read_register(dev, reg_lo, &lo) < 0) return -1;

  *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
  return 0;
}

int tmp421_init(struct tmp421_device *dev,
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

void tmp421_deinit(struct tmp421_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int tmp421_probe(struct tmp421_device *dev)
{
  uint8_t mid;

  if (dev == NULL || dev->fd < 0) return -1;

  if (tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mid) < 0)
    return -1;

  if (mid != TMP421_MANUFACTURER_ID_TI) return -1;
  return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev,
                           int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -1;

  return tmp421_read_temp_pair(dev,
                               TMP421_REG_LOCAL_TEMP_HI,
                               TMP421_REG_LOCAL_TEMP_LO,
                               temp_mcelsius);
}

int tmp421_read_remote_temp(struct tmp421_device *dev,
                            int32_t *temp_mcelsius)
{
  if (dev == NULL || temp_mcelsius == NULL) return -1;

  return tmp421_read_temp_pair(dev,
                               TMP421_REG_REMOTE_TEMP_HI,
                               TMP421_REG_REMOTE_TEMP_LO,
                               temp_mcelsius);
}
