#include "emc1413.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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

#define EMC1413_EXPECTED_MANUFACTURER_ID 0x5D
#define EMC1413_EXPECTED_PRODUCT_ID 0x21

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = wlen;
    msgs[0].buf = wbuf;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = rlen;
    msgs[1].buf = rbuf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -errno;
    }
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t addr, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    int ret;

    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = len;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    ret = ioctl(dev->fd, I2C_RDWR, &rdwr);
    if (ret < 0) {
        return -errno;
    }
    return 0;
}

static int emc1413_read_register(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    uint8_t wbuf[1] = { reg };
    return emc1413_write_then_read(dev, EMC1413_I2C_ADDR, wbuf, 1, val, 1);
}

static int emc1413_probe(struct emc1413_dev *dev)
{
    uint8_t val;
    int ret;

    ret = emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &val);
    if (ret < 0) return ret;
    if (val != EMC1413_EXPECTED_MANUFACTURER_ID) return -ENODEV;

    ret = emc1413_read_register(dev, EMC1413_REG_PRODUCT_ID, &val);
    if (ret < 0) return ret;
    if (val != EMC1413_EXPECTED_PRODUCT_ID) return -ENODEV;

    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    int ret;
    uint8_t wbuf[2];

    (void)bus_handle;

    ret = emc1413_probe(dev);
    if (ret < 0) return ret;

    wbuf[0] = EMC1413_REG_CONFIG;
    wbuf[1] = 0x00;
    ret = emc1413_write(dev, EMC1413_I2C_ADDR, wbuf, 2);
    if (ret < 0) return ret;

    wbuf[0] = EMC1413_REG_CONV_RATE;
    wbuf[1] = 0x06;
    ret = emc1413_write(dev, EMC1413_I2C_ADDR, wbuf, 2);
    if (ret < 0) return ret;

    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high, low;
    int ret;

    ret = emc1413_read_register(dev, high_reg, &high);
    if (ret < 0) return ret;

    ret = emc1413_read_register(dev, low_reg, &low);
    if (ret < 0) return ret;

    *temp_milli = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
