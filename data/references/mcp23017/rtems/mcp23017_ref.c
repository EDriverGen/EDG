#include "mcp23017_ref.h"
#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int _read_regs(struct mcp23017_device *dev, uint8_t reg,
                      uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd, ret;

    if (!dev || !dev->bus_path || !buf || !len) return -1;

    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    close(fd);
    return (ret >= 0) ? 0 : -1;
}

int mcp23017_init(struct mcp23017_device *dev,
                  const char *bus_path, uint8_t addr)
{
    if (!dev || !bus_path) return -1;
    dev->bus_path = bus_path;
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
