/*
 * SHT30 Temperature/Humidity Sensor Driver for RIOT OS
 * Uses i2c_acquire/release, i2c_write_bytes/i2c_read_bytes.
 */
#include "sht30_ref.h"

int sht30_init(struct sht30_device *dev, unsigned int bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    int ret;
    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, cmd, 2, 0);
    i2c_release(dev->bus);
    return ret;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6]; int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;
    i2c_acquire(dev->bus);
    ret = i2c_write_bytes(dev->bus, dev->addr, cmd, 2, 0);
    i2c_release(dev->bus);
    if (ret) return ret;
    ztimer_sleep(ZTIMER_MSEC, 20);
    i2c_acquire(dev->bus);
    ret = i2c_read_bytes(dev->bus, dev->addr, data, 6, 0);
    i2c_release(dev->bus);
    if (ret) return ret;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
