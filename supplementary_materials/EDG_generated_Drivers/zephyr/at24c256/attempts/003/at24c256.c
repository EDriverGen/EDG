#include "at24c256.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <zephyr/sys/byteorder.h>
int at24c256_init(const struct device *dev, const struct device *bus)
{
    (void)dev;
    (void)bus;
    return 0;
}

int at24c256_read(const struct device *dev, uint16_t addr, uint8_t *buf, size_t len)
{
    const struct device *bus = dev;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    int ret = i2c_write_read(bus, AT24C256_I2C_ADDR, addr_bytes, 2, buf, len);
    if (ret != 0) {
        return -EIO;
    }
    return 0;
}

int at24c256_write(const struct device *dev, uint16_t addr, const uint8_t *buf, size_t len)
{
    const struct device *bus = dev;
    uint8_t addr_bytes[2];
    addr_bytes[0] = (uint8_t)(addr >> 8);
    addr_bytes[1] = (uint8_t)(addr & 0xFF);
    uint8_t *write_buf = NULL;
    size_t total_len = 2 + len;
    uint8_t *tmp = NULL;
    if (len > 0) {
        tmp = (uint8_t *)malloc(total_len);
        if (tmp == NULL) {
            return -ENOMEM;
        }
        tmp[0] = addr_bytes[0];
        tmp[1] = addr_bytes[1];
        for (size_t i = 0; i < len; i++) {
            tmp[2 + i] = buf[i];
        }
        write_buf = tmp;
    } else {
        write_buf = addr_bytes;
        total_len = 2;
    }
    int ret = i2c_write(bus, write_buf, total_len, AT24C256_I2C_ADDR);
    if (tmp != NULL) {
        free(tmp);
    }
    if (ret != 0) {
        return -EIO;
    }
    k_msleep(5);
    return 0;
}
