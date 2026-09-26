#include "dps310.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define DPS310_I2C_ADDR 0x77
#define PSR_B2 0x00
#define TMP_B2 0x03
#define RESET 0x0C
#define ID 0x0D
#define COEF 0x10
#define MEAS_CFG 0x08
#define INT_STS 0x0A
#define FIFO_STS 0x0B
#define SOFT_RESET_VAL 0x09

static int i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return ret;
    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return ret;
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return ret;
    return 0;
}

static int i2c_write(struct dps310_dev *dev, uint8_t reg, uint8_t val)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return ret;
    uint8_t buf[2] = {reg, val};
    ret = PrivWrite(dev->fd, buf, 2);
    if (ret < 0) return ret;
    return 0;
}

static int32_t sign_extend24(uint32_t raw)
{
    if (raw & 0x800000) {
        return (int32_t)(raw | 0xFF000000);
    } else {
        return (int32_t)raw;
    }
}

int dps310_init(struct dps310_dev *dev, struct I2cBus *bus_handle)
{
    int ret;
    uint8_t buf[1];
    dev->bus = bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -1;
    PrivTaskDelay(12);
    PrivTaskDelay(40);
    ret = i2c_write_then_read(dev, ID, buf, 1);
    if (ret < 0) return ret;
    ret = i2c_write(dev, RESET, SOFT_RESET_VAL);
    if (ret < 0) return ret;
    PrivTaskDelay(12);
    PrivTaskDelay(40);
    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    int ret;
    uint8_t buf[3];
    ret = i2c_write_then_read(dev, PSR_B2, buf, 3);
    if (ret < 0) return ret;
    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    *pressure_raw = sign_extend24(raw);
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    int ret;
    uint8_t buf[3];
    ret = i2c_write_then_read(dev, TMP_B2, buf, 3);
    if (ret < 0) return ret;
    uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    *temp_raw = sign_extend24(raw);
    return 0;
}
