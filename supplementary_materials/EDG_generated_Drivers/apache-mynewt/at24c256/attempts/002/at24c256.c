#include "at24c256.h"
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "apache_mynewt.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

int at24c256_init(struct at24c256_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = AT24C256_I2C_ADDR;
    return 0;
}

static int i2c_write_raw(uint8_t i2c_num, uint8_t addr, const uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data pdata;
    pdata.address = addr;
    pdata.len = len;
    pdata.buffer = (uint8_t *)buf;
    return hal_i2c_master_write(i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

static int i2c_read_raw(uint8_t i2c_num, uint8_t addr, uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data pdata;
    pdata.address = addr;
    pdata.len = len;
    pdata.buffer = buf;
    return hal_i2c_master_read(i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, uint16_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint8_t i2c_num = dev->i2c_num;
    uint8_t i2c_addr = dev->i2c_addr;
    uint16_t offset = 0;
    while (offset < len) {
        uint16_t page_offset = addr % AT24C256_PAGE_SIZE;
        uint16_t page_remaining = AT24C256_PAGE_SIZE - page_offset;
        uint16_t chunk = (len - offset) < page_remaining ? (len - offset) : page_remaining;
        uint8_t cmd[2];
        cmd[0] = (addr >> 8) & 0xFF;
        cmd[1] = addr & 0xFF;
        uint16_t total_len = 2 + chunk;
        uint8_t *tx_buf = malloc(total_len);
        if (!tx_buf) return -1;
        memcpy(tx_buf, cmd, 2);
        memcpy(tx_buf + 2, buf + offset, chunk);
        int ret = i2c_write_raw(i2c_num, i2c_addr, tx_buf, total_len);
        free(tx_buf);
        if (ret != 0) return ret;
        os_time_delay(AT24C256_WRITE_CYCLE_MS);
        offset += chunk;
        addr += chunk;
    }
    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, uint16_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint8_t i2c_num = dev->i2c_num;
    uint8_t i2c_addr = dev->i2c_addr;
    uint8_t cmd[2];
    cmd[0] = (addr >> 8) & 0xFF;
    cmd[1] = addr & 0xFF;
    int ret = i2c_write_raw(i2c_num, i2c_addr, cmd, 2);
    if (ret != 0) return ret;
    return i2c_read_raw(i2c_num, i2c_addr, buf, len);
}
