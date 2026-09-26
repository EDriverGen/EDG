#include "sht30.h"
#include "xtimer.h"
#include <errno.h>
#include <stddef.h>

#include "riot.h"
#define SHT30_ADDR 0x44
#define CMD_SOFT_RESET 0x30A2
#define CMD_GENERAL_CALL_RESET 0x0006
#define CMD_SINGLE_SHOT_HIGH 0x2400
#define MEASUREMENT_DELAY_MS 15
#define SOFT_RESET_DELAY_MS 2

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void sht30_init(sht30_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = SHT30_ADDR;
    uint8_t cmd[2];
    cmd[0] = (CMD_SOFT_RESET >> 8) & 0xFF;
    cmd[1] = CMD_SOFT_RESET & 0xFF;
    i2c_write_bytes(bus, dev->addr, cmd, 2, 0);
    xtimer_msleep(SOFT_RESET_DELAY_MS);
    cmd[0] = (CMD_GENERAL_CALL_RESET >> 8) & 0xFF;
    cmd[1] = CMD_GENERAL_CALL_RESET & 0xFF;
    i2c_write_bytes(bus, 0x00, cmd, 2, 0);
    xtimer_msleep(SOFT_RESET_DELAY_MS);
}

int sht30_read_single_shot(sht30_t *dev, int32_t *temp_val, int32_t *hum_val) {
    uint8_t cmd[2];
    cmd[0] = (CMD_SINGLE_SHOT_HIGH >> 8) & 0xFF;
    cmd[1] = CMD_SINGLE_SHOT_HIGH & 0xFF;
    int ret = i2c_write_bytes(dev->bus, dev->addr, cmd, 2, 0);
    if (ret != 0) return -EIO;
    xtimer_msleep(MEASUREMENT_DELAY_MS);
    uint8_t buf[6];
    ret = i2c_read_bytes(dev->bus, dev->addr, buf, 6, 0);
    if (ret != 0) return -EIO;
    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];
    if (crc8(buf, 2) != buf[2]) return -EIO;
    if (crc8(buf + 3, 2) != buf[5]) return -EIO;
    int64_t temp_raw = (int64_t)st * 175000LL / 65535LL - 45000LL;
    *temp_val = (int32_t)temp_raw;
    int64_t hum_raw = (int64_t)srh * 100000LL / 65535LL;
    *hum_val = (int32_t)hum_raw;
    return 0;
}
