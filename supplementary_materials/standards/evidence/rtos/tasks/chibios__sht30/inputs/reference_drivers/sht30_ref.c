/*
 * SHT30 Temperature/Humidity Sensor Driver for ChibiOS
 * Uses i2cMasterTransmitTimeout / i2cMasterReceiveTimeout.
 */
#include "sht30_ref.h"

int sht30_init(struct sht30_device *dev, I2CDriver *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    msg_t ret;
    if (!dev || !dev->bus) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr, cmd, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6]; msg_t ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr, cmd, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    if (ret != MSG_OK) return -1;
    chThdSleepMilliseconds(20);
    i2cAcquireBus(dev->bus);
    ret = i2cMasterReceiveTimeout(dev->bus, dev->addr, data, 6, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    if (ret != MSG_OK) return -1;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
