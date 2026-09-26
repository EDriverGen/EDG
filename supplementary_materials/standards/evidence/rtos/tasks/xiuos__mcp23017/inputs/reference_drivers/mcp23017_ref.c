#include "mcp23017_ref.h"
#include <stdio.h>

static int _write_then_read(struct mcp23017_device *dev,
                            const uint8_t *wbuf, int wlen,
                            uint8_t *rbuf, int rlen)
{
    if (PrivWrite(dev->fd, wbuf, wlen) < 0)
        return -1;
    if (rlen > 0) {
        if (PrivRead(dev->fd, rbuf, rlen) < 0)
            return -1;
    }
    return 0;
}

static int _read_regs(struct mcp23017_device *dev, uint8_t reg,
                      uint8_t *buf, int len)
{
    if (!dev || !buf) return -1;
    return _write_then_read(dev, &reg, 1, buf, len);
}

int mcp23017_init(struct mcp23017_device *dev,
                  const char *i2c_dev_path, uint16_t addr)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = addr;

    if (!dev || !i2c_dev_path) return -1;

    dev->fd = PrivOpen(i2c_dev_path, O_RDWR);
    if (dev->fd < 0) {
        printf("mcp23017: open %s failed\n", i2c_dev_path);
        return -1;
    }

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0) {
        printf("mcp23017: ioctl set addr 0x%02X failed\n", addr);
        PrivClose(dev->fd);
        dev->fd = -1;
        return -1;
    }

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
