/*
 * SHT30 Temperature/Humidity Sensor Driver for NuttX
 * Uses NuttX i2c_write() / i2c_read() with config struct.
 */
#include "sht30_ref.h"
#include <unistd.h>

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

int sht30_init(FAR struct sht30_device *dev, FAR struct i2c_master_s *i2c, uint16_t addr) {
    if (!dev) return -1;
    dev->i2c = i2c;
    dev->config.address = addr;
    dev->config.frequency = 100000;
    dev->config.addrlen = 7;
    return 0;
}

int sht30_probe(FAR struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    if (!dev || !dev->i2c) return -1;
    return i2c_write(dev->i2c, &dev->config, cmd, 2);
}

int sht30_read(FAR struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6]; int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;
    ret = i2c_write(dev->i2c, &dev->config, cmd, 2);
    if (ret) return ret;
    usleep(20000);
    ret = i2c_read(dev->i2c, &dev->config, data, 6);
    if (ret) return ret;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
