/*
 * SHT30 Temperature/Humidity Sensor Driver for Zephyr
 * Uses Zephyr i2c_write() / i2c_read() API.
 */
#include "sht30_ref.h"

static uint8_t sht30_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    uint16_t i;
    uint8_t bit;
    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80U) != 0U)
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            else
                crc <<= 1;
        }
    }
    return crc;
}

int sht30_init(struct sht30_device *dev, const struct device *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    if (!dev || !dev->bus) return -1;
    return i2c_write(dev->bus, cmd, 2, dev->addr);
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6]; int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;
    ret = i2c_write(dev->bus, cmd, 2, dev->addr);
    if (ret) return ret;
    k_msleep(20);
    ret = i2c_read(dev->bus, data, 6, dev->addr);
    if (ret) return ret;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
