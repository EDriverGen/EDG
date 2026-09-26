#include "at24c256.h"
#include <assert.h>

#include "apache_mynewt.h"
#define AT24C256_I2C_ADDR 0x50
#define AT24C256_PAGE_SIZE 64
#define AT24C256_WRITE_CYCLE_MS 5

int at24c256_init(struct at24c256_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->addr = AT24C256_I2C_ADDR;
    return 0;
}

static int at24c256_wait_write_cycle(struct at24c256_dev *dev)
{
    os_time_delay(AT24C256_WRITE_CYCLE_MS);
    return 0;
}

int at24c256_write(struct at24c256_dev *dev, uint16_t addr, const uint8_t *buf, uint16_t len)
{
    uint8_t tmp[66];
    uint16_t offset = 0;
    int rc;

    while (len > 0) {
        uint16_t page_offset = addr % AT24C256_PAGE_SIZE;
        uint16_t page_remaining = AT24C256_PAGE_SIZE - page_offset;
        uint16_t chunk = len < page_remaining ? len : page_remaining;

        tmp[0] = (uint8_t)(addr >> 8);
        tmp[1] = (uint8_t)(addr & 0xFF);
        for (uint16_t i = 0; i < chunk; i++) {
            tmp[2 + i] = buf[offset + i];
        }

        struct hal_i2c_master_data pdata = {
            .address = (uint16_t)(dev->addr << 1),
            .len = 2 + chunk,
            .buffer = tmp
        };

        rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
        if (rc != 0) {
            return rc;
        }

        at24c256_wait_write_cycle(dev);

        offset += chunk;
        addr += chunk;
        len -= chunk;
    }

    return 0;
}

int at24c256_read(struct at24c256_dev *dev, uint16_t addr, uint8_t *buf, uint16_t len)
{
    uint8_t addr_bytes[2];
    int rc;

    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);

    struct hal_i2c_master_data write_data = {
        .address = (uint16_t)(dev->addr << 1),
        .len = 2,
        .buffer = addr_bytes
    };

    rc = hal_i2c_master_write(dev->i2c_num, &write_data, OS_TIMEOUT_NEVER, 0);
    if (rc != 0) {
        return rc;
    }

    struct hal_i2c_master_data read_data = {
        .address = (uint16_t)(dev->addr << 1) | 1,
        .len = len,
        .buffer = buf
    };

    rc = hal_i2c_master_read(dev->i2c_num, &read_data, OS_TIMEOUT_NEVER, 1);
    return rc;
}
