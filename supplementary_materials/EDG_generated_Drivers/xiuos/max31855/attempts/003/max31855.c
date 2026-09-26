#include "max31855.h"
#include "transform.h"
#include <stdint.h>
#include <string.h>

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->fd = 0;
    PrivTaskDelay(200);
    return 0;
}

static int32_t convert_thermocouple(uint16_t raw_14) {
    int32_t sign = (raw_14 >> 13) & 1;
    int32_t mag = raw_14 & 0x1FFF;
    int32_t val = sign * (-8192) + (mag ^ (sign * 0x2000));
    return (val * 250) / 1000;
}

static int32_t convert_internal(uint16_t raw_12) {
    int32_t sign = (raw_12 >> 11) & 1;
    int32_t mag = raw_12 & 0x7FF;
    int32_t val = sign * (-2048) + (mag ^ (sign * 0x800));
    return (val * 625) / 10000;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc_val, int32_t *int_val) {
    uint8_t buf[4];
    memset(buf, 0, sizeof(buf));
    int ret = read(dev->fd, buf, 4);
    if (ret != 4) {
        return -1;
    }
    uint32_t raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    if (raw & 0x00010000) {
        return 1;
    }
    uint16_t tc_raw = (raw >> 18) & 0x3FFF;
    uint16_t int_raw = (raw >> 4) & 0xFFF;
    *tc_val = convert_thermocouple(tc_raw);
    *int_val = convert_internal(int_raw);
    return 0;
}