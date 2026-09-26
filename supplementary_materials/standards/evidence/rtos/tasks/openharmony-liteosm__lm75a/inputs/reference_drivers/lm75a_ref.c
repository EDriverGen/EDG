#include "lm75a_ref.h"


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

static int lm75a_read_reg(struct lm75a_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

int lm75a_init(struct lm75a_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int lm75a_probe(struct lm75a_device *dev) {
    uint8_t conf;
    return lm75a_read_reg(dev, LM75A_REG_CONF, &conf, 1);
}

int lm75a_read_temperature(struct lm75a_device *dev, int32_t *temp_mcelsius) {
    uint8_t buf[2]; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2);
    if (ret) return ret;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_mcelsius = (int32_t)(raw >> 5) * 125;
    return 0;
}

int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw_out) {
    uint8_t buf[2]; int ret;
    if (!dev || !raw_out) return -1;
    ret = lm75a_read_reg(dev, LM75A_REG_TEMP, buf, 2);
    if (ret) return ret;
    /*
     * LM75A stores the signed 11-bit temperature in bits [15:5] of
     * a 16-bit big-endian register (bits [4:0] are always zero).
     * Decode into eighth-celsius units (0.125 C / LSB): load as
     * unsigned, cast to signed, arithmetic right-shift by 5.
     * Range: [-55 C -> -440, +125 C -> +1000].
     */
    uint16_t reg_u = ((uint16_t)buf[0] << 8) | buf[1];
    *raw_out = (int16_t)((int16_t)reg_u >> 5);
    return 0;
}
