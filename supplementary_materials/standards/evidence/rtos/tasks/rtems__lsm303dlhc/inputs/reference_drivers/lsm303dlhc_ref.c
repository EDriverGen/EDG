#include "lsm303dlhc_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int lsm303dlhc_transfer(struct lsm303dlhc_device *dev, struct i2c_msg *msgs,
                               uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || msgs == 0 || nmsgs == 0) {
        return -1;
    }
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int lsm303dlhc_read_reg(struct lsm303dlhc_device *dev, uint8_t addr,
                               uint8_t reg, uint8_t *value)
{
    struct i2c_msg msgs[2];
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 1;
    msgs[1].buf = value;
    return lsm303dlhc_transfer(dev, msgs, 2);
}

static int lsm303dlhc_write_reg(struct lsm303dlhc_device *dev, uint8_t addr,
                                uint8_t reg, uint8_t value)
{
    struct i2c_msg msg;
    uint8_t frame[2] = {reg, value};
    msg.addr = addr;
    msg.flags = 0;
    msg.len = sizeof(frame);
    msg.buf = frame;
    return lsm303dlhc_transfer(dev, &msg, 1);
}

static int lsm303dlhc_read_multi(struct lsm303dlhc_device *dev, uint8_t addr,
                                 uint8_t start_reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg msgs[2];
    uint8_t reg = (uint8_t)(start_reg | 0x80U);
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    return lsm303dlhc_transfer(dev, msgs, 2);
}

int lsm303dlhc_init(struct lsm303dlhc_device *dev, const char *bus_path)
{
    if (dev == 0 || bus_path == 0) {
        return -1;
    }
    dev->bus_path = bus_path;
    return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
    uint8_t ira = 0, irb = 0, irc = 0;
    if (lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRA_REG_M, &ira) != 0 ||
        lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRB_REG_M, &irb) != 0 ||
        lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRC_REG_M, &irc) != 0) {
        return -1;
    }
    return ira == LSM303DLHC_IRA_VALUE &&
           irb == LSM303DLHC_IRB_VALUE &&
           irc == LSM303DLHC_IRC_VALUE ? 0 : -1;
}

int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev)
{
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG1_A,
                             LSM303DLHC_ODR_50HZ | LSM303DLHC_AXES_ENABLE) != 0) {
        return -1;
    }
    return lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG4_A,
                                LSM303DLHC_FS_2G | LSM303DLHC_HR_BIT);
}

int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel)
{
    uint8_t data[6];
    if (accel == 0) {
        return -1;
    }
    if (lsm303dlhc_read_multi(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_OUT_X_L_A,
                              data, sizeof(data)) != 0) {
        return -1;
    }
    accel->x = ((int16_t)(((uint16_t)data[1] << 8) | data[0])) >> 4;
    accel->y = ((int16_t)(((uint16_t)data[3] << 8) | data[2])) >> 4;
    accel->z = ((int16_t)(((uint16_t)data[5] << 8) | data[4])) >> 4;
    return 0;
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev)
{
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRA_REG_M,
                             LSM303DLHC_MAG_ODR_15HZ) != 0) {
        return -1;
    }
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRB_REG_M,
                             LSM303DLHC_MAG_GAIN_1_3) != 0) {
        return -1;
    }
    return lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_MR_REG_M,
                                LSM303DLHC_MAG_CONTINUOUS);
}

int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag)
{
    uint8_t data[6];
    if (mag == 0) {
        return -1;
    }
    if (lsm303dlhc_read_multi(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_OUT_X_H_M,
                              data, sizeof(data)) != 0) {
        return -1;
    }
    mag->x = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    mag->z = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    mag->y = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    return 0;
}
