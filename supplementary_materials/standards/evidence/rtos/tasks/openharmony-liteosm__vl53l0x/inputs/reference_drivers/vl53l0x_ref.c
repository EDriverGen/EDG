#include "vl53l0x_ref.h"


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

static int vl_read_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return openharmony_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int vl_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return openharmony_i2c_write(dev->bus, dev->addr, buf, 2);
}

int vl53l0x_init(struct vl53l0x_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev) {
    uint8_t id;
    int ret = vl_read_reg(dev, 0xC0, &id, 1);
    if (ret) return ret;
    return (id == VL53L0X_MODEL_ID) ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm) {
    uint8_t data[12], status;
    int ret;
    if (!dev || !range_mm) return -1;

    /* Start single-shot measurement */
    ret = vl_write_reg(dev, 0x00, 0x01); if (ret) return ret;
    OsalMSleep(50);

    /* Check measurement complete */
    ret = vl_read_reg(dev, 0x13, &status, 1);
    if (ret) return ret;

    /* Read range result (at offset 10-11 in result block) */
    ret = vl_read_reg(dev, 0x14, data, 12);
    if (ret) return ret;

    *range_mm = (uint16_t)((data[10] << 8) | data[11]);
    /* Clear interrupt */
    ret = vl_write_reg(dev, 0x0B, 0x01); if (ret) return ret;
    return 0;
}
