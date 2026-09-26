#include "lsm303dlhc.h"
#include <string.h>
#include "i2c_if.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
#define CTRL_REG1_A 0x20
#define CTRL_REG4_A 0x23
#define CRA_REG_M 0x00
#define CRB_REG_M 0x01
#define MR_REG_M 0x02
#define OUT_X_L_A 0x28
#define OUT_X_H_M 0x03
#define TEMP_OUT_H_M 0x31
#define STATUS_REG_A 0x27
#define SR_REG_Mg 0x09

static int32_t i2c_write(struct lsm303dlhc_dev *dev, uint16_t addr, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[1];
    msgs[0].addr = addr;
    msgs[0].buf = buf;
    msgs[0].len = len;
    msgs[0].flags = 0;
    if (I2cTransfer(dev->bus_handle, msgs, 1) != 1) {
        return -1;
    }
    return 0;
}

static int32_t i2c_write_then_read(struct lsm303dlhc_dev *dev, uint16_t addr, uint8_t *wbuf, uint16_t wlen, uint8_t *rbuf, uint16_t rlen)
{
    struct I2cMsg msgs[2];
    msgs[0].addr = addr;
    msgs[0].buf = wbuf;
    msgs[0].len = wlen;
    msgs[0].flags = 0;
    msgs[1].addr = addr;
    msgs[1].buf = rbuf;
    msgs[1].len = rlen;
    msgs[1].flags = 0x0001; // I2C_FLAG_READ
    if (I2cTransfer(dev->bus_handle, msgs, 2) != 2) {
        return -1;
    }
    return 0;
}

int32_t lsm303dlhc_init(struct lsm303dlhc_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    uint8_t buf[2];
    int32_t ret;

    // Init accelerometer
    buf[0] = CTRL_REG1_A;
    buf[1] = 0x57;
    ret = i2c_write(dev, LSM303DLHC_ACCEL_ADDR, buf, 2);
    if (ret != 0) return ret;

    buf[0] = CTRL_REG4_A;
    buf[1] = 0x08;
    ret = i2c_write(dev, LSM303DLHC_ACCEL_ADDR, buf, 2);
    if (ret != 0) return ret;

    OsalMSleep(70);

    // Init magnetometer
    buf[0] = CRA_REG_M;
    buf[1] = 0x0C;
    ret = i2c_write(dev, LSM303DLHC_MAG_ADDR, buf, 2);
    if (ret != 0) return ret;

    buf[0] = CRB_REG_M;
    buf[1] = 0x20;
    ret = i2c_write(dev, LSM303DLHC_MAG_ADDR, buf, 2);
    if (ret != 0) return ret;

    buf[0] = MR_REG_M;
    buf[1] = 0x00;
    ret = i2c_write(dev, LSM303DLHC_MAG_ADDR, buf, 2);
    if (ret != 0) return ret;

    OsalMSleep(1);

    return 0;
}

int32_t lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int32_t *ax, int32_t *ay, int32_t *az)
{
    uint8_t status;
    uint8_t cmd;
    int32_t ret;

    // Poll STATUS_REG_A for data ready
    cmd = STATUS_REG_A;
    ret = i2c_write_then_read(dev, LSM303DLHC_ACCEL_ADDR, &cmd, 1, &status, 1);
    if (ret != 0) return ret;
    if (!(status & 0x08)) {
        return -1; // data not ready
    }

    // Read 6 bytes from OUT_X_L_A with auto-increment
    cmd = 0xA8; // 0x28 | 0x80
    uint8_t buf[6];
    ret = i2c_write_then_read(dev, LSM303DLHC_ACCEL_ADDR, &cmd, 1, buf, 6);
    if (ret != 0) return ret;

    int16_t raw_x = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t raw_y = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t raw_z = (int16_t)((buf[5] << 8) | buf[4]);

    *ax = raw_x * 1;
    *ay = raw_y * 1;
    *az = raw_z * 1;

    return 0;
}

int32_t lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int32_t *mx, int32_t *my, int32_t *mz)
{
    uint8_t status;
    uint8_t cmd;
    int32_t ret;

    // Poll SR_REG_Mg for data ready
    cmd = SR_REG_Mg;
    ret = i2c_write_then_read(dev, LSM303DLHC_MAG_ADDR, &cmd, 1, &status, 1);
    if (ret != 0) return ret;
    if (!(status & 0x01)) {
        return -1; // data not ready
    }

    // Read 6 bytes from OUT_X_H_M
    cmd = 0x03;
    uint8_t buf[6];
    ret = i2c_write_then_read(dev, LSM303DLHC_MAG_ADDR, &cmd, 1, buf, 6);
    if (ret != 0) return ret;

    // Order: X_H, X_L, Z_H, Z_L, Y_H, Y_L
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_z = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_y = (int16_t)((buf[4] << 8) | buf[5]);

    // Convert using integer approximation: (raw * 1000) / 1100
    *mx = (int32_t)(((int64_t)raw_x * 1000) / 1100);
    *my = (int32_t)(((int64_t)raw_y * 1000) / 1100);
    *mz = (int32_t)(((int64_t)raw_z * 1000) / 1100);

    return 0;
}

int32_t lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t)
{
    uint8_t cmd;
    int32_t ret;

    cmd = TEMP_OUT_H_M;
    uint8_t buf[2];
    ret = i2c_write_then_read(dev, LSM303DLHC_MAG_ADDR, &cmd, 1, buf, 2);
    if (ret != 0) return ret;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)((int64_t)raw * 125);

    return 0;
}
