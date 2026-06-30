#include "pcf8574_ref.h"

rt_err_t pcf8574_init(struct pcf8574_device *dev,
                      struct rt_i2c_bus_device *bus, uint16_t addr)
{
    if (!dev || !bus) return -RT_EINVAL;
    dev->bus  = bus;
    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state. */
    return pcf8574_write_port(dev, 0xFF);
}

rt_err_t pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    struct rt_i2c_msg msg;
    if (!dev || !val) return -RT_EINVAL;

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_RD;
    msg.buf   = val;
    msg.len   = 1;
    return rt_i2c_transfer(dev->bus, &msg, 1) == 1 ? RT_EOK : -RT_ERROR;
}

rt_err_t pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    struct rt_i2c_msg msg;
    if (!dev) return -RT_EINVAL;

    msg.addr  = dev->addr;
    msg.flags = RT_I2C_WR;
    msg.buf   = &val;
    msg.len   = 1;
    return rt_i2c_transfer(dev->bus, &msg, 1) == 1 ? RT_EOK : -RT_ERROR;
}

/* Per-pin readers: P7=bit7 ... P0=bit0 */
#define _PIN_READ(dev, bit) do { \
    uint8_t v; \
    if (pcf8574_read_port(dev, &v) != RT_EOK) return -1; \
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
