#include "lsm303dlhc.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "xtimer.h"

#include "riot.h"
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

int lsm303dlhc_init(lsm303dlhc_t *dev, i2c_t bus) {
    dev->bus = bus;
    uint8_t data[2];
    int ret;

    // Accelerometer init
    data[0] = CTRL_REG1_A;
    data[1] = 0x57;
    ret = i2c_write_bytes(bus, ACCEL_ADDR, data, 2, 0);
    if (ret < 0) return -EIO;

    data[0] = CTRL_REG4_A;
    data[1] = 0x08;
    ret = i2c_write_bytes(bus, ACCEL_ADDR, data, 2, 0);
    if (ret < 0) return -EIO;

    xtimer_msleep(70);

    // Magnetometer init
    data[0] = CRA_REG_M;
    data[1] = 0x0C;
    ret = i2c_write_bytes(bus, MAG_ADDR, data, 2, 0);
    if (ret < 0) return -EIO;

    data[0] = CRB_REG_M;
    data[1] = 0x20;
    ret = i2c_write_bytes(bus, MAG_ADDR, data, 2, 0);
    if (ret < 0) return -EIO;

    data[0] = MR_REG_M;
    data[1] = 0x00;
    ret = i2c_write_bytes(bus, MAG_ADDR, data, 2, 0);
    if (ret < 0) return -EIO;

    xtimer_msleep(1);

    return 0;
}

int lsm303dlhc_read_accel(lsm303dlhc_t *dev, int32_t *ax, int32_t *ay, int32_t *az) {
    uint8_t buf[6];
    int ret;

    ret = i2c_read_regs(dev->bus, ACCEL_ADDR, 0xA8, buf, 6, 0);
    if (ret < 0) return -EIO;

    int16_t raw_x = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t raw_y = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t raw_z = (int16_t)((buf[5] << 8) | buf[4]);

    *ax = raw_x * 1;
    *ay = raw_y * 1;
    *az = raw_z * 1;

    return 0;
}

int lsm303dlhc_read_mag(lsm303dlhc_t *dev, int32_t *mx, int32_t *my, int32_t *mz, int32_t *t) {
    uint8_t buf[6];
    int ret;

    ret = i2c_read_regs(dev->bus, MAG_ADDR, 0x03, buf, 6, 0);
    if (ret < 0) return -EIO;

    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_z = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_y = (int16_t)((buf[4] << 8) | buf[5]);

    *mx = ((int32_t)raw_x * 1000) / 1100;
    *my = ((int32_t)raw_y * 1000) / 1100;
    *mz = ((int32_t)raw_z * 1000) / 1100;

    uint8_t temp_buf[2];
    ret = i2c_read_regs(dev->bus, MAG_ADDR, 0x31, temp_buf, 2, 0);
    if (ret < 0) return -EIO;

    int16_t raw_temp = (int16_t)((temp_buf[0] << 8) | temp_buf[1]);
    *t = (int32_t)raw_temp * 125;

    return 0;
}
