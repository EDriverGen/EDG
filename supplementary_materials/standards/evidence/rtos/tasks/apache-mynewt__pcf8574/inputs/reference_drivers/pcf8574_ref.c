/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for Apache Mynewt
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 */
#include "pcf8574_ref.h"

int pcf8574_init(struct pcf8574_device *dev, uint8_t i2c_num, uint8_t addr)
{
    if (dev == NULL)
    {
        return -1;
    }

    hal_i2c_init(i2c_num, NULL);

    dev->i2c_num = i2c_num;
    dev->addr    = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    struct hal_i2c_master_data xfer;

    if (dev == NULL || val == NULL)
    {
        return -1;
    }

    xfer.address = dev->addr;
    xfer.len     = 1;
    xfer.buffer  = val;

    return hal_i2c_master_read(dev->i2c_num, &xfer,
                               os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    struct hal_i2c_master_data xfer;

    if (dev == NULL)
    {
        return -1;
    }

    xfer.address = dev->addr;
    xfer.len     = 1;
    xfer.buffer  = &val;

    return hal_i2c_master_write(dev->i2c_num, &xfer,
                                os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
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
