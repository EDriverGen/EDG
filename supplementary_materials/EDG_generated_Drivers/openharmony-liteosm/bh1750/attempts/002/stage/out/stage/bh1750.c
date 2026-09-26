#include "bh1750.h"
#include <stdint.h>
#include <string.h>

#include "openharmony_liteosm.h"
#define BH1750_I2C_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_MEAS_DELAY_MS 180

static int32_t bh1750_i2c_write(struct bh1750_device *dev, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int32_t bh1750_i2c_read(struct bh1750_device *dev, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = 1;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

int32_t bh1750_init(struct bh1750_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BH1750_I2C_ADDR;
    uint8_t cmd = BH1750_CMD_POWER_ON;
    return bh1750_i2c_write(dev, &cmd, 1);
}

int32_t bh1750_read_illuminance(struct bh1750_device *dev, int32_t *raw)
{
    if (dev == NULL || raw == NULL) {
        return -1;
    }
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    int32_t ret = bh1750_i2c_write(dev, &cmd, 1);
    if (ret != 0) {
        return -1;
    }
    OsalMSleep(BH1750_MEAS_DELAY_MS);
    uint8_t buf[2];
    ret = bh1750_i2c_read(dev, buf, 2);
    if (ret != 0) {
        return -1;
    }
    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}
