/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for XiUOS
 */
#include "tmp105_ref.h"
#include <string.h>

static int tmp105_read_reg(struct tmp105_device *dev,
                           uint8_t reg, uint8_t *buf, int len)
{
  if (PrivWrite(dev->fd, &reg, 1) < 0) return -1;
  if (PrivRead(dev->fd, buf, len) < 0) return -1;
  return 0;
}

static int tmp105_write_reg(struct tmp105_device *dev,
                            uint8_t reg, const uint8_t *buf, int len)
{
  uint8_t frame[3];

  if (len > 2) return -1;
  frame[0] = reg;
  memcpy(&frame[1], buf, len);
  if (PrivWrite(dev->fd, frame, len + 1) < 0) return -1;
  return 0;
}

static int32_t tmp105_raw_to_mcelsius(int16_t raw, uint8_t resolution)
{
  int shift_bits;
  int32_t step_mc;

  switch (resolution)
    {
    case TMP105_RES_9BIT:  shift_bits = 7; step_mc = 500; break;
    case TMP105_RES_10BIT: shift_bits = 6; step_mc = 250; break;
    case TMP105_RES_11BIT: shift_bits = 5; step_mc = 125; break;
    default:               shift_bits = 4; step_mc = 625; break;
    }

  if (resolution >= TMP105_RES_12BIT)
    return (int32_t)(raw >> shift_bits) * step_mc / 10;
  return (int32_t)(raw >> shift_bits) * step_mc;
}

int tmp105_init(struct tmp105_device *dev,
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
  dev->resolution = TMP105_RES_12BIT;
  return 0;
}

void tmp105_deinit(struct tmp105_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int tmp105_probe(struct tmp105_device *dev)
{
  uint8_t conf;

  if (dev == NULL || dev->fd < 0) return -1;
  return tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1);
}

int tmp105_set_resolution(struct tmp105_device *dev, uint8_t res)
{
  uint8_t conf;

  if (dev == NULL || res > TMP105_RES_12BIT) return -1;

  if (tmp105_read_reg(dev, TMP105_REG_CONF, &conf, 1) < 0) return -1;

  conf &= ~(TMP105_CONF_R0 | TMP105_CONF_R1);
  conf |= (res << 5);

  if (tmp105_write_reg(dev, TMP105_REG_CONF, &conf, 1) < 0) return -1;

  dev->resolution = res;
  return 0;
}

int tmp105_read_temperature(struct tmp105_device *dev,
                            int32_t *temp_mcelsius)
{
  uint8_t buf[2];
  int16_t raw;

  if (dev == NULL || temp_mcelsius == NULL) return -1;

  if (tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2) < 0) return -1;

  raw = (int16_t)((buf[0] << 8) | buf[1]);
  *temp_mcelsius = tmp105_raw_to_mcelsius(raw, dev->resolution);
  return 0;
}
