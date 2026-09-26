#include "lsm303dlhc_ref.h"

static int lsm303dlhc_read_reg(struct lsm303dlhc_device *dev, uint8_t addr,
                               uint8_t reg, uint8_t *value)
{
    if (dev == 0 || dev->bus == 0 || value == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int lsm303dlhc_write_reg(struct lsm303dlhc_device *dev, uint8_t addr,
                                uint8_t reg, uint8_t value)
{
    if (dev == 0 || dev->bus == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Write(dev->bus, (uint16_t)(addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int lsm303dlhc_read_multi(struct lsm303dlhc_device *dev, uint8_t addr,
                                 uint8_t start_reg, uint8_t *buf, uint16_t len)
{
    uint8_t reg = (uint8_t)(start_reg | 0x80U);
    if (dev == 0 || dev->bus == 0 || buf == 0 || len == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK
        ? 0 : -1;
}

int lsm303dlhc_init(struct lsm303dlhc_device *dev, I2C_HandleTypeDef *bus)
{
    if (dev == 0 || bus == 0) {
        return -1;
    }
    if (HAL_I2C_Init(bus) != HAL_OK) {
        return -1;
    }
    dev->bus = bus;
    return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
    uint8_t ira = 0, irb = 0, irc = 0;
    if (lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRA_REG_M, &ira) != 0 ||
        lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRB_REG_M, &irb) != 0 ||
        lsm303dlhc_read_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_IRC_REG_M, &irc) != 0) {
        return -1;
    }
    return ira == LSM303DLHC_IRA_VALUE &&
           irb == LSM303DLHC_IRB_VALUE &&
           irc == LSM303DLHC_IRC_VALUE ? 0 : -1;
}

int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev)
{
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG1_A,
                             LSM303DLHC_ODR_50HZ | LSM303DLHC_AXES_ENABLE) != 0) {
        return -1;
    }
    return lsm303dlhc_write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG4_A,
                                LSM303DLHC_FS_2G | LSM303DLHC_HR_BIT);
}

int lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *accel)
{
    uint8_t data[6];
    if (accel == 0) {
        return -1;
    }
    if (lsm303dlhc_read_multi(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_OUT_X_L_A,
                              data, sizeof(data)) != 0) {
        return -1;
    }
    accel->x = ((int16_t)(((uint16_t)data[1] << 8) | data[0])) >> 4;
    accel->y = ((int16_t)(((uint16_t)data[3] << 8) | data[2])) >> 4;
    accel->z = ((int16_t)(((uint16_t)data[5] << 8) | data[4])) >> 4;
    return 0;
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev)
{
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRA_REG_M,
                             LSM303DLHC_MAG_ODR_15HZ) != 0) {
        return -1;
    }
    if (lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRB_REG_M,
                             LSM303DLHC_MAG_GAIN_1_3) != 0) {
        return -1;
    }
    return lsm303dlhc_write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_MR_REG_M,
                                LSM303DLHC_MAG_CONTINUOUS);
}

int lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev, struct lsm303dlhc_xyz *mag)
{
    uint8_t data[6];
    if (mag == 0) {
        return -1;
    }
    if (lsm303dlhc_read_multi(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_OUT_X_H_M,
                              data, sizeof(data)) != 0) {
        return -1;
    }
    mag->x = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
    mag->z = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
    mag->y = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
    return 0;
}
