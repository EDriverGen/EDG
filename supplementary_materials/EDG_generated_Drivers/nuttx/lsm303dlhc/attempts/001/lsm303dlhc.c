#include "lsm303dlhc.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <nuttx/i2c/i2c_master.h>
#include "arch.h"

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

static int i2c_write_reg(struct i2c_master_s *bus, uint8_t addr, uint8_t reg, uint8_t val)
{
    struct i2c_msg_s msg;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    msg.frequency = 100000;
    msg.addr = addr;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 2;
    return I2C_TRANSFER(bus, &msg, 1);
}

static int i2c_write_then_read(struct i2c_master_s *bus, uint8_t addr, uint8_t reg, uint8_t *rbuf, int rlen)
{
    struct i2c_msg_s msg[2];
    msg[0].frequency = 100000;
    msg[0].addr = addr;
    msg[0].flags = 0;
    msg[0].buffer = &reg;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = rbuf;
    msg[1].length = rlen;
    return I2C_TRANSFER(bus, msg, 2);
}

int lsm303dlhc_init(struct lsm303dlhc_dev_s *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    ret = i2c_write_reg(bus, ACCEL_ADDR, CTRL_REG1_A, 0x57);
    if (ret < 0) return -EIO;
    ret = i2c_write_reg(bus, ACCEL_ADDR, CTRL_REG4_A, 0x08);
    if (ret < 0) return -EIO;
    up_mdelay(70);

    ret = i2c_write_reg(bus, MAG_ADDR, CRA_REG_M, 0x0C);
    if (ret < 0) return -EIO;
    ret = i2c_write_reg(bus, MAG_ADDR, CRB_REG_M, 0x20);
    if (ret < 0) return -EIO;
    ret = i2c_write_reg(bus, MAG_ADDR, MR_REG_M, 0x00);
    if (ret < 0) return -EIO;
    up_mdelay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev_s *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    int ret;
    ret = i2c_write_then_read(dev->bus, dev->accel_addr, 0xA8, buf, 6);
    if (ret < 0) return -EIO;
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev_s *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
    uint8_t buf[6];
    int ret;
    ret = i2c_write_then_read(dev->bus, dev->mag_addr, 0x03, buf, 6);
    if (ret < 0) return -EIO;
    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev_s *dev, int32_t *t)
{
    uint8_t buf[2];
    int ret;
    ret = i2c_write_then_read(dev->bus, dev->mag_addr, 0x31, buf, 2);
    if (ret < 0) return -EIO;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}