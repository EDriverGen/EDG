#include "sht30.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_MEASURE_HIGH_NOCLOCK 0x2400
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006

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

void sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->bus = (I2CDriver *)bus_handle;
    dev->addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    uint8_t cmd = 0x06;
    i2cAcquireBus(dev->bus);
    i2cMasterTransmitTimeout(dev->bus, 0x00, &cmd, 1, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    chThdSleepMilliseconds(2); // wait 1.5 ms

    // Soft reset
    uint8_t soft_reset_cmd[2] = {0x30, 0xA2};
    i2cAcquireBus(dev->bus);
    i2cMasterTransmitTimeout(dev->bus, dev->addr, soft_reset_cmd, 2, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    chThdSleepMilliseconds(2); // wait 1.5 ms
}

void sht30_read_temp_humidity(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent) {
    uint8_t tx_buf[2] = {0x24, 0x00};
    uint8_t rx_buf[6];
    msg_t status;

    i2cAcquireBus(dev->bus);
    // Send measurement command
    status = i2cMasterTransmitTimeout(dev->bus, dev->addr, tx_buf, 2, NULL, 0, TIME_MS2I(100));
    if (status != MSG_OK) {
        i2cReleaseBus(dev->bus);
        *temp_milliC = -1;
        *humidity_milliPercent = -1;
        return;
    }
    i2cReleaseBus(dev->bus);

    // Wait for measurement (max 15 ms)
    chThdSleepMilliseconds(15);

    // Read 6 bytes
    i2cAcquireBus(dev->bus);
    status = i2cMasterReceiveTimeout(dev->bus, dev->addr, rx_buf, 6, TIME_MS2I(100));
    if (status != MSG_OK) {
        i2cReleaseBus(dev->bus);
        *temp_milliC = -1;
        *humidity_milliPercent = -1;
        return;
    }
    i2cReleaseBus(dev->bus);

    // Verify CRC for temperature bytes (bytes 0-1, CRC at byte 2)
    uint8_t temp_crc = crc8(rx_buf, 2);
    if (temp_crc != rx_buf[2]) {
        *temp_milliC = -1;
        *humidity_milliPercent = -1;
        return;
    }
    // Verify CRC for humidity bytes (bytes 3-4, CRC at byte 5)
    uint8_t hum_crc = crc8(rx_buf + 3, 2);
    if (hum_crc != rx_buf[5]) {
        *temp_milliC = -1;
        *humidity_milliPercent = -1;
        return;
    }

    uint16_t st = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
    uint16_t srh = ((uint16_t)rx_buf[3] << 8) | rx_buf[4];

    // Convert using integer approximation
    int32_t temp = ((int32_t)st * 175000) / 65535 - 45000;
    int32_t hum = ((int32_t)srh * 100000) / 65535;

    *temp_milliC = temp;
    *humidity_milliPercent = hum;
}
