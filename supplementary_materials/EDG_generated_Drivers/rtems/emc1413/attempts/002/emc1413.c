#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rtems.h"
#define EMC1413_REG_MANUFACTURER_ID 0xFE
#define EMC1413_REG_PRODUCT_ID 0xFD
#define EMC1413_REG_CONFIG 0x03
#define EMC1413_REG_CONV_RATE 0x04
#define EMC1413_REG_INT_HIGH 0x00
#define EMC1413_REG_INT_LOW 0x29
#define EMC1413_REG_EXT1_HIGH 0x01
#define EMC1413_REG_EXT1_LOW 0x10
#define EMC1413_REG_EXT2_HIGH 0x23
#define EMC1413_REG_EXT2_LOW 0x24

static int i2c_write_then_read(int fd, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct i2c_msg msgs[2];
    int ret;
    struct i2c_rdwr_ioctl_data rdwr;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = wlen;
    msgs[0].buf = wbuf;

    if (rlen > 0) {
        msgs[1].addr = addr;
        msgs[1].flags = I2C_M_RD;
        msgs[1].len = rlen;
        msgs[1].buf = rbuf;
        rdwr.msgs = msgs;
        rdwr.nmsgs = 2;
    } else {
        rdwr.msgs = msgs;
        rdwr.nmsgs = 1;
    }

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -errno;
    }
    return 0;
}

static int i2c_write(int fd, uint8_t addr, uint8_t *buf, uint16_t len)
{
    return i2c_write_then_read(fd, addr, buf, len, NULL, 0);
}

static int i2c_read(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return i2c_write_then_read(fd, addr, &reg, 1, buf, len);
}

static int read_temperature_channel(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *result)
{
    uint8_t high_byte, low_byte;
    int ret;

    ret = i2c_read(dev->fd, EMC1413_I2C_ADDR, high_reg, &high_byte, 1);
    if (ret < 0) return ret;

    ret = i2c_read(dev->fd, EMC1413_I2C_ADDR, low_reg, &low_byte, 1);
    if (ret < 0) return ret;

    *result = ((int32_t)high_byte * 1000) + ((((low_byte >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    int fd;
    uint8_t buf[2];
    uint8_t reg;
    uint8_t id;
    int ret;

    fd = open("/dev/i2c-0", O_RDWR);
    if (fd < 0) {
        return -errno;
    }
    dev->fd = fd;

    usleep(15000);

    reg = EMC1413_REG_MANUFACTURER_ID;
    ret = i2c_read(dev->fd, EMC1413_I2C_ADDR, reg, &id, 1);
    if (ret < 0 || id != 0x5D) {
        close(dev->fd);
        return -ENODEV;
    }

    reg = EMC1413_REG_PRODUCT_ID;
    ret = i2c_read(dev->fd, EMC1413_I2C_ADDR, reg, &id, 1);
    if (ret < 0 || id != 0x21) {
        close(dev->fd);
        return -ENODEV;
    }

    buf[0] = EMC1413_REG_CONFIG;
    buf[1] = 0x00;
    ret = i2c_write(dev->fd, EMC1413_I2C_ADDR, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    buf[0] = EMC1413_REG_CONV_RATE;
    buf[1] = 0x06;
    ret = i2c_write(dev->fd, EMC1413_I2C_ADDR, buf, 2);
    if (ret < 0) {
        close(dev->fd);
        return ret;
    }

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    return read_temperature_channel(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    return read_temperature_channel(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    return read_temperature_channel(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
