#include "dps310.h"
#include "xtimer.h"
#include <stddef.h>

#include "riot.h"
#define DPS310_ADDR 0x77

#define REG_PSR_B2  0x00
#define REG_TMP_B2  0x03
#define REG_COEF    0x10
#define REG_MEAS_CFG 0x08
#define REG_INT_STS 0x0A
#define REG_FIFO_STS 0x0B
#define REG_RESET   0x0C
#define REG_ID      0x0D

#define RESET_SOFT_RST 0x09

static int read_regs(dps310_t *dev, uint16_t reg, void *data, size_t len) {
    return i2c_read_regs(dev->bus, dev->addr, reg, data, len, 0);
}

static int write_reg(dps310_t *dev, uint16_t reg, uint8_t value) {
    uint8_t buf[2] = { (uint8_t)(reg & 0xFF), value };
    return i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
}

int dps310_init(dps310_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = DPS310_ADDR;

    xtimer_msleep(12);
    xtimer_msleep(40);

    uint8_t id;
    int ret = read_regs(dev, REG_ID, &id, 1);
    if (ret != 0) return -1;

    ret = write_reg(dev, REG_RESET, RESET_SOFT_RST);
    if (ret != 0) return -1;

    xtimer_msleep(12);

    uint8_t coeff[18];
    ret = read_regs(dev, REG_COEF, coeff, 18);
    if (ret != 0) return -1;

    return 0;
}

int dps310_read_pressure(dps310_t *dev, int32_t *pressure_raw) {
    uint8_t buf[3];
    int ret = read_regs(dev, REG_PSR_B2, buf, 3);
    if (ret != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(dps310_t *dev, int32_t *temp_raw) {
    uint8_t buf[3];
    int ret = read_regs(dev, REG_TMP_B2, buf, 3);
    if (ret != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *temp_raw = raw;
    return 0;
}
