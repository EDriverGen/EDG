#include "lsm303dlhc.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define ACCEL_ADDR 0x19
#define MAG_ADDR   0x1E

#define CTRL_REG1_A 0x20
#define CTRL_REG4_A 0x23
#define CRA_REG_M   0x00
#define CRB_REG_M   0x01
#define MR_REG_M    0x02
#define OUT_X_L_A   0x28
#define OUT_X_H_M   0x03
#define TEMP_OUT_H_M 0x31

static int i2c_write(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) != 0)
        return -EIO;
    if (PrivWrite(dev->fd, buf, 2) != 2)
        return -EIO;
    return 0;
}

static int i2c_write_then_read(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t *rxbuf, uint16_t len)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) != 0)
        return -EIO;
    if (PrivWrite(dev->fd, &reg, 1) != 1)
        return -EIO;
    if (PrivRead(dev->fd, rxbuf, len) != len)
        return -EIO;
    return 0;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, struct I2cBus *bus_handle)
{
    dev->bus = bus_handle;
    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0)
        return -EIO;

    if (i2c_write(dev, ACCEL_ADDR, CTRL_REG1_A, 0x57) != 0)
        goto err;
    if (i2c_write(dev, ACCEL_ADDR, CTRL_REG4_A, 0x08) != 0)
        goto err;
    PrivTaskDelay(70);

    if (i2c_write(dev, MAG_ADDR, CRA_REG_M, 0x0C) != 0)
        goto err;
    if (i2c_write(dev, MAG_ADDR, CRB_REG_M, 0x20) != 0)
        goto err;
    if (i2c_write(dev, MAG_ADDR, MR_REG_M, 0x00) != 0)
        goto err;
    PrivTaskDelay(1);

    return 0;
err:
    PrivClose(dev->fd);
    dev->fd = -1;
    return -EIO;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    if (i2c_write_then_read(dev, ACCEL_ADDR, 0xA8, buf, 6) != 0)
        return -EIO;
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    uint8_t buf[6];
    if (i2c_write_then_read(dev, MAG_ADDR, 0x03, buf, 6) != 0)
        return -EIO;
    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    uint8_t buf[2];
    if (i2c_write_then_read(dev, MAG_ADDR, 0x31, buf, 2) != 0)
        return -EIO;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
