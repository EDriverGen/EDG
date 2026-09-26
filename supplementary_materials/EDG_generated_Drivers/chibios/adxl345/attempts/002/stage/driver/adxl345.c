#include "adxl345.h"
#include "ch.h"
#include "hal.h"
#include <stdint.h>
#include <string.h>

#include "chibios.h"
#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg) (0xC0 | (reg))

static void adxl345_write_reg(ADXL345Device *dev, uint8_t reg, uint8_t val) {
    uint8_t tx[2] = {reg, val};
    uint8_t rx[2];
    spiAcquireBus((SPIDriver *)dev->bus_handle);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 2, tx, rx);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiReleaseBus((SPIDriver *)dev->bus_handle);
}

static void adxl345_read_burst(ADXL345Device *dev, uint8_t reg, uint8_t *buf, size_t len) {
    uint8_t cmd = ADXL345_MB_CMD(reg);
    uint8_t tx[1 + len];
    uint8_t rx[1 + len];
    tx[0] = cmd;
    memset(&tx[1], 0, len);
    spiAcquireBus((SPIDriver *)dev->bus_handle);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 1 + len, tx, rx);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiReleaseBus((SPIDriver *)dev->bus_handle);
    memcpy(buf, &rx[1], len);
}

void adxl345_init(ADXL345Device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = ADXL345_I2C_ADDR;
    spiInit();
    adxl345_write_reg(dev, 0x2D, 0x00);
    adxl345_write_reg(dev, 0x2D, 0x08);
}

void adxl345_read_xyz(ADXL345Device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    adxl345_read_burst(dev, 0x32, buf, 6);
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
}
