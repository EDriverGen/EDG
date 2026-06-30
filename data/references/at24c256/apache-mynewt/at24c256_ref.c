#include "at24c256_ref.h"

int at24c256_init(struct at24c256_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0) return -1;
    if (hal_i2c_init(i2c_num, 0) != 0) return -1;
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int at24c256_probe(struct at24c256_device *dev)
{
    uint8_t data = 0;
    return at24c256_read(dev, 0, &data, 1);
}

int at24c256_write(struct at24c256_device *dev, uint16_t mem_addr, const uint8_t *data, uint16_t len)
{
    uint8_t buf[AT24C256_PAGE_SIZE + 2];
    uint16_t offset = 0;
    struct hal_i2c_master_data xfer;
    if (dev == 0 || data == 0) return -1;
    while (offset < len) {
        uint16_t page_rem = (uint16_t)(AT24C256_PAGE_SIZE - ((mem_addr + offset) % AT24C256_PAGE_SIZE));
        uint16_t chunk = (uint16_t)((len - offset) < page_rem ? (len - offset) : page_rem);
        buf[0] = (uint8_t)((mem_addr + offset) >> 8);
        buf[1] = (uint8_t)((mem_addr + offset) & 0xFFU);
        for (uint16_t i = 0; i < chunk; i++) buf[i + 2] = data[offset + i];
        xfer.address = (uint8_t)dev->addr;
        xfer.len = (uint16_t)(chunk + 2);
        xfer.buffer = buf;
        if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) != 0) return -1;
        os_time_delay(5);
        offset = (uint16_t)(offset + chunk);
    }
    return 0;
}

int at24c256_read(struct at24c256_device *dev, uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    uint8_t addr_buf[2];
    struct hal_i2c_master_data xfer;
    if (dev == 0 || data == 0) return -1;
    addr_buf[0] = (uint8_t)(mem_addr >> 8);
    addr_buf[1] = (uint8_t)(mem_addr & 0xFFU);
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 2;
    xfer.buffer = addr_buf;
    if (hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 0) != 0) return -1;
    xfer.address = (uint8_t)dev->addr;
    xfer.len = len;
    xfer.buffer = data;
    return hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0 ? 0 : -1;
}
