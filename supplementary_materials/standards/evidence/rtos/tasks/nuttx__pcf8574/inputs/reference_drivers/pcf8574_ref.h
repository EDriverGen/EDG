/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-05-19     Lin          NuttX reference driver
 */
#ifndef __PCF8574_REF_H
#define __PCF8574_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* PCF8574 7-bit I2C address: 0 1 0 0 A2 A1 A0.
 * Default A2=A1=0, A0=1 -> 0x21 to avoid conflict with MCP23017 at 0x20. */
#define PCF8574_I2C_ADDR  0x21
#define PCF8574_I2C_FREQ  100000

struct pcf8574_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int pcf8574_init(FAR struct pcf8574_device *dev,
                 FAR struct i2c_master_s *i2c,
                 uint8_t addr);
int pcf8574_read_port(FAR struct pcf8574_device *dev, uint8_t *val);
int pcf8574_write_port(FAR struct pcf8574_device *dev, uint8_t val);

/* Per-pin readers: P7=bit7 ... P0=bit0 */
int pcf8574_read_p0(FAR struct pcf8574_device *dev);
int pcf8574_read_p1(FAR struct pcf8574_device *dev);
int pcf8574_read_p2(FAR struct pcf8574_device *dev);
int pcf8574_read_p3(FAR struct pcf8574_device *dev);
int pcf8574_read_p4(FAR struct pcf8574_device *dev);
int pcf8574_read_p5(FAR struct pcf8574_device *dev);
int pcf8574_read_p6(FAR struct pcf8574_device *dev);
int pcf8574_read_p7(FAR struct pcf8574_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PCF8574_REF_H */
