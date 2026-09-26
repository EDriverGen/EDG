/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for Zephyr
 */
#include "tmp105_ref.h"
#include <errno.h>

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

int tmp105_init(struct tmp105_device *dev,
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
  dev->resolution = TMP105_RES_12BIT;

  return 0;
}

int tmp105_probe(struct tmp105_device *dev)
{
  uint8_t conf;

  if (dev == NULL || dev->bus == NULL)
    {
      return -EINVAL;
    }

  return i2c_reg_read_byte(dev->bus, dev->addr, TMP105_REG_CONF, &conf);
}

int tmp105_set_resolution(struct tmp105_device *dev, uint8_t res)
{
  uint8_t conf;
  int ret;

  if (dev == NULL || res > TMP105_RES_12BIT)
    {
      return -EINVAL;
    }

  ret = i2c_reg_read_byte(dev->bus, dev->addr, TMP105_REG_CONF, &conf);
  if (ret < 0) return ret;

  conf &= ~(TMP105_CONF_R0 | TMP105_CONF_R1);
  conf |= (res << 5);

  ret = i2c_reg_write_byte(dev->bus, dev->addr, TMP105_REG_CONF, conf);
  if (ret < 0) return ret;

  dev->resolution = res;
  return 0;
}

int tmp105_read_temperature(struct tmp105_device *dev,
                            int32_t *temp_mcelsius)
{
  uint8_t buf[2];
  int16_t raw;
  int ret;

  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  ret = i2c_burst_read(dev->bus, dev->addr, TMP105_REG_TEMP, buf, 2);
  if (ret < 0) return ret;

  raw = (int16_t)((buf[0] << 8) | buf[1]);
  *temp_mcelsius = tmp105_raw_to_mcelsius(raw, dev->resolution);

  return 0;
}

int tmp105_read_config(struct tmp105_device *dev, uint8_t *config)
{
  if (dev == NULL || config == NULL)
    {
      return -EINVAL;
    }

  return i2c_reg_read_byte(dev->bus, dev->addr, TMP105_REG_CONF, config);
}

int tmp105_write_config(struct tmp105_device *dev, uint8_t config)
{
  return i2c_reg_write_byte(dev->bus, dev->addr, TMP105_REG_CONF, config);
}
