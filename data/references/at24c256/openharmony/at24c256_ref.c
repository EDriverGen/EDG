#include "at24c256_ref.h"


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

int at24c256_init(struct at24c256_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev) {
    uint8_t addr_buf[2] = {0, 0};
    uint8_t data;
    if (!dev || !dev->bus) return -1;
    return openharmony_i2c_write_read(dev->bus, dev->addr, addr_buf, 2, &data, 1);
}

int at24c256_write_byte(struct at24c256_device *dev, uint16_t mem_addr, uint8_t data) {
    uint8_t buf[3] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF), data};
    int ret = openharmony_i2c_write(dev->bus, dev->addr, buf, 3);
    if (ret) return ret;
    OsalMSleep(5); /* write cycle time */
    return 0;
}

int at24c256_read_byte(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data) {
    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    return openharmony_i2c_write_read(dev->bus, dev->addr, addr_buf, 2, data, 1);
}

int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len) {
    uint8_t buf[AT24C256_PAGE_SIZE + 2];
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t page_rem = AT24C256_PAGE_SIZE - ((mem_addr + offset) % AT24C256_PAGE_SIZE);
        uint16_t chunk = (len - offset < page_rem) ? (len - offset) : page_rem;
        buf[0] = (uint8_t)((mem_addr + offset) >> 8);
        buf[1] = (uint8_t)((mem_addr + offset) & 0xFF);
        for (uint16_t i = 0; i < chunk; i++) buf[2+i] = data[offset+i];
        int ret = openharmony_i2c_write(dev->bus, dev->addr, buf, chunk + 2);
        if (ret) return ret;
        OsalMSleep(5);
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    return openharmony_i2c_write_read(dev->bus, dev->addr, addr_buf, 2, data, len);
}
