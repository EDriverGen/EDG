/*
 * SHT30 Temperature/Humidity Sensor Driver for ThreadX
 * Command-based I2C: send [0x24, 0x00] for single-shot high-repeatability.
 * Returns 6 bytes: [temp_H, temp_L, CRC, hum_H, hum_L, CRC]
 */
#include "sht30_ref.h"
#include <stddef.h>

static ULONG sht30_ms_to_ticks(ULONG ms)
{
    ULONG ticks;

    if (ms == 0U) {
        return 0U;
    }

    ticks = (ms * TX_TIMER_TICKS_PER_SECOND + 999U) / 1000U;
    return (ticks == 0U) ? 1U : ticks;
}

static uint8_t sht30_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    uint16_t i;
    uint8_t bit;

    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static int sht30_threadx_i2c_write(struct sht30_device *dev, uint16_t addr,
                                   const uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write == NULL) return -1;
    return dev->ops->write(dev->bus_context, addr, data, len);
}

static int sht30_threadx_i2c_read(struct sht30_device *dev, uint16_t addr,
                                  uint8_t *data, uint16_t len)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->read == NULL) return -1;
    return dev->ops->read(dev->bus_context, addr, data, len);
}

static int sht30_threadx_i2c_write_read(struct sht30_device *dev, uint16_t addr,
                                        const uint8_t *wdata, uint16_t wlen,
                                        uint8_t *rdata, uint16_t rlen)
{
    if (dev == NULL || dev->bus_context == NULL || dev->ops == NULL || dev->ops->write_read == NULL) return -1;
    return dev->ops->write_read(dev->bus_context, addr, wdata, wlen, rdata, rlen);
}

#define SHT30_I2C_WRITE(_bus, _addr, _data, _len) \
    sht30_threadx_i2c_write(dev, (_addr), (_data), (_len))
#define SHT30_I2C_READ(_bus, _addr, _data, _len) \
    sht30_threadx_i2c_read(dev, (_addr), (_data), (_len))
#define SHT30_I2C_WRITE_READ(_bus, _addr, _wdata, _wlen, _rdata, _rlen) \
    sht30_threadx_i2c_write_read(dev, (_addr), (_wdata), (_wlen), (_rdata), (_rlen))

int sht30_init(struct sht30_device *dev, void *bus_context, const struct sht30_i2c_ops *ops, uint16_t addr) {
    if (!dev) return -1;
    dev->bus_context = bus_context;
    dev->ops = ops; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    uint8_t cmd[2] = {0x30, 0xA2};
    int ret;
    if (!dev || !dev->bus_context) return -1;
    ret = SHT30_I2C_WRITE(dev->bus_context, dev->addr, cmd, 2);
    if (ret != 0) return ret;
    tx_thread_sleep(sht30_ms_to_ticks(2));
    return 0;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00};
    uint8_t data[6]; int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;
    ret = SHT30_I2C_WRITE(dev->bus_context, dev->addr, cmd, 2);
    if (ret) return ret;
    tx_thread_sleep(sht30_ms_to_ticks(20));
    ret = SHT30_I2C_READ(dev->bus_context, dev->addr, data, 6);
    if (ret) return ret;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;
    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
