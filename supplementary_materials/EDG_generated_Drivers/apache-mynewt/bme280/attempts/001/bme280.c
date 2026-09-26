#include "bme280.h"
#include <stdint.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define BME280_I2C_ADDR 0x76
#define BME280_CHIP_ID_REG 0xD0
#define BME280_RESET_REG 0xE0
#define BME280_RESET_CMD 0xB6
#define BME280_CONFIG_REG 0xF5
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_STATUS_REG 0xF3
#define BME280_PRESS_MSB_REG 0xF7
#define BME280_TEMP_MSB_REG 0xFA
#define BME280_HUM_MSB_REG 0xFD
#define BME280_CALIB_REG 0x88
#define BME280_CALIB_HUM_REG 0xE1

static int bme280_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.buffer = &reg;
    pdata.len = 1;
    int rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    if (rc != 0) return -1;
    pdata.buffer = buf;
    pdata.len = len;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
    if (rc != 0) return -1;
    return 0;
}

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.buffer = buf;
    pdata.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICKS_PER_SEC / 10, 1);
}

int bme280_init(struct bme280_dev *dev, void *bus_handle) {
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    // Read chip ID
    uint8_t chip_id = 0;
    int rc = bme280_write_then_read(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (rc != 0 || chip_id != 0x60) return -1;

    // Soft reset
    rc = bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (rc != 0) return -1;
    os_time_delay(OS_TICKS_PER_SEC / 500); // 2ms

    // Write config register (filter off, standby 0.5ms)
    rc = bme280_write_reg(dev, BME280_CONFIG_REG, 0x00);
    if (rc != 0) return -1;

    // Write ctrl_hum (oversampling x1)
    rc = bme280_write_reg(dev, BME280_CTRL_HUM_REG, 0x01);
    if (rc != 0) return -1;

    // Write ctrl_meas (oversampling x1 for temp and pressure, normal mode)
    rc = bme280_write_reg(dev, BME280_CTRL_MEAS_REG, 0x27);
    if (rc != 0) return -1;

    os_time_delay(OS_TICKS_PER_SEC / 100); // 10ms for first measurement
    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw) {
    uint8_t buf[8];
    int rc = bme280_write_then_read(dev, BME280_PRESS_MSB_REG, buf, 8);
    if (rc != 0) return -1;

    // Pressure: 20-bit unsigned, big-endian, right-shift 4
    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    // Temperature: 20-bit unsigned, big-endian, right-shift 4
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    // Humidity: 16-bit unsigned, big-endian
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
