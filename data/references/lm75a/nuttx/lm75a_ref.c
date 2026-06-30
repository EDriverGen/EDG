/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference driver
 */
#include "lm75a_ref.h"
#include <errno.h>
#include <string.h>

/* ---- Internal helpers ---- */

static int lm75a_read_reg(FAR struct lm75a_device *dev,
                          uint8_t reg,
                          FAR uint8_t *buf,
                          int len)
{
  return i2c_writeread(dev->i2c, &dev->config, &reg, 1, buf, len);
}

static int lm75a_write_reg(FAR struct lm75a_device *dev,
                           uint8_t reg,
                           FAR const uint8_t *buf,
                           int len)
{
  uint8_t frame[3]; /* max: 1 byte reg + 2 bytes data */

  if (len > 2)
    {
      return -EINVAL;
    }

  frame[0] = reg;
  memcpy(&frame[1], buf, len);

  return i2c_write(dev->i2c, &dev->config, frame, len + 1);
}

/*
 * Convert a signed 16-bit raw value (11-bit, 0.125 C step) to milli-Celsius.
 * The temperature register has the following format:
 *   [15:5] = 11-bit signed integer, 0.125 C per LSB
 *   [4:0]  = reserved
 */
static int32_t lm75a_raw_to_mcelsius(int16_t raw)
{
  /* Arithmetic right-shift by 5 preserves sign */
  int32_t value = (int32_t)(raw >> 5);

  return value * LM75A_TEMP_STEP_MC;
}

/*
 * Convert milli-Celsius to the 16-bit register format for Tos/Thyst.
 * These registers use the same encoding as the temperature register:
 * bits [15:7] are a 9-bit signed integer with 0.5 C resolution.
 */
static int16_t lm75a_mcelsius_to_raw(int32_t mc)
{
  int16_t value = (int16_t)(mc / LM75A_TEMP_STEP_MC);

  return (int16_t)(value << 5);
}

/* ---- Public API ---- */

int lm75a_init(FAR struct lm75a_device *dev,
               FAR struct i2c_master_s *i2c,
               uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  if (addr < LM75A_ADDR_MIN || addr > LM75A_ADDR_MAX)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;
  dev->config.frequency = LM75A_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;

  return 0;
}

int lm75a_probe(FAR struct lm75a_device *dev)
{
  uint8_t conf;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1);
  if (ret < 0)
    {
      return ret;
    }

  /* Upper 3 bits of config register should be 0 on LM75A */
  if ((conf & 0xE0) != 0)
    {
      return -ENODEV;
    }

  return 0;
}

int lm75a_read_temperature(FAR struct lm75a_device *dev,
                           FAR int32_t *temp_mcelsius)
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

int lm75a_read_config(FAR struct lm75a_device *dev,
                      FAR uint8_t *config)
{
  if (dev == NULL || config == NULL)
    {
      return -EINVAL;
    }

  return lm75a_read_reg(dev, LM75A_REG_CONF, config, 1);
}

int lm75a_write_config(FAR struct lm75a_device *dev,
                       uint8_t config)
{
  return lm75a_write_reg(dev, LM75A_REG_CONF, &config, 1);
}

int lm75a_set_shutdown(FAR struct lm75a_device *dev, bool enable)
{
  uint8_t conf;
  int ret;

  ret = lm75a_read_config(dev, &conf);
  if (ret < 0)
    {
      return ret;
    }

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

int lm75a_set_tos(FAR struct lm75a_device *dev,
                  int32_t tos_mcelsius)
{
  int16_t raw;
  uint8_t buf[2];

  if (dev == NULL)
    {
      return -EINVAL;
    }

  raw = lm75a_mcelsius_to_raw(tos_mcelsius);
  buf[0] = (uint8_t)(raw >> 8);
  buf[1] = (uint8_t)(raw & 0xFF);

  return lm75a_write_reg(dev, LM75A_REG_TOS, buf, 2);
}

int lm75a_set_thyst(FAR struct lm75a_device *dev,
                    int32_t thyst_mcelsius)
{
  int16_t raw;
  uint8_t buf[2];

  if (dev == NULL)
    {
      return -EINVAL;
    }

  raw = lm75a_mcelsius_to_raw(thyst_mcelsius);
  buf[0] = (uint8_t)(raw >> 8);
  buf[1] = (uint8_t)(raw & 0xFF);

  return lm75a_write_reg(dev, LM75A_REG_THYST, buf, 2);
}
