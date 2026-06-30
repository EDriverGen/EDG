#include "vl53l0x_ref.h"


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


static int vl_read_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    return freertos_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}
static int vl_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return freertos_i2c_write(dev->bus, dev->addr, buf, 2);
}

int vl53l0x_init(struct vl53l0x_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev) {
    uint8_t id;
    int ret = vl_read_reg(dev, 0xC0, &id, 1);
    if (ret) return ret;
    return (id == VL53L0X_MODEL_ID) ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm) {
    uint8_t data[12], status;
    int ret;
    if (!dev || !range_mm) return -1;

    /* Start single-shot measurement */
    ret = vl_write_reg(dev, 0x00, 0x01); if (ret) return ret;
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Check measurement complete */
    ret = vl_read_reg(dev, 0x13, &status, 1);
    if (ret) return ret;

    /* Read range result (at offset 10-11 in result block) */
    ret = vl_read_reg(dev, 0x14, data, 12);
    if (ret) return ret;

    *range_mm = (uint16_t)((data[10] << 8) | data[11]);
    /* Clear interrupt */
    ret = vl_write_reg(dev, 0x0B, 0x01); if (ret) return ret;
    return 0;
}
