/*
 * AT24C256 I2C EEPROM Driver
 * 32KB, 64-byte page write, 5ms write cycle time.
 * I2C: write [addr_H, addr_L, data...], read: set addr then read bytes.
 */
#include "at24c256_ref.h"




int at24c256_init(struct at24c256_device *dev, const struct device * bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev) {
    uint8_t addr_buf[2] = {0, 0};
    uint8_t data;
    return i2c_write_read(dev->bus, dev->addr, addr_buf, 2, &data, 1);
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
        int ret = i2c_write(dev->bus, buf, chunk + 2, dev->addr); if (ret) return ret;
        k_msleep(5);
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    return i2c_write_read(dev->bus, dev->addr, addr_buf, 2, data, len);
}
