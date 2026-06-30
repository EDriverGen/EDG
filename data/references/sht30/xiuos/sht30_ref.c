/*
 * SHT30 Temperature/Humidity Sensor Driver for XiUOS
 * Uses PrivWrite() / PrivRead() POSIX-like I2C interface.
 */
#include "sht30_ref.h"
#include <stdio.h>

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

int sht30_init(struct sht30_device *dev, const char *i2c_path, uint16_t addr) {
    if (!dev) return -1;
    dev->fd = PrivOpen(i2c_path, O_RDWR);
    if (dev->fd < 0) return -1;
    dev->addr = addr;
    struct PrivIoctlCfg cfg;
    cfg.ioctl_driver_type = I2C_TYPE;
    cfg.args = &addr;
    PrivIoctl(dev->fd, OPE_INT, &cfg);
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    if (dev->fd < 0) return -1;
    return (PrivWrite(dev->fd, cmd, 2) < 0) ? -1 : 0;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6];
    if (!temp_mcelsius || !rh_mpercent) return -1;
    if (PrivWrite(dev->fd, cmd, 2) < 0) return -1;
    PrivTaskDelay(20);
    if (PrivRead(dev->fd, data, 6) < 0) return -1;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
