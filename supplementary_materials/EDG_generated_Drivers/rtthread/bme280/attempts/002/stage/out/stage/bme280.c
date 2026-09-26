#include "bme280.h"
#include <stdint.h>
#include <stddef.h>
#include "rtthread.h"
#include <drivers/dev_i2c.h>

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t value)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, value};
    msgs[0].addr = BME280_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 2;
    msgs[0].buf = buf;
    if (rt_i2c_transfer(dev->bus, msgs, 1) != 1)
        return -1;
    return 0;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    msgs[0].addr = BME280_I2C_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = BME280_I2C_ADDR;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int bme280_read_calibration(struct bme280_dev *dev)
{
    uint8_t buf[26];
    if (bme280_read_regs(dev, 0x88, buf, 26) != 0)
        return -1;
    dev->dig_T1 = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    dev->dig_T2 = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    dev->dig_T3 = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    dev->dig_P1 = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
    dev->dig_P2 = (int16_t)((uint16_t)buf[8] | ((uint16_t)buf[9] << 8));
    dev->dig_P3 = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));
    dev->dig_P4 = (int16_t)((uint16_t)buf[12] | ((uint16_t)buf[13] << 8));
    dev->dig_P5 = (int16_t)((uint16_t)buf[14] | ((uint16_t)buf[15] << 8));
    dev->dig_P6 = (int16_t)((uint16_t)buf[16] | ((uint16_t)buf[17] << 8));
    dev->dig_P7 = (int16_t)((uint16_t)buf[18] | ((uint16_t)buf[19] << 8));
    dev->dig_P8 = (int16_t)((uint16_t)buf[20] | ((uint16_t)buf[21] << 8));
    dev->dig_P9 = (int16_t)((uint16_t)buf[22] | ((uint16_t)buf[23] << 8));
    dev->dig_H1 = buf[25];

    uint8_t buf2[7];
    if (bme280_read_regs(dev, 0xE1, buf2, 7) != 0)
        return -1;
    dev->dig_H2 = (int16_t)((uint16_t)buf2[0] | ((uint16_t)buf2[1] << 8));
    dev->dig_H3 = buf2[2];
    dev->dig_H4 = (int16_t)(((uint16_t)buf2[3] << 4) | ((uint16_t)buf2[4] & 0x0F));
    dev->dig_H5 = (int16_t)(((uint16_t)buf2[4] >> 4) | ((uint16_t)buf2[5] << 4));
    dev->dig_H6 = (int8_t)buf2[6];
    return 0;
}

int bme280_init(struct bme280_dev *dev, struct rt_i2c_bus_device *bus)
{
    uint8_t chip_id;
    dev->bus = bus;

    rt_thread_mdelay(2);

    if (bme280_write_reg(dev, 0xE0, 0xB6) != 0)
        return -1;
    rt_thread_mdelay(2);

    if (bme280_read_regs(dev, 0xD0, &chip_id, 1) != 0)
        return -1;
    if (chip_id != 0x60)
        return -1;

    if (bme280_read_calibration(dev) != 0)
        return -1;

    if (bme280_write_reg(dev, 0xF2, 0x01) != 0)
        return -1;
    if (bme280_write_reg(dev, 0xF4, 0x27) != 0)
        return -1;
    if (bme280_write_reg(dev, 0xF5, 0x00) != 0)
        return -1;

    return 0;
}

int bme280_read_measurements(struct bme280_dev *dev, int32_t *temp, int32_t *pressure, int32_t *humidity)
{
    uint8_t buf[8];
    if (bme280_read_regs(dev, 0xF7, buf, 8) != 0)
        return -1;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);
    int32_t adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];

    // Temperature compensation (integer approximation)
    int32_t var1, var2;
    var1 = (int32_t)(((int64_t)adc_T * 256 - (int64_t)dev->dig_T1 * 16384) / 16384);
    var1 = var1 * (int32_t)dev->dig_T2;
    var2 = (int32_t)(((int64_t)adc_T * 256 - (int64_t)dev->dig_T1 * 16384) / 131072);
    var2 = var2 * var2 * (int32_t)dev->dig_T3 / 8;
    dev->t_fine = (var1 + var2 + 128) / 256;
    *temp = dev->t_fine;

    // Pressure: raw count
    *pressure = adc_P;

    // Humidity: raw count
    *humidity = adc_H;

    return 0;
}