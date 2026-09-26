#include "mpu6050_ref.h"
#include <string.h>


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


static int mpu_read_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return tobudos_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int mpu_write_reg(struct mpu6050_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return tobudos_i2c_write(dev->bus, dev->addr, buf, 2);
}

int mpu6050_init(struct mpu6050_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev || !bus) return -1;
    if (addr != MPU6050_ADDR_LOW && addr != MPU6050_ADDR_HIGH) return -1;
    memset(dev, 0, sizeof(*dev));
    dev->bus = bus;
    dev->addr = addr;
    /* Verify chip identity (WHO_AM_I @0x75 == 0x68) before configuring it.
     * A robust driver MUST refuse to wake-up an unknown / mis-wired device. */
    int ret = mpu6050_probe(dev);
    if (ret) return ret;
    /*
     * Datasheet/Register Map:
     * - Device powers up in sleep mode.
     * - Using a gyro PLL clock source is recommended for better stability.
     * Writing 0x01 to PWR_MGMT_1 clears SLEEP and selects PLL with X gyro.
     */
    return mpu_write_reg(dev, 0x6B, 0x01);
}

int mpu6050_probe(struct mpu6050_device *dev) {
    uint8_t id;
    int ret = mpu_read_reg(dev, 0x75, &id, 1);
    if (ret) return ret;
    return (id == MPU6050_WHO_AM_I_VAL) ? 0 : -3;
}

int mpu6050_read_accel(struct mpu6050_device *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6]; int ret;
    if (!dev || !ax || !ay || !az) return -1;
    ret = mpu_read_reg(dev, 0x3B, buf, 6);
    if (ret) return ret;
    *ax = (int16_t)((buf[0]<<8)|buf[1]);
    *ay = (int16_t)((buf[2]<<8)|buf[3]);
    *az = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}

int mpu6050_read_gyro(struct mpu6050_device *dev, int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[6]; int ret;
    if (!dev || !gx || !gy || !gz) return -1;
    ret = mpu_read_reg(dev, 0x43, buf, 6);
    if (ret) return ret;
    *gx = (int16_t)((buf[0]<<8)|buf[1]);
    *gy = (int16_t)((buf[2]<<8)|buf[3]);
    *gz = (int16_t)((buf[4]<<8)|buf[5]);
    return 0;
}
