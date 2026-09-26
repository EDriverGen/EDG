#include "bme280.h"
#include <stdint.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define BME280_I2C_ADDR 0x76
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_STATUS 0xF3
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD

static int bme280_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.buffer = &reg;
    pdata.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 1000, 1);
    if (rc != 0) return -1;

    pdata.buffer = buf;
    pdata.len = len;
    rc = hal_i2c_master_read(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 1000, 1);
    if (rc != 0) return -1;

    return 0;
}

static int bme280_write(struct bme280_dev *dev, uint8_t reg, uint8_t value)
{
    struct hal_i2c_master_data pdata;
    uint8_t buf[2] = {reg, value};
    int rc;

    pdata.address = dev->i2c_addr;
    pdata.buffer = buf;
    pdata.len = 2;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TICK_PER_SECOND / 1000, 1);
    if (rc != 0) return -1;

    return 0;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    uint8_t chip_id;
    int rc;

    dev->i2c_num = 0;
    dev->i2c_addr = BME280_I2C_ADDR;

    (void)bus_handle;

    os_time_delay(OS_TICK_PER_SECOND / 500);

    rc = bme280_write_then_read(dev, BME280_REG_ID, &chip_id, 1);
    if (rc != 0) return -1;
    if (chip_id != 0x60) return -1;

    rc = bme280_write(dev, BME280_REG_RESET, 0xB6);
    if (rc != 0) return -1;

    os_time_delay(OS_TICK_PER_SECOND / 500);

    rc = bme280_write(dev, BME280_REG_CONFIG, 0x00);
    if (rc != 0) return -1;

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    int rc;

    rc = bme280_write_then_read(dev, BME280_REG_PRESS_MSB, buf, 8);
    if (rc != 0) return -1;

    *pressure_raw = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    *temp_raw = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    *humidity_raw = ((uint16_t)buf[6] << 8) | buf[7];

    return 0;
}
