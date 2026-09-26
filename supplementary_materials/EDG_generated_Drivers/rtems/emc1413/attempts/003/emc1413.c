#include "emc1413.h"
#include "rtems.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
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
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t reg_buf = reg;

    msgs[0].addr = EMC1413_I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;

    msgs[1].addr = EMC1413_I2C_ADDR;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 2;

    if (ioctl(dev->fd, I2C_RDWR, &rdwr) < 0) {
        return -errno;
    }
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr;
    uint8_t buf[2] = {reg, val};

    msgs[0].addr = EMC1413_I2C_ADDR;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = buf;

    rdwr.msgs = msgs;
    rdwr.nmsgs = 1;

    if (ioctl(dev->fd, I2C_RDWR, &rdwr) < 0) {
        return -errno;
    }
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

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    int ret;
    uint8_t buf;

    /* bus_handle is actually a pointer to dev, which already has fd set */
    (void)bus_handle;

    /* Probe: read Manufacturer ID */
    ret = emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, &buf, 1);
    if (ret < 0) return ret;
    if (buf != 0x5D) return -ENODEV;

    /* Probe: read Product ID */
    ret = emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, &buf, 1);
    if (ret < 0) return ret;
    if (buf != 0x21) return -ENODEV;

    /* Write Configuration: 0x00 */
    ret = emc1413_write(dev, EMC1413_REG_CONFIG, 0x00);
    if (ret < 0) return ret;

    /* Write Conversion Rate: 0x06 */
    ret = emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06);
    if (ret < 0) return ret;

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
