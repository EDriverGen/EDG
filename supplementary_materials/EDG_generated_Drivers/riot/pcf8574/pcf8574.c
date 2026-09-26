#include "pcf8574.h"
#include <stdint.h>
#include <stddef.h>
#include "periph/i2c.h"

#include "riot.h"
int pcf8574_init(pcf8574_t *dev, i2c_t bus, uint8_t addr)
{
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int pcf8574_read_port(pcf8574_t *dev, uint8_t *port_byte)
{
    int ret = i2c_read_regs(dev->bus, dev->addr, 0x00, port_byte, 1, 0);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
