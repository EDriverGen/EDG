#include "dps310.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_COEF   0x10
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_ID     0x0D
#define DPS310_REG_RESET  0x0C

static int dps310_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t cmd[1] = { reg };
    int ret;
    ret = PrivWrite(dev->fd, cmd, 1);
    if (ret < 0) return -EIO;
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return -EIO;
    return 0;
}

static int dps310_write(struct dps310_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t cmd[2] = { reg, val };
    int ret = PrivWrite(dev->fd, cmd, 2);
    if (ret < 0) return -EIO;
    return 0;
}

static int32_t sign_extend24(uint32_t raw)
{
    if (raw & 0x800000) {
        return (int32_t)(raw | 0xFF000000);
    }
    return (int32_t)raw;
}

int dps310_init(struct dps310_dev *dev, struct I2cBus *bus)
{
    int ret;
    uint8_t buf[1];
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = DPS310_I2C_ADDR;

    dev->bus = bus;
    dev->i2c_addr = DPS310_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -ENODEV;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) { PrivClose(dev->fd); return -EIO; }

    PrivTaskDelay(12);
    PrivTaskDelay(40);

    ret = dps310_write_then_read(dev, DPS310_REG_ID, buf, 1);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    ret = dps310_write(dev, DPS310_REG_RESET, 0x09);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    PrivTaskDelay(12);
    PrivTaskDelay(40);

    ret = dps310_write_then_read(dev, DPS310_REG_COEF, buf, 1);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    int ret = dps310_write_then_read(dev, DPS310_REG_PSR_B2, buf, 3);
    if (ret < 0) return ret;
    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    *pressure_raw = sign_extend24(raw);
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t buf[3];
    int ret = dps310_write_then_read(dev, DPS310_REG_TMP_B2, buf, 3);
    if (ret < 0) return ret;
    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    *temp_raw = sign_extend24(raw);
    return 0;
}
