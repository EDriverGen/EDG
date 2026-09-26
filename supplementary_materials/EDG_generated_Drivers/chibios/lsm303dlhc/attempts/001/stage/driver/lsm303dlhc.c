#include "lsm303dlhc.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define ACCEL_ADDR 0x19
#define MAG_ADDR 0x1E

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

#define TIMEOUT_MS 100

static int i2c_write(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t data) {
    uint8_t txbuf[2] = {reg, data};
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, addr, txbuf, 2, NULL, 0, MS2ST(TIMEOUT_MS));
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_write_then_read(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t *rxbuf, size_t len) {
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, addr, &reg, 1, rxbuf, len, MS2ST(TIMEOUT_MS));
    return (ret == MSG_OK) ? 0 : -1;
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    // Init accelerometer
    if (i2c_write(dev, dev->accel_addr, CTRL_REG1_A, 0x57) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }
    if (i2c_write(dev, dev->accel_addr, CTRL_REG4_A, 0x08) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    // Delay 70 ms
    chThdSleepMilliseconds(70);

    // Init magnetometer
    if (i2c_write(dev, dev->mag_addr, CRA_REG_M, 0x0C) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }
    if (i2c_write(dev, dev->mag_addr, CRB_REG_M, 0x20) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }
    if (i2c_write(dev, dev->mag_addr, MR_REG_M, 0x00) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    // Delay 1 ms
    chThdSleepMilliseconds(1);

    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t status;
    uint8_t buf[6];

    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    // Poll STATUS_REG_A for data ready
    do {
        if (i2c_write_then_read(dev, dev->accel_addr, STATUS_REG_A, &status, 1) != 0) {
            i2cReleaseBus((I2CDriver *)dev->bus_handle);
            return -1;
        }
    } while (!(status & 0x08));

    // Read 6 bytes from OUT_X_L_A with auto-increment (0x28 | 0x80 = 0xA8)
    if (i2c_write_then_read(dev, dev->accel_addr, 0xA8, buf, 6) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    i2cReleaseBus((I2CDriver *)dev->bus_handle);

    // Combine bytes: buf[0]=X_L, buf[1]=X_H, buf[2]=Y_L, buf[3]=Y_H, buf[4]=Z_L, buf[5]=Z_H
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);

    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz) {
    uint8_t status;
    uint8_t buf[6];

    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    // Poll SR_REG_Mg for data ready
    do {
        if (i2c_write_then_read(dev, dev->mag_addr, SR_REG_Mg, &status, 1) != 0) {
            i2cReleaseBus((I2CDriver *)dev->bus_handle);
            return -1;
        }
    } while (!(status & 0x01));

    // Read 6 bytes from OUT_X_H_M (0x03)
    if (i2c_write_then_read(dev, dev->mag_addr, OUT_X_H_M, buf, 6) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    i2cReleaseBus((I2CDriver *)dev->bus_handle);

    // Order: X_H, X_L, Z_H, Z_L, Y_H, Y_L
    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);

    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t) {
    uint8_t buf[2];

    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    // Read 2 bytes from TEMP_OUT_H_M (0x31)
    if (i2c_write_then_read(dev, dev->mag_addr, TEMP_OUT_H_M, buf, 2) != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    i2cReleaseBus((I2CDriver *)dev->bus_handle);

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;

    return 0;
}
