#include "emc1413_ref.h"
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

static int emc1413_read_reg(struct emc1413_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

static int emc1413_read_temp_pair(struct emc1413_device *dev, uint8_t reg_h, uint8_t reg_l,
                                  uint8_t *msb, uint8_t *lsb)
{
    int ret;

    if (!dev || !msb || !lsb) return -1;
    ret = emc1413_read_reg(dev, reg_h, msb, 1);
    if (ret != 0) return ret;
    ret = emc1413_read_reg(dev, reg_l, lsb, 1);
    if (ret != 0) return ret;
    return 0;
}

static int32_t emc1413_temp_to_mcelsius(uint8_t msb, uint8_t lsb)
{
    /*
     * EMC1413 temperature registers hold a signed 11-bit value: MSB carries
     * the 8 integer bits and the LSB places the 3 fractional bits at [7:5]
     * (step = 0.125 C). Build a signed int16 (integer byte in the high 8
     * bits) then arithmetic right-shift by 5 so the sign is preserved for
     * negative temperatures such as -20 C (0xEC00 >> 5 = -160 -> -20000 mC).
     */
    int16_t raw = (int16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
    return (int32_t)(raw >> 5) * 125;
}

int emc1413_init(struct emc1413_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int emc1413_probe(struct emc1413_device *dev) {
    uint8_t id;
    uint8_t product_id;
    int ret = emc1413_read_reg(dev, EMC1413_REG_MFG_ID, &id, 1);
    if (ret) return ret;
    if (id != EMC1413_MFG_ID_EXPECTED) return -3;
    ret = emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &product_id, 1);
    if (ret) return ret;
    if (product_id != EMC1413_PRODUCT_ID_EXPECTED) return -3;
    return 0;
}

int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = emc1413_read_temp_pair(dev, EMC1413_REG_INTERNAL_TEMP, EMC1413_REG_INTERNAL_TEMP_L, &msb, &lsb);
    if (ret) return ret;
    *temp_mcelsius = emc1413_temp_to_mcelsius(msb, lsb);
    return 0;
}

int emc1413_read_external1_temp(struct emc1413_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = emc1413_read_temp_pair(dev, EMC1413_REG_EXT1_TEMP_H, EMC1413_REG_EXT1_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = emc1413_temp_to_mcelsius(msb, lsb);
    return 0;
}

int emc1413_read_external2_temp(struct emc1413_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = emc1413_read_temp_pair(dev, EMC1413_REG_EXT2_TEMP_H, EMC1413_REG_EXT2_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = emc1413_temp_to_mcelsius(msb, lsb);
    return 0;
}

int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel, int32_t *temp_mcelsius) {
    switch (channel) {
    case EMC1413_CH_INTERNAL:   return emc1413_read_internal_temp(dev, temp_mcelsius);
    case EMC1413_CH_EXTERNAL_1: return emc1413_read_external1_temp(dev, temp_mcelsius);
    case EMC1413_CH_EXTERNAL_2: return emc1413_read_external2_temp(dev, temp_mcelsius);
    default: return -1;
    }
}
