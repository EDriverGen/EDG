/*
 * AT24C256 I2C EEPROM Driver
 * 32KB, 64-byte page write, 5ms write cycle time.
 * I2C: write [addr_H, addr_L, data...], read: set addr then read bytes.
 */
#include "at24c256_ref.h"




int at24c256_init(struct at24c256_device *dev, struct rt_i2c_bus_device * bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev) {
    uint8_t addr_buf[2] = {0, 0};
    uint8_t data;
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->addr; msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = addr_buf; msgs[0].len = 2;
    msgs[1].addr = dev->addr; msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = &data; msgs[1].len = 1;
    return (rt_i2c_transfer(dev->bus, msgs, 2) == 2) ? 0 : -1;
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
        struct rt_i2c_msg msg;
        msg.addr = dev->addr; msg.flags = RT_I2C_WR;
        msg.buf = buf; msg.len = chunk + 2;
        if (rt_i2c_transfer(dev->bus, &msg, 1) != 1) return -1;
        rt_thread_mdelay(5);
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len) {
    uint8_t addr_buf[2] = {(uint8_t)(mem_addr >> 8), (uint8_t)(mem_addr & 0xFF)};
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = dev->addr; msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = addr_buf; msgs[0].len = 2;
    msgs[1].addr = dev->addr; msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = data; msgs[1].len = len;
    return (rt_i2c_transfer(dev->bus, msgs, 2) == 2) ? 0 : -1;
}
