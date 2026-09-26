#include "lsm303dlhc.h"
#include <string.h>

static int i2c_write_reg(struct rt_i2c_bus_device *bus, uint8_t addr, uint8_t reg, uint8_t val)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2] = {reg, val};
    msg.addr  = addr;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = 2;
    if (rt_i2c_transfer(bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int i2c_write_then_read(struct rt_i2c_bus_device *bus, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
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
    if (rt_i2c_transfer(bus, msgs, 2) != 2)
        return -1;
    return 0;
}

int lsm303dlhc_init(lsm303dlhc_device_t *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    if (i2c_write_reg(bus, LSM303DLHC_ACCEL_ADDR, 0x20, 0x57) != 0)
        return -1;
    if (i2c_write_reg(bus, LSM303DLHC_ACCEL_ADDR, 0x23, 0x08) != 0)
        return -1;
    rt_thread_mdelay(70);
    if (i2c_write_reg(bus, LSM303DLHC_MAG_ADDR, 0x00, 0x0C) != 0)
        return -1;
    if (i2c_write_reg(bus, LSM303DLHC_MAG_ADDR, 0x01, 0x20) != 0)
        return -1;
    if (i2c_write_reg(bus, LSM303DLHC_MAG_ADDR, 0x02, 0x00) != 0)
        return -1;
    rt_thread_mdelay(1);
    return 0;
}

static int read_accel_raw(lsm303dlhc_device_t *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    if (i2c_write_then_read(dev->bus, LSM303DLHC_ACCEL_ADDR, 0xA8, buf, 6) != 0)
        return -1;
    *x = (int16_t)((buf[0] << 8) | buf[1]);
    *y = (int16_t)((buf[2] << 8) | buf[3]);
    *z = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

static int read_mag_raw(lsm303dlhc_device_t *dev, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    if (i2c_write_then_read(dev->bus, LSM303DLHC_MAG_ADDR, 0x03, buf, 6) != 0)
        return -1;
    *x = (int16_t)((buf[0] << 8) | buf[1]);
    *z = (int16_t)((buf[2] << 8) | buf[3]);
    *y = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

static int read_temp_raw(lsm303dlhc_device_t *dev, int16_t *temp)
{
    uint8_t buf[2];
    if (i2c_write_then_read(dev->bus, LSM303DLHC_MAG_ADDR, 0x31, buf, 2) != 0)
        return -1;
    *temp = (int16_t)((buf[0] << 8) | buf[1]);
    return 0;
}

int lsm303dlhc_read_accel_x(lsm303dlhc_device_t *dev, int32_t *ax)
{
    int16_t x, y, z;
    if (read_accel_raw(dev, &x, &y, &z) != 0)
        return -1;
    *ax = x * 1;
    return 0;
}

int lsm303dlhc_read_accel_y(lsm303dlhc_device_t *dev, int32_t *ay)
{
    int16_t x, y, z;
    if (read_accel_raw(dev, &x, &y, &z) != 0)
        return -1;
    *ay = y * 1;
    return 0;
}

int lsm303dlhc_read_accel_z(lsm303dlhc_device_t *dev, int32_t *az)
{
    int16_t x, y, z;
    if (read_accel_raw(dev, &x, &y, &z) != 0)
        return -1;
    *az = z * 1;
    return 0;
}

int lsm303dlhc_read_mag_x(lsm303dlhc_device_t *dev, int32_t *mx)
{
    int16_t x, y, z;
    if (read_mag_raw(dev, &x, &y, &z) != 0)
        return -1;
    *mx = ((int32_t)x * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_mag_y(lsm303dlhc_device_t *dev, int32_t *my)
{
    int16_t x, y, z;
    if (read_mag_raw(dev, &x, &y, &z) != 0)
        return -1;
    *my = ((int32_t)y * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_mag_z(lsm303dlhc_device_t *dev, int32_t *mz)
{
    int16_t x, y, z;
    if (read_mag_raw(dev, &x, &y, &z) != 0)
        return -1;
    *mz = ((int32_t)z * 1000) / 1100;
    return 0;
}

int lsm303dlhc_read_temperature(lsm303dlhc_device_t *dev, int32_t *temp)
{
    int16_t t;
    if (read_temp_raw(dev, &t) != 0)
        return -1;
    *temp = (int32_t)t * 125;
    return 0;
}
