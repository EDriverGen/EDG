/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for NuttX
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-19     Lin          NuttX reference driver
 */
#include "pcf8574_ref.h"
#include <errno.h>

int pcf8574_init(FAR struct pcf8574_device *dev,
                 FAR struct i2c_master_s *i2c,
                 uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  dev->i2c              = i2c;
  dev->config.frequency = PCF8574_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;

  /* Set all pins HIGH (input mode for quasi-bidirectional ports).
   * POR default is already HIGH; this write ensures known state. */
  return pcf8574_write_port(dev, 0xFF);
}

int pcf8574_read_port(FAR struct pcf8574_device *dev, uint8_t *val)
{
  int ret;

  if (dev == NULL || val == NULL)
    {
      return -EINVAL;
    }

  ret = i2c_read(dev->i2c, &dev->config, val, 1);
  return (ret >= 0) ? 0 : ret;
}

int pcf8574_write_port(FAR struct pcf8574_device *dev, uint8_t val)
{
  uint8_t buf = val;
  int ret;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  ret = i2c_write(dev->i2c, &dev->config, &buf, 1);
  return (ret >= 0) ? 0 : ret;
}

/* Per-pin readers: P7=bit7 ... P0=bit0 */
#define _PIN_READ(dev, bit) do { \
    uint8_t v; \
    int ret = pcf8574_read_port(dev, &v); \
    if (ret != 0) return ret; \
    return (int)((v >> bit) & 1); \
} while(0)

int pcf8574_read_p0(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 0); }
int pcf8574_read_p1(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 1); }
int pcf8574_read_p2(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 2); }
int pcf8574_read_p3(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 3); }
int pcf8574_read_p4(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 4); }
int pcf8574_read_p5(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 5); }
int pcf8574_read_p6(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 6); }
int pcf8574_read_p7(FAR struct pcf8574_device *dev) { _PIN_READ(dev, 7); }
