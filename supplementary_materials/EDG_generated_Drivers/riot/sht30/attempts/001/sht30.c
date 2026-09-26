#include "sht30.h"
#include "xtimer.h"
#include <errno.h>
#include <stddef.h>

#include "riot.h"
#define SHT30_ADDR 0x44
#define CMD_SOFT_RESET {0x30, 0xA2}
#define CMD_GENERAL_CALL_RESET {0x00, 0x06}
#define CMD_SINGLE_SHOT_HIGH {0x24, 0x00}

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void sht30_init(sht30_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = SHT30_ADDR;
    uint8_t soft_reset_cmd[] = CMD_SOFT_RESET;
    i2c_write_bytes(bus, dev->addr, soft_reset_cmd, 2, 0);
    xtimer_msleep(2);
    uint8_t general_call_cmd[] = CMD_GENERAL_CALL_RESET;
    i2c_write_bytes(bus, 0x00, general_call_cmd, 2, 0);
    xtimer_msleep(2);
}

int sht30_read_single_shot(sht30_t *dev, int32_t *temp_val, int32_t *hum_val) {
    uint8_t cmd[] = CMD_SINGLE_SHOT_HIGH;
    int ret = i2c_write_bytes(dev->bus, dev->addr, cmd, 2, 0);
    if (ret != 0) {
        return -EIO;
    }
    xtimer_msleep(15);
    uint8_t buf[6];
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, 6, 0);
    if (ret != 0) {
        return -EIO;
    }
    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];
    uint8_t crc_temp = crc8(buf, 2);
    uint8_t crc_hum = crc8(buf + 3, 2);
    if (crc_temp != buf[2] || crc_hum != buf[5]) {
        return -EIO;
    }
    *temp_val = ((int32_t)st * 175000) / 65535 - 45000;
    *hum_val = ((int32_t)srh * 100000) / 65535;
    return 0;
}
