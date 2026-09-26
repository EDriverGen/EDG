#include "emc1413.h"
#include "transform.h"
#include "bus.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define EMC1413_I2C_ADDR 0x4C

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

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int ret;
    ret = emc1413_write_then_read(dev, high_reg, &high_byte, 1);
    if (ret < 0) return ret;
    ret = emc1413_write_then_read(dev, low_reg, &low_byte, 1);
    if (ret < 0) return ret;
    *temp = ((int32_t)high_byte * 1000) + ((((low_byte >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = EMC1413_I2C_ADDR;
    uint8_t buf[1];

    (void)bus_handle;

    dev->i2c_addr = EMC1413_I2C_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return dev->fd;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    PrivTaskDelay(15);

    ret = emc1413_write_then_read(dev, 0xFE, buf, 1);
    if (ret < 0 || buf[0] != 0x5D) { PrivClose(dev->fd); return -ENODEV; }

    ret = emc1413_write_then_read(dev, 0xFD, buf, 1);
    if (ret < 0 || buf[0] != 0x21) { PrivClose(dev->fd); return -ENODEV; }

    ret = emc1413_write(dev, 0x03, 0x00);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    ret = emc1413_write(dev, 0x04, 0x06);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x00, 0x29, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x01, 0x10, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x23, 0x24, temp);
}