#include "sht30.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_MEASURE_HIGH 0x2400
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006

static int sht30_write_command(struct sht30_dev *dev, uint16_t cmd) {
    struct hal_i2c_master_data data;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    data.address = dev->i2c_addr;
    data.buffer = buf;
    data.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
}

static int sht30_read_data(struct sht30_dev *dev, uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data data;
    data.address = dev->i2c_addr;
    data.buffer = buf;
    data.len = len;
    return hal_i2c_master_read(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
}

int sht30_init(struct sht30_dev *dev, void *bus_handle) {
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    // General call reset (broadcast to address 0x00)
    struct hal_i2c_master_data data;
    uint8_t cmd = 0x06;
    data.address = 0x00;
    data.buffer = &cmd;
    data.len = 1;
    int ret = hal_i2c_master_write(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
    if (ret) return ret;
    os_time_delay(2); // 1.5 ms, round up to 2 ticks

    // Soft reset
    ret = sht30_write_command(dev, SHT30_CMD_SOFT_RESET);
    if (ret) return ret;
    os_time_delay(2);

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_val, int32_t *hum_val) {
    int ret;

    // Send single shot measurement command (high repeatability, no clock stretching)
    ret = sht30_write_command(dev, SHT30_CMD_MEASURE_HIGH);
    if (ret) return ret;

    // Wait for measurement (max 15 ms)
    os_time_delay(15);

    // Read 6 bytes: temperature MSB, LSB, CRC, humidity MSB, LSB, CRC
    uint8_t buf[6];
    ret = sht30_read_data(dev, buf, 6);
    if (ret) return ret;

    // CRC check (simplified: skip CRC validation for now)
    // Combine raw values
    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    // Convert using integer approximations with 64-bit intermediates
    int64_t temp_raw = (int64_t)st * 175000 / 65535 - 45000;
    int64_t hum_raw = (int64_t)srh * 100000 / 65535;

    *temp_val = (int32_t)temp_raw;
    *hum_val = (int32_t)hum_raw;

    return 0;
}
