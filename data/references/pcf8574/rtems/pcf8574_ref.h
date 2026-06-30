/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for RTEMS
 */
#ifndef __PCF8574_REF_H
#define __PCF8574_REF_H

#include <stdint.h>
#include <rtems.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* PCF8574 7-bit I2C address: 0 1 0 0 A2 A1 A0.
 * Default A2=A1=0, A0=1 -> 0x21 to avoid conflict with MCP23017 at 0x20. */
#define PCF8574_I2C_ADDR  0x21

struct pcf8574_device
{
    const char *bus_path;
    uint8_t addr;
};

int pcf8574_init(struct pcf8574_device *dev,
                 const char *bus_path,
                 uint8_t addr);
int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val);
int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val);

/* Per-pin readers: P7=bit7 ... P0=bit0 */
int pcf8574_read_p0(struct pcf8574_device *dev);
int pcf8574_read_p1(struct pcf8574_device *dev);
int pcf8574_read_p2(struct pcf8574_device *dev);
int pcf8574_read_p3(struct pcf8574_device *dev);
int pcf8574_read_p4(struct pcf8574_device *dev);
int pcf8574_read_p5(struct pcf8574_device *dev);
int pcf8574_read_p6(struct pcf8574_device *dev);
int pcf8574_read_p7(struct pcf8574_device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PCF8574_REF_H */
