#include "tmp105_ref.h"


static int openharmony_i2c_write(DevHandle bus, uint16_t addr,
                                 const uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = (uint8_t *)data;
    msg.len = len;
    msg.flags = 0;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_read(DevHandle bus, uint16_t addr,
                                uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = data;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_write_read(DevHandle bus, uint16_t addr,
                                      const uint8_t *wdata, uint16_t wlen,
                                      uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;
    msg[0].addr = addr;
    msg[0].buf = (uint8_t *)wdata;
    msg[0].len = wlen;
    msg[0].flags = 0;
    msg[1].addr = addr;
    msg[1].buf = rdata;
    msg[1].len = rlen;
    msg[1].flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, msg, 2) == 2) ? 0 : -1;
}

static int tmp105_read_reg(struct tmp105_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

int tmp105_init(struct tmp105_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int tmp105_probe(struct tmp105_device *dev) {
    uint8_t buf[2];
    return tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2);
}

int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius) {
    uint8_t buf[2]; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = tmp105_read_reg(dev, TMP105_REG_TEMP, buf, 2);
    if (ret) return ret;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_mcelsius = ((int32_t)(raw >> 4) * 625) / 10;
    return 0;
}
