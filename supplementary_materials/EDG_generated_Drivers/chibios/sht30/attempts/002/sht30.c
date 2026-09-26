#include "sht30.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400
#define SHT30_READ_LEN 6
#define SHT30_TIMEOUT_MS 100

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

static int i2c_write(struct sht30_dev *dev, uint8_t addr, const uint8_t *txbuf, size_t txlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, addr, txbuf, txlen, NULL, 0, MS2ST(SHT30_TIMEOUT_MS));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_read(struct sht30_dev *dev, uint8_t addr, uint8_t *rxbuf, size_t rxlen) {
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterReceiveTimeout(i2cp, addr, rxbuf, rxlen, MS2ST(SHT30_TIMEOUT_MS));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    uint8_t reset_cmd[] = {0x06};
    if (i2c_write(dev, 0x00, reset_cmd, sizeof(reset_cmd)) != 0)
        return -1;
    chThdSleepMilliseconds(2); // wait 1.5 ms, round up

    // Soft reset to device address
    uint8_t soft_reset_cmd[] = {0x30, 0xA2};
    if (i2c_write(dev, dev->i2c_addr, soft_reset_cmd, sizeof(soft_reset_cmd)) != 0)
        return -1;
    chThdSleepMilliseconds(2);

    return 0;
}

int sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    uint8_t cmd[] = {0x24, 0x00};
    if (i2c_write(dev, dev->i2c_addr, cmd, sizeof(cmd)) != 0)
        return -1;

    chThdSleepMilliseconds(15); // wait for measurement

    uint8_t buf[SHT30_READ_LEN];
    if (i2c_read(dev, dev->i2c_addr, buf, SHT30_READ_LEN) != 0)
        return -1;

    // Verify CRC for temperature bytes (buf[0], buf[1]) with CRC byte buf[2]
    if (crc8(buf, 2) != buf[2])
        return -1;
    // Verify CRC for humidity bytes (buf[3], buf[4]) with CRC byte buf[5]
    if (crc8(buf + 3, 2) != buf[5])
        return -1;

    uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_hum = ((uint16_t)buf[3] << 8) | buf[4];

    // Convert using integer approximation with 64-bit intermediate
    int64_t temp64 = (int64_t)raw_temp * 175000 / 65535 - 45000;
    *temp_milliC = (int32_t)temp64;

    int64_t hum64 = (int64_t)raw_hum * 100000 / 65535;
    *humidity_milliPercent = (int32_t)hum64;

    return 0;
}
