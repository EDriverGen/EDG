#include "sht30_ref.h"
#include <stddef.h>

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

static int openharmony_i2c_write(DevHandle bus, uint16_t addr,
                                 const uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = (uint8_t *)data;
    msg.len = len;
    msg.flags = 0;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_read(DevHandle bus, uint16_t addr,
                                uint8_t *data, uint16_t len)
{
    struct I2cMsg msg;

    if (bus == NULL || data == NULL) return -1;
    msg.addr = addr;
    msg.buf = data;
    msg.len = len;
    msg.flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, &msg, 1) == 1) ? 0 : -1;
}

static int openharmony_i2c_write_read(DevHandle bus, uint16_t addr,
                                      const uint8_t *wdata, uint16_t wlen,
                                      uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;
    msg[0].addr = addr;
    msg[0].buf = (uint8_t *)wdata;
    msg[0].len = wlen;
    msg[0].flags = 0;
    msg[1].addr = addr;
    msg[1].buf = rdata;
    msg[1].len = rlen;
    msg[1].flags = I2C_FLAG_READ;
    return (I2cTransfer(bus, msg, 2) == 2) ? 0 : -1;
}

int sht30_init(struct sht30_device *dev, DevHandle bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    /* Send soft reset command 0x30A2 */
    uint8_t cmd[2] = {0x30, 0xA2};
    int ret;
    if (!dev || !dev->bus) return -1;
    ret = openharmony_i2c_write(dev->bus, dev->addr, cmd, 2);
    if (ret != 0) return ret;
    OsalMSleep(2);
    return 0;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00}; /* single shot, high repeatability */
    uint8_t data[6];
    int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;

    ret = openharmony_i2c_write(dev->bus, dev->addr, cmd, 2);
    if (ret) return ret;
    OsalMSleep(20);

    ret = openharmony_i2c_read(dev->bus, dev->addr, data, 6);
    if (ret) return ret;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;

    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);

    /* temp = -45 + 175 * raw / 65535, in milli-celsius (use 64-bit to avoid overflow) */
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    /* rh = 100 * raw / 65535, in milli-percent */
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
