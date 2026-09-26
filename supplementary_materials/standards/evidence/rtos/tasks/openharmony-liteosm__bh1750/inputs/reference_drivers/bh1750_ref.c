#include "bh1750_ref.h"


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

#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_RESET      0x07

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd) {
    if (!dev || !dev->bus) return -1;
    return openharmony_i2c_write(dev->bus, dev->addr, &cmd, 1);
}

static int bh1750_read_bytes(struct bh1750_device *dev, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return openharmony_i2c_read(dev->bus, dev->addr, buf, len);
}

int bh1750_init(struct bh1750_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) return -1;
    dev->bus = bus; dev->addr = addr; dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_probe(struct bh1750_device *dev) {
    int ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) return ret;
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw) {
    uint8_t data[2]; int ret;
    if (!dev || !raw) return -1;
    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);  if (ret) return ret;
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);     if (ret) return ret;
    ret = bh1750_write_cmd(dev, dev->mode);             if (ret) return ret;
    OsalMSleep(180);
    ret = bh1750_read_bytes(dev, data, 2);              if (ret) return ret;
    *raw = (uint16_t)((data[0] << 8) | data[1]);
    return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw) {
    return (uint32_t)raw * 100 / 12;
}

int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100) {
    uint16_t raw; int ret = bh1750_read_raw(dev, &raw);
    if (ret) return ret;
    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return 0;
}
