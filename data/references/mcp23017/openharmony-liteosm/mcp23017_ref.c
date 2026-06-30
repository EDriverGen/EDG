#include "mcp23017_ref.h"

static int _i2c_write_read(DevHandle bus, uint16_t addr,
                           const uint8_t *wdata, uint16_t wlen,
                           uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];
    if (!bus || !wdata || !rdata) return -1;
    msg[0].addr = addr;
    msg[0].buf = (uint8_t *)wdata;
    msg[0].len = wlen;
    msg[0].flags = 0;
    msg[1].addr = addr;
    msg[1].buf = rdata;
    msg[1].len = rlen;
    msg[1].flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, msg, 2) == 2) ? 0 : -1;
}

static int _read_regs(struct mcp23017_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    if (!dev || !dev->bus || !buf) return -1;
    return _i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

int mcp23017_init(struct mcp23017_device *dev, DevHandle bus, uint16_t addr)
{
    if (!dev) return -1;
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int mcp23017_read_ports(struct mcp23017_device *dev,
                          uint8_t *porta, uint8_t *portb)
{
    uint8_t buf[2];
    if (_read_regs(dev, MCP23017_REG_GPIOA, buf, 2) != 0)
        return -1;
    *porta = buf[0];
    *portb = buf[1];
    return 0;
}

#define _PIN_READ_A(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != 0) return -1; \
    return (pa >> bit) & 1; \
} while(0)

#define _PIN_READ_B(dev, bit) do { \
    uint8_t pa, pb; \
    if (mcp23017_read_ports(dev, &pa, &pb) != 0) return -1; \
    return (pb >> bit) & 1; \
} while(0)

int mcp23017_read_gpa0(struct mcp23017_device *dev) { _PIN_READ_A(dev, 0); }
int mcp23017_read_gpa1(struct mcp23017_device *dev) { _PIN_READ_A(dev, 1); }
int mcp23017_read_gpa2(struct mcp23017_device *dev) { _PIN_READ_A(dev, 2); }
int mcp23017_read_gpa3(struct mcp23017_device *dev) { _PIN_READ_A(dev, 3); }
int mcp23017_read_gpa4(struct mcp23017_device *dev) { _PIN_READ_A(dev, 4); }
int mcp23017_read_gpa5(struct mcp23017_device *dev) { _PIN_READ_A(dev, 5); }
int mcp23017_read_gpa6(struct mcp23017_device *dev) { _PIN_READ_A(dev, 6); }
int mcp23017_read_gpa7(struct mcp23017_device *dev) { _PIN_READ_A(dev, 7); }
int mcp23017_read_gpb0(struct mcp23017_device *dev) { _PIN_READ_B(dev, 0); }
int mcp23017_read_gpb1(struct mcp23017_device *dev) { _PIN_READ_B(dev, 1); }
int mcp23017_read_gpb2(struct mcp23017_device *dev) { _PIN_READ_B(dev, 2); }
int mcp23017_read_gpb3(struct mcp23017_device *dev) { _PIN_READ_B(dev, 3); }
int mcp23017_read_gpb4(struct mcp23017_device *dev) { _PIN_READ_B(dev, 4); }
int mcp23017_read_gpb5(struct mcp23017_device *dev) { _PIN_READ_B(dev, 5); }
int mcp23017_read_gpb6(struct mcp23017_device *dev) { _PIN_READ_B(dev, 6); }
int mcp23017_read_gpb7(struct mcp23017_device *dev) { _PIN_READ_B(dev, 7); }
