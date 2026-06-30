#include "at24c256_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int at24c256_init(struct at24c256_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0) return -1;
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev)
{
    uint8_t data = 0;
    return at24c256_read(dev, 0, &data, 1);
}

int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len)
{
    uint8_t buf[AT24C256_PAGE_SIZE + 2];
    uint16_t offset = 0;
    if (dev == 0 || dev->bus_path == 0 || data == 0) return -1;
    while (offset < len) {
        struct i2c_msg msg;
        struct i2c_rdwr_ioctl_data rdwr;
        int fd;
        int ret;
        uint16_t page_rem = (uint16_t)(AT24C256_PAGE_SIZE - ((mem_addr + offset) % AT24C256_PAGE_SIZE));
        uint16_t chunk = (uint16_t)((len - offset) < page_rem ? (len - offset) : page_rem);
        buf[0] = (uint8_t)((mem_addr + offset) >> 8);
        buf[1] = (uint8_t)((mem_addr + offset) & 0xFFU);
        for (uint16_t i = 0; i < chunk; i++) buf[i + 2] = data[offset + i];
        msg.addr = dev->addr;
        msg.flags = 0;
        msg.len = (uint16_t)(chunk + 2);
        msg.buf = buf;
        rdwr.msgs = &msg;
        rdwr.nmsgs = 1;
        fd = open(dev->bus_path, O_RDWR);
        if (fd < 0) return -1;
        ret = ioctl(fd, I2C_RDWR, &rdwr);
        (void)close(fd);
        if (ret != 0) return -1;
        (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(5));
        offset = (uint16_t)(offset + chunk);
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    uint8_t addr_buf[2];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || data == 0) return -1;
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFFU);
    msgs[0].addr = dev->addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = addr_buf;
    msgs[1].addr = dev->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = data;
    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) return -1;
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}
