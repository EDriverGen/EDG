#include "pcf8574_ref.h"

int pcf8574_init(struct pcf8574_device *dev, void *bus_context,
                 const struct pcf8574_i2c_ops *ops, uint16_t addr)
{
    uint8_t val = 0xFF;

    if (!dev) return -1;
    dev->bus_context = bus_context;
    dev->ops  = ops;
    dev->addr = addr;

    /* Set all pins HIGH (input mode for quasi-bidirectional ports).
     * POR default is already HIGH; this write ensures known state.
     * PCF8574 has NO registers — pure I2C write of 1 byte to port. */
    return pcf8574_write_port(dev, val);
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *val)
{
    if (!dev || !dev->bus_context || !dev->ops ||
        !dev->ops->write_read || !val)
        return -1;

    /* PCF8574 has NO registers — pure I2C read, no write-then-read pattern.
     * wdata=NULL, wlen=0 — just receive 1 byte from the device. */
    return dev->ops->write_read(dev->bus_context, dev->addr,
                                NULL, 0, val, 1);
}

int pcf8574_write_port(struct pcf8574_device *dev, uint8_t val)
{
    if (!dev || !dev->bus_context || !dev->ops ||
        !dev->ops->write_read)
        return -1;

    /* PCF8574 has NO registers — pure I2C write, no register address byte.
     * wdata=&val, wlen=1, rdata=NULL, rlen=0 — just transmit 1 byte. */
    return dev->ops->write_read(dev->bus_context, dev->addr,
                                &val, 1, NULL, 0);
}

/* Per-pin readers: P7=bit7 ... P0=bit0 */
#define _PIN_READ(dev, bit) do { \
    uint8_t v; \
    if (pcf8574_read_port(dev, &v) != 0) return -1; \
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
