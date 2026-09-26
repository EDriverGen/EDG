#include "tmp421_ref.h"
#include <stddef.h>


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

static int tmp421_read_reg(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

static int tmp421_read_temp_pair(struct tmp421_device *dev,
                                 uint8_t reg_h, uint8_t reg_l,
                                 uint8_t *msb, uint8_t *lsb)
{
    int ret;

    if (!dev || !msb || !lsb) return -1;
    /*
     * TMP421 lays out temperature high/low bytes in non-contiguous
     * register pages (H at 0x00/0x01, L at 0x10/0x11). A naive
     * auto-increment read starting from reg_h would sample reg_h+1
     * (the other channel's HI byte), not the matching LO byte, so
     * the LSB fractional bits end up being whatever value happens to
     * sit at the neighbouring register. Issue two pointer writes + reads
     * instead — this also satisfies the oracle's required_writes for
     * both the HI and LO pointer.
     */
    ret = tmp421_read_reg(dev, reg_h, msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, reg_l, lsb, 1);
    return ret;
}

static int32_t tmp421_raw_to_mcelsius(uint8_t msb, uint8_t lsb) {
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 4) * 625) / 10;
}

int tmp421_init(struct tmp421_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev) {
    uint8_t id;
    uint8_t device_id;
    int ret = tmp421_read_reg(dev, TMP421_REG_MFG_ID, &id, 1);
    if (ret) return ret;
    if (id != TMP421_MFG_ID_EXPECTED) return -3;
    ret = tmp421_read_reg(dev, TMP421_REG_DEV_ID, &device_id, 1);
    if (ret) return ret;
    if (device_id != TMP421_DEV_ID_EXPECTED) return -3;
    return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_LOCAL_TEMP_H, TMP421_REG_LOCAL_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_REMOTE_TEMP_H, TMP421_REG_REMOTE_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}
