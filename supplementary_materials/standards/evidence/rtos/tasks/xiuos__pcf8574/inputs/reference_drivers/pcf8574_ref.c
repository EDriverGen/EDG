/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for XiUOS
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 */
#include "pcf8574_ref.h"

int pcf8574_init(struct pcf8574_device *dev,
                 const char *i2c_dev_path,
                 uint16_t addr)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = addr;

    if (dev == NULL || i2c_dev_path == NULL) return -1;

    dev->fd = PrivOpen(i2c_dev_path, O_RDWR);
    if (dev->fd < 0)
    {
        return -1;
    }

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0)
    {
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }

    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

void pcf8574_deinit(struct pcf8574_device *dev)
{
    if (dev != NULL && dev->fd >= 0)
    {
        PrivClose(dev->fd);
        dev->fd = -1;
    }
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    if (dev == NULL || val == NULL) return -1;
    if (PrivRead(dev->fd, val, 1) < 0) return -1;
    return 0;
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    if (dev == NULL) return -1;
    if (PrivWrite(dev->fd, &val, 1) < 0) return -1;
    return 0;
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
