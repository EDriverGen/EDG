#include "adxl345.h"
#include "ch.h"
#include "hal.h"
#include <stdint.h>
#include <string.h>

#include "chibios.h"
static int adxl345_write_reg(ADXL345Device *dev, uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { reg, value };
    uint8_t rx[2] = { 0, 0 };
    spiAcquireBus((SPIDriver *)dev->bus_handle);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 2, tx, rx);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiReleaseBus((SPIDriver *)dev->bus_handle);
    return 0;
}

static int adxl345_read_burst(ADXL345Device *dev, uint8_t reg, uint8_t *buf, size_t len) {
    uint8_t cmd = reg | ADXL345_READ_MASK | ADXL345_MB_MASK;
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    memset(tx, 0, sizeof(tx));
    tx[0] = cmd;
    spiAcquireBus((SPIDriver *)dev->bus_handle);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 1 + len, tx, rx);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiReleaseBus((SPIDriver *)dev->bus_handle);
    memcpy(buf, rx + 1, len);
    return 0;
}

int adxl345_init(ADXL345Device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = ADXL345_I2C_ADDR;
    spiInit();
    adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x00);
    adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x08);
    chThdSleepMilliseconds(12);
    return 0;
}

int adxl345_read_xyz(ADXL345Device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    int ret = adxl345_read_burst(dev, ADXL345_DATAX0, buf, 6);
    if (ret != 0) return ret;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
