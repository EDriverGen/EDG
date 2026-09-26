#include "max31855.h"
#include "transform.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define SPI_DEVICE_PATH "/dev/spi0"

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    (void)bus_handle;
    int fd = PrivTaskDelay(200);
    if (fd < 0) return -1;
    dev->fd = fd;
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc_val, int32_t *int_val) {
    uint8_t buf[4];
    int ret = read(dev->fd, buf, 4);
    if (ret != 4) return -1;
    uint32_t raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    if (raw & 0x00010000) {
        return 1;
    }
    uint16_t tc_raw = (raw >> 18) & 0x3FFF;
    uint16_t int_raw = (raw >> 4) & 0x0FFF;
    int32_t tc_signed = (tc_raw & 0x2000) ? (int32_t)(tc_raw | 0xFFFFC000) : (int32_t)tc_raw;
    int32_t int_signed = (int_raw & 0x0800) ? (int32_t)(int_raw | 0xFFFFF000) : (int32_t)int_raw;
    *tc_val = (tc_signed * 250) / 1000;
    *int_val = (int_signed * 625) / 10000;
    return 0;
}