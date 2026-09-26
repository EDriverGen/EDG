#include "max31855.h"
#include "transform.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->fd = *(int *)bus_handle;
    PrivTaskDelay(200);
    return 0;
}

static int32_t sign_extend_14(uint16_t raw) {
    if (raw & 0x2000) {
        return (int32_t)(raw | 0xFFFFC000);
    }
    return (int32_t)raw;
}

static int32_t sign_extend_12(uint16_t raw) {
    if (raw & 0x0800) {
        return (int32_t)(raw | 0xFFFFF000);
    }
    return (int32_t)raw;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc_val, int32_t *int_val) {
    if (!dev || !tc_val || !int_val) return -1;
    uint8_t buf[4];
    memset(buf, 0, sizeof(buf));
    int ret = read(dev->fd, buf, 4);
    if (ret != 4) return -1;
    uint32_t raw32 = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    if (raw32 & 0x00010000) {
        return 1;
    }
    uint16_t tc_raw = (raw32 >> 18) & 0x3FFF;
    uint16_t int_raw = (raw32 >> 4) & 0x0FFF;
    int32_t tc_signed = sign_extend_14(tc_raw);
    int32_t int_signed = sign_extend_12(int_raw);
    *tc_val = (tc_signed * 250) / 1000;
    *int_val = (int_signed * 625) / 10000;
    return 0;
}