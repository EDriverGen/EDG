#include "tmp105.h"
#include "transform.h"
#include "bus.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "bus_pin.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    int fd = PrivOpen("/dev/i2c1", 0);
    if (fd < 0) {
        return -1;
    }
    uint16_t addr = TMP105_I2C_ADDR;
    struct PrivIoctlCfg ioctl_cfg;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    int ret = PrivIoctl(fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(fd);
        return -1;
    }
    dev->fd = fd;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -1;
    }
    // Wait for conversion time (12-bit resolution typical 220ms)
    PrivTaskDelay(220);

    uint8_t cmd = TMP105_PTR_TEMP;
    int ret = PrivWrite(dev->fd, &cmd, 1);
    if (ret < 0) {
        return -1;
    }

    uint8_t buf[2];
    ret = PrivRead(dev->fd, buf, 2);
    if (ret < 0) {
        return -1;
    }

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw12 = (int16_t)(raw16 >> 4);
    // Sign extend from bit 11
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    // Convert to milli_degC: raw12 * 625 / 10
    int32_t result = ((int32_t)raw12 * 625) / 10;
    *raw = result;
    return 0;
}
