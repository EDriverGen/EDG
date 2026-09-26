#include "lm75a.h"
#include <string.h>
#include <ch.h>

#define LM75A_I2C_ADDR 0x48
#define LM75A_TEMP_REG 0x00

int lm75a_init(struct lm75a_ctx *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    chThdSleepMilliseconds(100);
    return 0;
}

int lm75a_read_temp(struct lm75a_ctx *dev, int32_t *raw) {
    if (!dev || !raw) return -1;
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[1] = {LM75A_TEMP_REG};
    uint8_t rxbuf[2];
    msg_t ret;

    i2cAcquireBus(i2cp);
    ret = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 1, rxbuf, 2, TIME_MS2I(100));
    i2cReleaseBus(i2cp);

    if (ret != MSG_OK) return -1;

    uint16_t raw16 = ((uint16_t)rxbuf[0] << 8) | rxbuf[1];
    int16_t signed11 = (int16_t)(raw16 >> 5);
    if (signed11 & 0x0400) {
        signed11 |= 0xF800;
    }
    *raw = (int32_t)signed11;
    return 0;
}