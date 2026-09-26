/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * PCF8574 8-bit quasi-bidirectional I/O expander driver for RTEMS
 *
 * Datasheet: PCF8574 — no registers. Pure I2C read/write for port access.
 */
#include "pcf8574_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int _i2c_transfer(const char *bus_path, struct i2c_msg *msgs,
                         uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;

    if (bus_path == NULL || msgs == NULL || nmsgs == 0) return -1;

    fd = open(bus_path, O_RDWR);
    if (fd < 0) return -1;

    rdwr.msgs  = msgs;
    rdwr.nmsgs = nmsgs;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);

    return (ret >= 0) ? 0 : -1;
}

int pcf8574_init(struct pcf8574_device *dev,
                 const char *bus_path,
                 uint8_t addr)
{
    if (dev == NULL || bus_path == NULL) return -1;

    dev->bus_path = bus_path;
    dev->addr     = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    struct i2c_msg msg;

    if (dev == NULL || val == NULL) return -1;

    msg.addr  = dev->addr;
    msg.flags = I2C_M_RD;
    msg.len   = 1;
    msg.buf   = val;

    return _i2c_transfer(dev->bus_path, &msg, 1);
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    struct i2c_msg msg;

    if (dev == NULL) return -1;

    msg.addr  = dev->addr;
    msg.flags = 0;
    msg.len   = 1;
    msg.buf   = &val;

    return _i2c_transfer(dev->bus_path, &msg, 1);
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
