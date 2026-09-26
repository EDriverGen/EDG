#include "dps310.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define DPS310_I2C_ADDR 0x77
#define DPS310_PSR_B2 0x00
#define DPS310_TMP_B2 0x03
#define DPS310_COEF 0x10
#define DPS310_MEAS_CFG 0x08
#define DPS310_INT_STS 0x0A
#define DPS310_FIFO_STS 0x0B
#define DPS310_ID 0x0D
#define DPS310_RESET 0x0C
#define DPS310_RESET_PATTERN 0x09

static int i2c_write_then_read(struct dps310_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxbytes) {
    msg_t ret;
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, DPS310_I2C_ADDR, &reg, 1, rxbuf, rxbytes, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_write(struct dps310_device *dev, uint8_t reg, uint8_t data) {
    uint8_t txbuf[2] = {reg, data};
    msg_t ret;
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, DPS310_I2C_ADDR, txbuf, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

int dps310_init(struct dps310_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    chThdSleepMilliseconds(12);
    chThdSleepMilliseconds(40);

    uint8_t id;
    if (i2c_write_then_read(dev, DPS310_ID, &id, 1) != 0) return -1;

    if (i2c_write(dev, DPS310_RESET, DPS310_RESET_PATTERN) != 0) return -1;

    chThdSleepMilliseconds(12);

    uint8_t coef[18];
    if (i2c_write_then_read(dev, DPS310_COEF, coef, 18) != 0) return -1;

    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_raw) {
    uint8_t buf[3];
    if (i2c_write_then_read(dev, DPS310_PSR_B2, buf, 3) != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= ~0xFFFFFF;
    }
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(struct dps310_device *dev, int32_t *temp_raw) {
    uint8_t buf[3];
    if (i2c_write_then_read(dev, DPS310_TMP_B2, buf, 3) != 0) return -1;
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= ~0xFFFFFF;
    }
    *temp_raw = raw;
    return 0;
}
