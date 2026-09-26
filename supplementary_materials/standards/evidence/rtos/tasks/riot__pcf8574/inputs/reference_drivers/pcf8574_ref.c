/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for RIOT OS
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 */
#include "pcf8574_ref.h"

int pcf8574_init(struct pcf8574_device *dev, i2c_t bus, uint16_t addr)
{
    if (dev == NULL) return -EINVAL;

    dev->bus  = bus;
    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    int ret;

    if (dev == NULL || val == NULL) return -EINVAL;

    i2c_acquire(dev->bus);
    ret = i2c_read_bytes(dev->bus, dev->addr, val, 1, 0);
    i2c_release(dev->bus);

    return ret;
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    int ret;

    if (dev == NULL) return -EINVAL;

    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, &val, 1, 0);
    i2c_release(dev->bus);

    return ret;
}

/* Per-pin readers: P7=bit7 ... P0=bit0 */
#define _PIN_READ(dev, bit) do { \
    uint8_t v; \
    int ret = pcf8574_read_port(dev, &v); \
    if (ret != 0) return ret; \
    return (int)((v >> bit) & 1); \
} while(0)

int pcf8574_read_p0(struct pcf8574_device *dev) { _PIN_READ(dev, 0); }
int pcf8574_read_p1(struct pcf8574_device *dev) { _PIN_READ(dev, 1); }
int pcf8574_read_p2(struct pcf8574_device *dev) { _PIN_READ(dev, 2); }
int pcf8574_read_p3(struct pcf8574_device *dev) { _PIN_READ(dev, 3); }
int pcf8574_read_p4(struct pcf8574_device *dev) { _PIN_READ(dev, 4); }
int pcf8574_read_p5(struct pcf8574_device *dev) { _PIN_READ(dev, 5); }
int pcf8574_read_p6(struct pcf8574_device *dev) { _PIN_READ(dev, 6); }
int pcf8574_read_p7(struct pcf8574_device *dev) { _PIN_READ(dev, 7); }
