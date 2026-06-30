/*
 * AT24C256 I2C EEPROM Driver
 * 32KB, 64-byte page write, 5ms write cycle time.
 * I2C: write [addr_H, addr_L, data...], read: set addr then read bytes.
 */
#include "at24c256_ref.h"


static int at24c256_threadx_i2c_write(struct at24c256_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int at24c256_threadx_i2c_read(struct at24c256_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int at24c256_threadx_i2c_write_read(struct at24c256_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define AT24C256_I2C_WRITE(_bus, _addr, _data, _len) \
    at24c256_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define AT24C256_I2C_READ(_bus, _addr, _data, _len) \
    at24c256_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define AT24C256_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    at24c256_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

int at24c256_init(struct at24c256_device *dev, void *bus_context, const struct at24c256_i2c_ops *ops, uint16_t addr) {
    if (!dev) return -1;
    dev->bus_context = bus_context;
    dev->ops = ops; dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev) {
    uint8_t addr_buf[2] = {0, 0};
    uint8_t data;
    return AT24C256_I2C_WRITE_READ(dev->bus_context, dev->addr, addr_buf, 2, &data, 1);
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
        int ret = AT24C256_I2C_WRITE(dev->bus_context, dev->addr, buf, chunk + 2); if (ret) return ret;
        tx_thread_sleep(1);
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    return AT24C256_I2C_WRITE_READ(dev->bus_context, dev->addr, addr_buf, 2, data, len);
}
