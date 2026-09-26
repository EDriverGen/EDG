#include "ds3231_ref.h"


static int freertos_i2c_write(I2C_HandleTypeDef *bus, uint16_t addr,
                              const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Transmit(bus, (uint16_t)(addr << 1), (uint8_t *)data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int freertos_i2c_read(I2C_HandleTypeDef *bus, uint16_t addr,
                             uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    if (bus == NULL || data == NULL) return -1;
    status = HAL_I2C_Master_Receive(bus, (uint16_t)(addr << 1), data, len, 100);
    return (status == HAL_OK) ? 0 : -1;
}

static int freertos_i2c_write_read(I2C_HandleTypeDef *bus, uint16_t addr,
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


static int ds_read_reg(struct ds3231_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return freertos_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

static uint8_t bcd_to_dec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }

int ds3231_init(struct ds3231_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int ds3231_probe(struct ds3231_device *dev) {
    uint8_t val;
    return ds_read_reg(dev, 0x00, &val, 1);
}

int ds3231_read_time(struct ds3231_device *dev, struct ds3231_time *t) {
    uint8_t buf[7]; int ret;
    if (!dev || !t) return -1;
    ret = ds_read_reg(dev, 0x00, buf, 7);
    if (ret) return ret;
    t->seconds = bcd_to_dec(buf[0] & 0x7F);
    t->minutes = bcd_to_dec(buf[1] & 0x7F);
    t->hours   = bcd_to_dec(buf[2] & 0x3F);
    t->day     = bcd_to_dec(buf[3] & 0x07);
    t->date    = bcd_to_dec(buf[4] & 0x3F);
    t->month   = bcd_to_dec(buf[5] & 0x1F);
    t->year    = bcd_to_dec(buf[6]);
    return 0;
}

int ds3231_read_temperature(struct ds3231_device *dev, int32_t *temp_mcelsius) {
    uint8_t buf[2]; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = ds_read_reg(dev, 0x11, buf, 2);
    if (ret) return ret;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_mcelsius = ((int32_t)(raw >> 6) * 250);
    return 0;
}
