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
    uint8_t *data = NULL;
    size_t total_len = 2 + len;
    data = (uint8_t *)k_malloc(total_len);
    if (data == NULL) {
        return -ENOMEM;
    }
    data[0] = (uint8_t)(addr >> 8);
    data[1] = (uint8_t)(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        data[2 + i] = buf[i];
    }
    int ret = i2c_write(bus, AT24C256_I2C_ADDR, data, total_len);
    k_free(data);
    if (ret != 0) {
        return -EIO;
    }
    k_sleep(K_MSEC(5));
    return 0;
}
