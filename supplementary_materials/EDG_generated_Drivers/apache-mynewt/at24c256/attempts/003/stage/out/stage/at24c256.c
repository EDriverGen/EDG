#include "at24c256.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "apache_mynewt.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

int at24c256_init(struct at24c256_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_write_raw(uint8_t i2c_num, uint8_t addr, const uint8_t *buf, size_t len, uint32_t timeout) {
    struct hal_i2c_master_data pdata = {
        .address = addr,
        .len = len,
        .buffer = (uint8_t *)buf
    };
    return hal_i2c_master_write(i2c_num, &pdata, timeout, 1);
}

static int i2c_read_raw(uint8_t i2c_num, uint8_t addr, uint8_t *buf, size_t len, uint32_t timeout) {
    struct hal_i2c_master_data pdata = {
        .address = addr,
        .len = len,
        .buffer = buf
    };
    return hal_i2c_master_read(i2c_num, &pdata, timeout, 1);
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, size_t len) {
    uint8_t i2c_num = dev->i2c_num;
    uint8_t i2c_addr = dev->i2c_addr;
    size_t offset = 0;
    while (offset < len) {
        size_t page_offset = (addr + offset) % AT24C256_PAGE_SIZE;
        size_t chunk = AT24C256_PAGE_SIZE - page_offset;
        if (chunk > len - offset) chunk = len - offset;
        uint8_t cmd[2];
        uint16_t write_addr = addr + offset;
        cmd[0] = (write_addr >> 8) & 0xFF;
        cmd[1] = write_addr & 0xFF;
        int rc = i2c_write_raw(i2c_num, i2c_addr, cmd, 2, 100);
        if (rc != 0) return rc;
        rc = i2c_write_raw(i2c_num, i2c_addr, buf + offset, chunk, 100);
        if (rc != 0) return rc;
        os_time_delay(AT24C256_WRITE_CYCLE_MS);
        offset += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, size_t len) {
    uint8_t i2c_num = dev->i2c_num;
    uint8_t i2c_addr = dev->i2c_addr;
    uint8_t cmd[2];
    cmd[0] = (addr >> 8) & 0xFF;
    cmd[1] = addr & 0xFF;
    int rc = i2c_write_raw(i2c_num, i2c_addr, cmd, 2, 100);
    if (rc != 0) return rc;
    rc = i2c_read_raw(i2c_num, i2c_addr, buf, len, 100);
    return rc;
}
