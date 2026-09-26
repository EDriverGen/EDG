#include "lsm303dlhc_ref.h"
#include <stddef.h>


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


static int lsm_read_at(struct lsm303dlhc_device *dev, uint16_t addr,
                        uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return tobudos_i2c_write_read(dev->bus, addr, &reg, 1, buf, len);
}

static int lsm_write_at(struct lsm303dlhc_device *dev, uint16_t addr,
                         uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return tobudos_i2c_write(dev->bus, addr, buf, 2);
}

int lsm303dlhc_init(struct lsm303dlhc_device *dev, I2C_HandleTypeDef *bus, uint16_t accel_addr) {
    if (!dev) return -1;
    dev->bus = bus;
    dev->accel_addr = accel_addr;
    dev->mag_addr = LSM303DLHC_ADDR_MAG;
    return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev) {
    uint8_t accel_ctrl;
    uint8_t ira;
    uint8_t irb;
    uint8_t irc;
    int ret;

    if (!dev || !dev->bus) return -1;
    ret = lsm_read_at(dev, dev->accel_addr, LSM303DLHC_REG_CTRL_REG1_A, &accel_ctrl, 1);
    if (ret) return ret;
    ret = lsm_read_at(dev, dev->mag_addr, LSM303DLHC_REG_IRA_REG_M, &ira, 1);
    if (ret) return ret;
    ret = lsm_read_at(dev, dev->mag_addr, LSM303DLHC_REG_IRB_REG_M, &irb, 1);
    if (ret) return ret;
    ret = lsm_read_at(dev, dev->mag_addr, LSM303DLHC_REG_IRC_REG_M, &irc, 1);
    if (ret) return ret;
    if (ira != LSM303DLHC_IRA_VALUE || irb != LSM303DLHC_IRB_VALUE || irc != LSM303DLHC_IRC_VALUE) return -3;
    return 0;
}

int lsm303dlhc_enable_accel(struct lsm303dlhc_device *dev) {
    return lsm_write_at(dev, dev->accel_addr, LSM303DLHC_REG_CTRL_REG1_A, 0x47);
}

int lsm303dlhc_enable_mag(struct lsm303dlhc_device *dev) {
    int ret;
    ret = lsm_write_at(dev, dev->mag_addr, LSM303DLHC_REG_CRA_REG_M, 0x10); if (ret) return ret;
    ret = lsm_write_at(dev, dev->mag_addr, LSM303DLHC_REG_CRB_REG_M, 0x20); if (ret) return ret;
    return lsm_write_at(dev, dev->mag_addr, LSM303DLHC_REG_MR_REG_M, 0x00);
}

int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buf[6]; int ret;
    if (!dev) return -1;
    ret = lsm_read_at(dev, dev->accel_addr, LSM303DLHC_REG_OUT_X_L_A | 0x80, buf, 6);
    if (ret) return ret;
    *x = (int16_t)((buf[1]<<8)|buf[0]);
    *y = (int16_t)((buf[3]<<8)|buf[2]);
    *z = (int16_t)((buf[5]<<8)|buf[4]);
    return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buf[6]; int ret;
    if (!dev) return -1;
    ret = lsm_read_at(dev, dev->mag_addr, LSM303DLHC_REG_OUT_X_H_M, buf, 6);
    if (ret) return ret;
    *x = (int16_t)((buf[0]<<8)|buf[1]);
    *z = (int16_t)((buf[2]<<8)|buf[3]);
    *y = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}

/* RT-Thread API compatibility wrappers */
int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev) {
    return lsm303dlhc_enable_accel(dev);
}

int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel) {
    if (!accel) return -1;
    return lsm303dlhc_read_accel(dev, &accel->x, &accel->y, &accel->z);
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev) {
    return lsm303dlhc_enable_mag(dev);
}

int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag) {
    if (!mag) return -1;
    return lsm303dlhc_read_mag(dev, &mag->x, &mag->y, &mag->z);
}
