#include "emc1413.h"
#include "transform.h"
#include "bus.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_pin.h"
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return ret;
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return ret;
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return PrivWrite(dev->fd, buf, 2);
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    int fd;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = EMC1413_I2C_ADDR;
    uint8_t buf[1];
    int ret;

    (void)bus_handle;

    fd = PrivOpen("/dev/i2c1", 0);
    if (fd < 0) return -EIO;
    dev->fd = fd;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) { PrivClose(fd); return -EIO; }

    PrivTaskDelay(15);

    ret = emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, buf, 1);
    if (ret < 0 || buf[0] != 0x5D) { PrivClose(fd); return -EIO; }

    ret = emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, buf, 1);
    if (ret < 0 || buf[0] != 0x21) { PrivClose(fd); return -EIO; }

    ret = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret < 0) { PrivClose(fd); return -EIO; }

    ret = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret < 0) { PrivClose(fd); return -EIO; }

    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high, low;
    int ret;

    ret = emc1413_write_then_read(dev, reg_high, &high, 1);
    if (ret < 0) return ret;

    ret = emc1413_write_then_read(dev, reg_low, &low, 1);
    if (ret < 0) return ret;

    *temp = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp);
}
