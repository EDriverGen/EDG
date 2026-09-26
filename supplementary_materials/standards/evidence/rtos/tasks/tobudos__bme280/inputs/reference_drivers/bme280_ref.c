#include "bme280_ref.h"
#include <stddef.h>

static int bme_read_reg(struct bme280_device *dev, uint8_t reg, uint8_t *buf, uint16_t len);
static int bme_write_reg(struct bme280_device *dev, uint8_t reg, uint8_t val);

static int bme280_wait_ready(struct bme280_device *dev, uint32_t timeout_ticks)
{
    uint32_t waited = 0;
    uint8_t status;
    int ret;

    if (dev == NULL || dev->bus == NULL) return -1;
    for (;;) {
        ret = bme_read_reg(dev, 0xF3, &status, 1);
        if (ret != 0) return ret;
        if ((status & 0x09U) == 0U) return 0;
        if (waited >= timeout_ticks) return -2;
        tos_sleep_ms(1);
        waited += (1);
    }
}

static int tobudos_i2c_write(I2C_HandleTypeDef *bus, uint16_t addr,
                              const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Transmit(bus, (uint16_t)(addr << 1), (uint8_t *)data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int tobudos_i2c_read(I2C_HandleTypeDef *bus, uint16_t addr,
                             uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Receive(bus, (uint16_t)(addr << 1), data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int tobudos_i2c_write_read(I2C_HandleTypeDef *bus, uint16_t addr,
                                   const uint8_t *wdata, uint16_t wlen,
                                   uint8_t *rdata, uint16_t rlen)
{
    HAL_StatusTypeDef status;
    uint16_t mem_addr;

    if (bus == NULL || wdata == NULL || rdata == NULL) return -1;

    if (wlen == 1) {
        status = HAL_I2C_Mem_Read(bus, (uint16_t)(addr << 1), wdata[0],
                                  I2C_MEMADD_SIZE_8BIT, rdata, rlen, 100);
        return (status == HAL_OK) ? 0 : -1;
    }

    if (wlen == 2) {
        mem_addr = (uint16_t)(((uint16_t)wdata[0] << 8) | wdata[1]);
        status = HAL_I2C_Mem_Read(bus, (uint16_t)(addr << 1), mem_addr,
                                  I2C_MEMADD_SIZE_16BIT, rdata, rlen, 100);
        return (status == HAL_OK) ? 0 : -1;
    }

    status = HAL_I2C_Master_Transmit(bus, (uint16_t)(addr << 1), (uint8_t *)wdata, wlen, 100);
    if (status != HAL_OK) return -1;
    status = HAL_I2C_Master_Receive(bus, (uint16_t)(addr << 1), rdata, rlen, 100);
    return (status == HAL_OK) ? 0 : -1;
}


static int bme_read_reg(struct bme280_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return tobudos_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int bme_write_reg(struct bme280_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return tobudos_i2c_write(dev->bus, dev->addr, buf, 2);
}

int bme280_init(struct bme280_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr; dev->t_fine = 0;
    return 0;
}

int bme280_probe(struct bme280_device *dev) {
    uint8_t id;
    int ret = bme_read_reg(dev, 0xD0, &id, 1);
    if (ret) return ret;
    return (id == BME280_CHIP_ID) ? 0 : -3;
}

int bme280_read_calibration(struct bme280_device *dev) {
    uint8_t buf[26], hbuf[7];
    int ret;
    if (!dev || !dev->bus) return -1;
    ret = bme_read_reg(dev, 0x88, buf, 26); if (ret) return ret;
    dev->cal.dig_T1 = (uint16_t)(buf[1]<<8|buf[0]);
    dev->cal.dig_T2 = (int16_t)(buf[3]<<8|buf[2]);
    dev->cal.dig_T3 = (int16_t)(buf[5]<<8|buf[4]);
    dev->cal.dig_P1 = (uint16_t)(buf[7]<<8|buf[6]);
    dev->cal.dig_P2 = (int16_t)(buf[9]<<8|buf[8]);
    dev->cal.dig_P3 = (int16_t)(buf[11]<<8|buf[10]);
    dev->cal.dig_P4 = (int16_t)(buf[13]<<8|buf[12]);
    dev->cal.dig_P5 = (int16_t)(buf[15]<<8|buf[14]);
    dev->cal.dig_P6 = (int16_t)(buf[17]<<8|buf[16]);
    dev->cal.dig_P7 = (int16_t)(buf[19]<<8|buf[18]);
    dev->cal.dig_P8 = (int16_t)(buf[21]<<8|buf[20]);
    dev->cal.dig_P9 = (int16_t)(buf[23]<<8|buf[22]);
    ret = bme_read_reg(dev, 0xA1, &dev->cal.dig_H1, 1); if (ret) return ret;
    ret = bme_read_reg(dev, 0xE1, hbuf, 7); if (ret) return ret;
    dev->cal.dig_H2 = (int16_t)(hbuf[1]<<8|hbuf[0]);
    dev->cal.dig_H3 = hbuf[2];
    dev->cal.dig_H4 = (int16_t)((((int16_t)(int8_t)hbuf[3]) << 4) | (hbuf[4] & 0x0F));
    dev->cal.dig_H5 = (int16_t)((((int16_t)(int8_t)hbuf[5]) << 4) | (hbuf[4] >> 4));
    dev->cal.dig_H6 = (int8_t)hbuf[6];
    return 0;
}

int bme280_read(struct bme280_device *dev, int32_t *temp_mc, uint32_t *press_pa, uint32_t *hum_mp) {
    uint8_t data[8]; int ret;
    if (!dev || !temp_mc) return -1;
    /* Force mode: ctrl_hum=0x01, ctrl_meas=0x25 (temp+press oversampling×1, forced) */
    ret = bme_write_reg(dev, 0xF2, 0x01); if (ret) return ret;
    ret = bme_write_reg(dev, 0xF4, 0x25); if (ret) return ret;
    ret = bme280_wait_ready(dev, (20)); if (ret) return ret;
    ret = bme_read_reg(dev, 0xF7, data, 8); if (ret) return ret;

    int32_t adc_T = (int32_t)(((uint32_t)data[3]<<12)|((uint32_t)data[4]<<4)|(data[5]>>4));
    int32_t adc_P = (int32_t)(((uint32_t)data[0]<<12)|((uint32_t)data[1]<<4)|(data[2]>>4));
    int32_t adc_H = (int32_t)((data[6]<<8)|data[7]);

    /* Temperature compensation */
    int32_t var1 = ((((adc_T>>3)-((int32_t)dev->cal.dig_T1<<1)))*((int32_t)dev->cal.dig_T2))>>11;
    int32_t var2 = (((((adc_T>>4)-((int32_t)dev->cal.dig_T1))*((adc_T>>4)-((int32_t)dev->cal.dig_T1)))>>12)*((int32_t)dev->cal.dig_T3))>>14;
    dev->t_fine = var1 + var2;
    int32_t T = (dev->t_fine * 5 + 128) >> 8; /* centidegrees */
    *temp_mc = T * 10; /* milli-celsius */

    /* Pressure compensation */
    if (press_pa) {
        int64_t v1 = (int64_t)dev->t_fine - 128000;
        int64_t v2 = v1*v1*(int64_t)dev->cal.dig_P6;
        v2 = v2 + ((v1*(int64_t)dev->cal.dig_P5)<<17);
        v2 = v2 + (((int64_t)dev->cal.dig_P4)<<35);
        v1 = ((v1*v1*(int64_t)dev->cal.dig_P3)>>8)+((v1*(int64_t)dev->cal.dig_P2)<<12);
        v1 = (((((int64_t)1)<<47)+v1))*((int64_t)dev->cal.dig_P1)>>33;
        if (v1 == 0) { *press_pa = 0; }
        else {
            int64_t p = 1048576 - adc_P;
            p = (((p<<31)-v2)*3125)/v1;
            v1 = (((int64_t)dev->cal.dig_P9)*(p>>13)*(p>>13))>>25;
            v2 = (((int64_t)dev->cal.dig_P8)*p)>>19;
            p = ((p+v1+v2)>>8)+(((int64_t)dev->cal.dig_P7)<<4);
            *press_pa = (uint32_t)(p >> 8); /* Pa */
        }
    }
    /* Humidity compensation */
    if (hum_mp) {
        int32_t h = dev->t_fine - 76800;
        int32_t h1 = (((adc_H << 14) - (((int32_t)dev->cal.dig_H4) << 20) -
                        (((int32_t)dev->cal.dig_H5) * h)) + 16384) >> 15;
        int32_t h2 = (h * ((int32_t)dev->cal.dig_H6)) >> 10;
        int32_t h3 = (h * ((int32_t)dev->cal.dig_H3)) >> 11;
        int32_t h4 = ((h2 * (h3 + 32768)) >> 10) + 2097152;
        int32_t h5 = ((h4 * ((int32_t)dev->cal.dig_H2)) + 8192) >> 14;
        h = h1 * h5;
        h = h - (((((h >> 15) * (h >> 15)) >> 7) * ((int32_t)dev->cal.dig_H1)) >> 4);
        h = (h < 0) ? 0 : h;
        h = (h > 419430400) ? 419430400 : h;
        uint32_t rh1024 = (uint32_t)(h >> 12);
        *hum_mp = (rh1024 * 1000) / 1024; /* milli-percent */
    }
    return 0;
}
