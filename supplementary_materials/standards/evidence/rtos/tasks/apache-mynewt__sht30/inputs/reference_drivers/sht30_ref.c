#include "sht30_ref.h"

static uint8_t sht30_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static int sht30_write_command(struct sht30_device *dev, const uint8_t cmd[2])
{
    struct hal_i2c_master_data xfer;
    if (dev == 0 || cmd == 0) {
        return -1;
    }
    xfer.address = (uint8_t)dev->addr;
    xfer.len = 2;
    xfer.buffer = (uint8_t *)cmd;
    return hal_i2c_master_write(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) == 0
        ? 0 : -1;
}

int sht30_init(struct sht30_device *dev, uint8_t i2c_num, uint16_t addr)
{
    if (dev == 0 || (addr != SHT30_ADDR_DEFAULT && addr != SHT30_ADDR_ALT)) {
        return -1;
    }
    if (hal_i2c_init(i2c_num, 0) != 0) {
        return -1;
    }
    dev->i2c_num = i2c_num;
    dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev)
{
    static const uint8_t soft_reset[2] = {0x30, 0xA2};
    return sht30_write_command(dev, soft_reset);
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent)
{
    static const uint8_t measure[2] = {0x24, 0x00};
    struct hal_i2c_master_data xfer;
    uint8_t data[6];
    uint16_t raw_temp;
    uint16_t raw_hum;

    if (dev == 0 || temp_mcelsius == 0 || rh_mpercent == 0) {
        return -1;
    }
    if (sht30_write_command(dev, measure) != 0) {
        return -1;
    }
    os_time_delay(os_time_ms_to_ticks32(20));
    xfer.address = (uint8_t)dev->addr;
    xfer.len = sizeof(data);
    xfer.buffer = data;
    if (hal_i2c_master_read(dev->i2c_num, &xfer, os_time_ms_to_ticks32(100), 1) != 0) {
        return -1;
    }
    if (sht30_crc8(&data[0], 2) != data[2] || sht30_crc8(&data[3], 2) != data[5]) {
        return -2;
    }

    raw_temp = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw_hum = (uint16_t)(((uint16_t)data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
