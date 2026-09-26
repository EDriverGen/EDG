#include "bh1750.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_CMD_RESET 0x07

int32_t bh1750_init(struct bh1750_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;

    struct I2cMsg msgs[1];
    uint8_t cmd = BH1750_CMD_POWER_ON;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &cmd;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

int32_t bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw)
{
    if (dev == NULL || raw == NULL) {
        return -1;
    }

    struct I2cMsg msgs[2];
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &cmd;
    msgs[0].len = 1;
    msgs[0].flags = 0;

    uint8_t buf[2];
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = 2;
    msgs[1].flags = 1;

    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }

    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
