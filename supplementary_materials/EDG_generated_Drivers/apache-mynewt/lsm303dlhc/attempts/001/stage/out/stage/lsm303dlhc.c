#include "lsm303dlhc.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
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

static int i2c_write(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t val) {
    struct hal_i2c_master_data data;
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    data.address = addr;
    data.buffer = buf;
    data.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
}

static int i2c_write_then_read(struct lsm303dlhc_dev *dev, uint8_t addr, uint8_t reg, uint8_t *rbuf, uint16_t rlen) {
    struct hal_i2c_master_data wdata, rdata;
    int rc;
    wdata.address = addr;
    wdata.buffer = &reg;
    wdata.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &wdata, OS_TIMEOUT_NEVER, 0);
    if (rc != 0) return rc;
    rdata.address = addr;
    rdata.buffer = rbuf;
    rdata.len = rlen;
    return hal_i2c_master_read(dev->i2c_num, &rdata, OS_TIMEOUT_NEVER, 1);
}

int lsm303dlhc_init(struct lsm303dlhc_dev *dev, void *bus_handle) {
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->accel_addr = ACCEL_ADDR;
    dev->mag_addr = MAG_ADDR;

    int rc;
    rc = i2c_write(dev, dev->accel_addr, CTRL_REG1_A, 0x57);
    if (rc) return rc;
    rc = i2c_write(dev, dev->accel_addr, CTRL_REG4_A, 0x08);
    if (rc) return rc;
    os_time_delay(70);

    rc = i2c_write(dev, dev->mag_addr, CRA_REG_M, 0x0C);
    if (rc) return rc;
    rc = i2c_write(dev, dev->mag_addr, CRB_REG_M, 0x20);
    if (rc) return rc;
    rc = i2c_write(dev, dev->mag_addr, MR_REG_M, 0x00);
    if (rc) return rc;
    os_time_delay(1);

    return 0;
}

int lsm303dlhc_read_accel(struct lsm303dlhc_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    int rc = i2c_write_then_read(dev, dev->accel_addr, 0xA8, buf, 6);
    if (rc) return rc;
    *ax = (int16_t)((buf[1] << 8) | buf[0]);
    *ay = (int16_t)((buf[3] << 8) | buf[2]);
    *az = (int16_t)((buf[5] << 8) | buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_dev *dev, int16_t *mx, int16_t *my, int16_t *mz) {
    uint8_t buf[6];
    int rc = i2c_write_then_read(dev, dev->mag_addr, 0x03, buf, 6);
    if (rc) return rc;
    *mx = (int16_t)((buf[0] << 8) | buf[1]);
    *mz = (int16_t)((buf[2] << 8) | buf[3]);
    *my = (int16_t)((buf[4] << 8) | buf[5]);
    return 0;
}

int lsm303dlhc_read_temp(struct lsm303dlhc_dev *dev, int32_t *t) {
    uint8_t buf[2];
    int rc = i2c_write_then_read(dev, dev->mag_addr, 0x31, buf, 2);
    if (rc) return rc;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *t = (int32_t)raw * 125;
    return 0;
}
