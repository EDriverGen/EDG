#include "lsm303dlhc.h"
#include <string.h>

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

static int i2c_write_reg(lsm303dlhc_device_t *dev, uint8_t addr, uint8_t reg, uint8_t val)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2] = {reg, val};
    msg.addr  = addr;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = 2;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int i2c_write_then_read(lsm303dlhc_device_t *dev, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr  = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;
    msgs[1].addr  = addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

int lsm303dlhc_init(lsm303dlhc_device_t *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    if (i2c_write_reg(dev, ACCEL_ADDR, CTRL_REG1_A, 0x57) != 0)
        return -1;
    if (i2c_write_reg(dev, ACCEL_ADDR, CTRL_REG4_A, 0x08) != 0)
        return -1;
    rt_thread_mdelay(70);

    if (i2c_write_reg(dev, MAG_ADDR, CRA_REG_M, 0x0C) != 0)
        return -1;
    if (i2c_write_reg(dev, MAG_ADDR, CRB_REG_M, 0x20) != 0)
        return -1;
    if (i2c_write_reg(dev, MAG_ADDR, MR_REG_M, 0x00) != 0)
        return -1;
    rt_thread_mdelay(1);

    return 0;
}

static int read_all(lsm303dlhc_device_t *dev)
{
    uint8_t buf[6];
    int16_t raw;

    /* Read accelerometer: 6 bytes from OUT_X_L_A with auto-increment (0xA8) */
    if (i2c_write_then_read(dev, ACCEL_ADDR, 0xA8, buf, 6) != 0)
        return -1;
    dev->accel_raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    dev->accel_raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    dev->accel_raw[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* Read magnetometer: 6 bytes from OUT_X_H_M (0x03) */
    if (i2c_write_then_read(dev, MAG_ADDR, 0x03, buf, 6) != 0)
        return -1;
    dev->mag_raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    dev->mag_raw[2] = (int16_t)((buf[2] << 8) | buf[3]);
    dev->mag_raw[1] = (int16_t)((buf[4] << 8) | buf[5]);

    /* Read temperature: 2 bytes from TEMP_OUT_H_M (0x31) */
    if (i2c_write_then_read(dev, MAG_ADDR, 0x31, buf, 2) != 0)
        return -1;
    dev->temp_raw = (int16_t)((buf[0] << 8) | buf[1]);

    return 0;
}

int lsm303dlhc_read_accel_x(lsm303dlhc_device_t *dev, int32_t *ax)
{
    if (read_all(dev) != 0)
        return -1;
    *ax = (int32_t)dev->accel_raw[0];
    return 0;
}

int lsm303dlhc_read_accel_y(lsm303dlhc_device_t *dev, int32_t *ay)
{
    if (read_all(dev) != 0)
        return -1;
    *ay = (int32_t)dev->accel_raw[1];
    return 0;
}

int lsm303dlhc_read_accel_z(lsm303dlhc_device_t *dev, int32_t *az)
{
    if (read_all(dev) != 0)
        return -1;
    *az = (int32_t)dev->accel_raw[2];
    return 0;
}

int lsm303dlhc_read_mag_x(lsm303dlhc_device_t *dev, int32_t *mx)
{
    if (read_all(dev) != 0)
        return -1;
    *mx = ((int32_t)dev->mag_raw[0] * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_mag_y(lsm303dlhc_device_t *dev, int32_t *my)
{
    if (read_all(dev) != 0)
        return -1;
    *my = ((int32_t)dev->mag_raw[1] * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_mag_z(lsm303dlhc_device_t *dev, int32_t *mz)
{
    if (read_all(dev) != 0)
        return -1;
    *mz = ((int32_t)dev->mag_raw[2] * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_temperature(lsm303dlhc_device_t *dev, int32_t *temp)
{
    if (read_all(dev) != 0)
        return -1;
    *temp = (int32_t)dev->temp_raw * 125;
    return 0;
}
