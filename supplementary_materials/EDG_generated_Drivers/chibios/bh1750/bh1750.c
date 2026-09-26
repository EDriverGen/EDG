#include "bh1750.h"
#include <string.h>

#include "chibios.h"
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07
#define BH1750_CMD_POWER_DOWN 0x00

static int bh1750_write_cmd(struct bh1750_ctx *dev, uint8_t cmd) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, BH1750_I2C_ADDR, &cmd, 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int bh1750_read_bytes(struct bh1750_ctx *dev, uint8_t *buf, size_t len) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterReceiveTimeout(i2cp, BH1750_I2C_ADDR, buf, len, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

int bh1750_init(struct bh1750_ctx *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
}

int bh1750_read_illuminance(struct bh1750_ctx *dev, int32_t *raw) {
    if (!dev || !raw) return -1;
    int ret = bh1750_write_cmd(dev, BH1750_CMD_CONT_HRES);
    if (ret != 0) return ret;
    chThdSleepMilliseconds(180);
    uint8_t buf[2];
    ret = bh1750_read_bytes(dev, buf, 2);
    if (ret != 0) return ret;
    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
