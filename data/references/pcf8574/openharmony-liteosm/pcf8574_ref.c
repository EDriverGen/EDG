/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for OpenHarmony LiteOS-M
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 */
#include "pcf8574_ref.h"

static int _i2c_write(DevHandle bus, uint16_t addr,
                      const uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;

    msg.addr  = addr;
    msg.buf   = (uint8_t *)data;
    msg.len   = len;
    msg.flags = 0;

    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int _i2c_read(DevHandle bus, uint16_t addr,
                     uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;

    msg.addr  = addr;
    msg.buf   = data;
    msg.len   = len;
    msg.flags = I2C_FLAG_READ;

    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

int pcf8574_init(struct pcf8574_device *dev, DevHandle bus, uint16_t addr)
{
    if (dev == NULL) return -1;

    dev->bus  = bus;
    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    if (dev == NULL || val == NULL) return -1;
    return _i2c_read(dev->bus, dev->addr, val, 1);
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    if (dev == NULL) return -1;
    return _i2c_write(dev->bus, dev->addr, &val, 1);
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
