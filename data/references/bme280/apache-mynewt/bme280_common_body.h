int bme280_probe(struct bme280_device *dev)
{
    uint8_t id = 0;
    if (bme280_read_reg(dev, 0xD0, &id, 1) != 0) return -1;
    return id == BME280_CHIP_ID ? 0 : -3;
}

int bme280_read_calibration(struct bme280_device *dev)
{
    uint8_t buf[26];
    uint8_t hbuf[7];
    uint8_t h1 = 0;
    if (bme280_read_reg(dev, 0x88, buf, 26) != 0) return -1;
    dev->cal.dig_T1 = (uint16_t)((buf[1] << 8) | buf[0]);
    dev->cal.dig_T2 = (int16_t)((buf[3] << 8) | buf[2]);
    dev->cal.dig_T3 = (int16_t)((buf[5] << 8) | buf[4]);
    dev->cal.dig_P1 = (uint16_t)((buf[7] << 8) | buf[6]);
    dev->cal.dig_P2 = (int16_t)((buf[9] << 8) | buf[8]);
    dev->cal.dig_P3 = (int16_t)((buf[11] << 8) | buf[10]);
    dev->cal.dig_P4 = (int16_t)((buf[13] << 8) | buf[12]);
    dev->cal.dig_P5 = (int16_t)((buf[15] << 8) | buf[14]);
    dev->cal.dig_P6 = (int16_t)((buf[17] << 8) | buf[16]);
    dev->cal.dig_P7 = (int16_t)((buf[19] << 8) | buf[18]);
    dev->cal.dig_P8 = (int16_t)((buf[21] << 8) | buf[20]);
    dev->cal.dig_P9 = (int16_t)((buf[23] << 8) | buf[22]);
    if (bme280_read_reg(dev, 0xA1, &h1, 1) != 0) return -1;
    dev->cal.dig_H1 = h1;
    if (bme280_read_reg(dev, 0xE1, hbuf, 7) != 0) return -1;
    dev->cal.dig_H2 = (int16_t)((hbuf[1] << 8) | hbuf[0]);
    dev->cal.dig_H3 = hbuf[2];
    dev->cal.dig_H4 = (int16_t)(((int16_t)(int8_t)hbuf[3] << 4) | (hbuf[4] & 0x0F));
    dev->cal.dig_H5 = (int16_t)(((int16_t)(int8_t)hbuf[5] << 4) | (hbuf[4] >> 4));
    dev->cal.dig_H6 = (int8_t)hbuf[6];
    return 0;
}

int bme280_read(struct bme280_device *dev, int32_t *temp_mc, uint32_t *press_pa, uint32_t *hum_mp)
{
    uint8_t data[8];
    int32_t adc_T, adc_P, adc_H;
    int32_t var1, var2;
    if (dev == 0 || temp_mc == 0) return -1;
    if (bme280_write_reg(dev, 0xF2, 0x01) != 0) return -1;
    if (bme280_write_reg(dev, 0xF4, 0x25) != 0) return -1;
    bme280_delay_ms(50);
    if (bme280_read_reg(dev, 0xF7, data, 8) != 0) return -1;
    adc_P = (int32_t)(((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (data[2] >> 4));
    adc_T = (int32_t)(((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | (data[5] >> 4));
    adc_H = (int32_t)((data[6] << 8) | data[7]);
    var1 = ((((adc_T >> 3) - ((int32_t)dev->cal.dig_T1 << 1))) * ((int32_t)dev->cal.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->cal.dig_T1))) >> 12) * ((int32_t)dev->cal.dig_T3)) >> 14;
    dev->t_fine = var1 + var2;
    *temp_mc = (((dev->t_fine * 5 + 128) >> 8) * 10);
    if (press_pa != 0) {
        int64_t v1 = (int64_t)dev->t_fine - 128000;
        int64_t v2 = v1 * v1 * (int64_t)dev->cal.dig_P6;
        v2 += (v1 * (int64_t)dev->cal.dig_P5) << 17;
        v2 += ((int64_t)dev->cal.dig_P4) << 35;
        v1 = ((v1 * v1 * (int64_t)dev->cal.dig_P3) >> 8) + ((v1 * (int64_t)dev->cal.dig_P2) << 12);
        v1 = (((((int64_t)1) << 47) + v1) * ((int64_t)dev->cal.dig_P1)) >> 33;
        if (v1 == 0) {
            *press_pa = 0;
        } else {
            int64_t p = 1048576 - adc_P;
            p = (((p << 31) - v2) * 3125) / v1;
            v1 = (((int64_t)dev->cal.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
            v2 = (((int64_t)dev->cal.dig_P8) * p) >> 19;
            p = ((p + v1 + v2) >> 8) + (((int64_t)dev->cal.dig_P7) << 4);
            *press_pa = (uint32_t)(p >> 8);
        }
    }
    if (hum_mp != 0) {
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
        *hum_mp = ((uint32_t)(h >> 12) * 1000U) / 1024U;
    }
    return 0;
}
