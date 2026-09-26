/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Driver for Zephyr
 */
#include "lm75a_ref.h"
#include <errno.h>
#include <string.h>

/* ---- Internal helpers ---- */

static int lm75a_read_reg(struct lm75a_device *dev,
                          uint8_t reg, uint8_t *buf, uint32_t len)
{
  return i2c_burst_read(dev->bus, dev->addr, reg, buf, len);
}

static int lm75a_write_reg(struct lm75a_device *dev,
                           uint8_t reg, const uint8_t *buf, uint32_t len)
{
  return i2c_burst_write(dev->bus, dev->addr, reg, buf, len);
}

static int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
  int32_t value = (int32_t)(raw >> 5);

  return value * LM75A_TEMP_STEP_MC;
}

static int16_t lm75a_mcelsius_to_raw(int32_t mc)
{
  int16_t value = (int16_t)(mc / LM75A_TEMP_STEP_MC);

  return (int16_t)(value << 5);
}

/* ---- Public API ---- */

int lm75a_init(struct lm75a_device *dev,
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

  if (addr < LM75A_ADDR_MIN || addr > LM75A_ADDR_MAX)
    {
      return -EINVAL;
    }

  dev->bus  = bus;
  dev->addr = addr;

  return 0;
}

int lm75a_probe(struct lm75a_device *dev)
{
  uint8_t conf;
  int ret;

  if (dev == NULL || dev->bus == NULL)
    {
      return -EINVAL;
    }

  ret = lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1);
  if (ret < 0)
    {
      return ret;
    }

  if ((conf & 0xE0) != 0)
    {
      return -ENODEV;
    }

  return 0;
}

int lm75a_read_temperature(struct lm75a_device *dev,
                           int32_t *temp_mcelsius)
{
  uint8_t buf[2];
  int16_t raw;
  int ret;

  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  ret = lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2);
  if (ret < 0)
    {
      return ret;
    }

  raw = (int16_t)((buf[0] << 8) | buf[1]);
  *temp_mcelsius = lm75a_raw_to_mcelsius(raw);

  return 0;
}

int lm75a_read_config(struct lm75a_device *dev, uint8_t *config)
{
  if (dev == NULL || config == NULL)
    {
      return -EINVAL;
    }

  return i2c_reg_read_byte(dev->bus, dev->addr, LM75A_REG_CONF, config);
}

int lm75a_write_config(struct lm75a_device *dev, uint8_t config)
{
  return i2c_reg_write_byte(dev->bus, dev->addr, LM75A_REG_CONF, config);
}

int lm75a_set_shutdown(struct lm75a_device *dev, bool enable)
{
  uint8_t conf;
  int ret;

  ret = lm75a_read_config(dev, &conf);
  if (ret < 0) return ret;

  if (enable)
    {
      conf |= LM75A_CONF_SHUTDOWN;
    }
  else
    {
      conf &= (uint8_t)~LM75A_CONF_SHUTDOWN;
    }

  return lm75a_write_config(dev, conf);
}

int lm75a_set_tos(struct lm75a_device *dev, int32_t tos_mcelsius)
{
  int16_t raw;
  uint8_t buf[2];

  if (dev == NULL) return -EINVAL;

  raw = lm75a_mcelsius_to_raw(tos_mcelsius);
  buf[0] = (uint8_t)(raw >> 8);
  buf[1] = (uint8_t)(raw & 0xFF);

  return lm75a_write_reg(dev, LM75A_REG_TOS, buf, 2);
}

int lm75a_set_thyst(struct lm75a_device *dev, int32_t thyst_mcelsius)
{
  int16_t raw;
  uint8_t buf[2];

  if (dev == NULL) return -EINVAL;

  raw = lm75a_mcelsius_to_raw(thyst_mcelsius);
  buf[0] = (uint8_t)(raw >> 8);
  buf[1] = (uint8_t)(raw & 0xFF);

  return lm75a_write_reg(dev, LM75A_REG_THYST, buf, 2);
}
