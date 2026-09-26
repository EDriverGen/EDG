#include "bme280.h"
#include <stdint.h>
#include <stddef.h>
#include "rtthread.h"
#include <drivers/dev_i2c.h>

#define BME280_RESET_WORD 0xB6
#define BME280_CHIP_ID 0x60
#define BME280_CHIP_ID_REG 0xD0
#define BME280_RESET_REG 0xE0
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_STATUS_REG 0xF3
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_CONFIG_REG 0xF5
#define BME280_PRESS_MSB_REG 0xF7
#define BME280_CALIB00_REG 0x88
#define BME280_CALIB41_REG 0xE1

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    struct rt_i2c_msg msgs[1];
    uint8_t buf[2] = {reg, data};
    msgs[0].addr = dev->i2c_addr;
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
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 1;
    msgs[0].buf = &reg;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int bme280_read_calibration(struct bme280_dev *dev)
{
    uint8_t calib[26];
    uint8_t calib_hum[7];
    if (bme280_read_regs(dev, BME280_CALIB00_REG, calib, 26) != 0)
        return -1;
    if (bme280_read_regs(dev, BME280_CALIB41_REG, calib_hum, 7) != 0)
        return -1;
    dev->dig_T1 = (uint16_t)(calib[1] << 8) | calib[0];
    dev->dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
    dev->dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);
    dev->dig_P1 = (uint16_t)(calib[7] << 8) | calib[6];
    dev->dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);
    dev->dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);
    dev->dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);
    dev->dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);
    dev->dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);
    dev->dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);
    dev->dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);
    dev->dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);
    dev->dig_H1 = calib[24];
    dev->dig_H2 = (int16_t)((calib_hum[1] << 8) | calib_hum[0]);
    dev->dig_H3 = calib_hum[2];
    dev->dig_H4 = (int16_t)((calib_hum[3] << 4) | (calib_hum[4] & 0x0F));
    dev->dig_H5 = (int16_t)((calib_hum[5] << 4) | ((calib_hum[4] >> 4) & 0x0F));
    dev->dig_H6 = (int8_t)calib_hum[6];
    return 0;
}

int bme280_init(struct bme280_dev *dev, struct rt_i2c_bus_device *bus)
{
    uint8_t chip_id;
    dev->bus = bus;
    dev->i2c_addr = BME280_I2C_ADDR;
    rt_thread_mdelay(2);
    if (bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_WORD) != 0)
        return -1;
    rt_thread_mdelay(2);
    if (bme280_read_regs(dev, BME280_CHIP_ID_REG, &chip_id, 1) != 0)
        return -1;
    if (chip_id != BME280_CHIP_ID)
        return -1;
    if (bme280_read_calibration(dev) != 0)
        return -1;
    if (bme280_write_reg(dev, BME280_CTRL_HUM_REG, 0x01) != 0)
        return -1;
    if (bme280_write_reg(dev, BME280_CTRL_MEAS_REG, 0x27) != 0)
        return -1;
    if (bme280_write_reg(dev, BME280_CONFIG_REG, 0x00) != 0)
        return -1;
    return 0;
}

int bme280_read_measurements(struct bme280_dev *dev, int32_t *temp, int32_t *pressure, int32_t *humidity)
{
    uint8_t buf[8];
    int32_t adc_T, adc_P, adc_H;
    int32_t var1, var2;
    if (bme280_read_regs(dev, BME280_PRESS_MSB_REG, buf, 8) != 0)
        return -1;
    adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);
    adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | ((int32_t)buf[5] >> 4);
    adc_H = ((int32_t)buf[6] << 8) | (int32_t)buf[7];
    var1 = ((((adc_T * 256) - (int32_t)dev->dig_T1 * 16384) / 16384) * (int32_t)dev->dig_T2);
    var2 = ((((adc_T * 256) - (int32_t)dev->dig_T1 * 16384) / 131072) * (((adc_T * 256) - (int32_t)dev->dig_T1 * 16384) / 131072)) * (int32_t)dev->dig_T3 / 8;
    dev->t_fine = (var1 + var2 + 128) / 256;
    *temp = dev->t_fine;
    *pressure = adc_P;
    *humidity = adc_H;
    return 0;
}