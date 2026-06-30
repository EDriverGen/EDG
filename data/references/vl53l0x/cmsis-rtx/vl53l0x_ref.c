#include "vl53l0x_ref.h"

static int vl53l0x_read_reg(struct vl53l0x_device *dev, uint8_t reg,
                            uint8_t *buf, uint16_t len)
{
    if (dev == 0 || dev->bus == 0 || buf == 0 || len == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK
        ? 0 : -1;
}

static int vl53l0x_write_reg(struct vl53l0x_device *dev, uint8_t reg, uint8_t val)
{
    if (dev == 0 || dev->bus == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Write(dev->bus, (uint16_t)(dev->addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 100) == HAL_OK
        ? 0 : -1;
}

int vl53l0x_init(struct vl53l0x_device *dev, I2C_HandleTypeDef *bus, uint16_t addr)
{
    if (dev == 0 || bus == 0 || addr == 0) {
        return -1;
    }
    if (HAL_I2C_Init(bus) != HAL_OK) {
        return -1;
    }
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int vl53l0x_probe(struct vl53l0x_device *dev)
{
    uint8_t id = 0;
    if (vl53l0x_read_reg(dev, 0xC0, &id, 1) != 0) {
        return -1;
    }
    return id == VL53L0X_MODEL_ID ? 0 : -3;
}

int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm)
{
    uint8_t status = 0;
    uint8_t data[12];
    if (dev == 0 || range_mm == 0) {
        return -1;
    }
    if (vl53l0x_write_reg(dev, 0x00, 0x01) != 0) {
        return -1;
    }
    (void)osDelay(50);
    if (vl53l0x_read_reg(dev, 0x13, &status, 1) != 0) {
        return -1;
    }
    (void)status;
    if (vl53l0x_read_reg(dev, 0x14, data, sizeof(data)) != 0) {
        return -1;
    }
    *range_mm = (uint16_t)(((uint16_t)data[10] << 8) | data[11]);
    return vl53l0x_write_reg(dev, 0x0B, 0x01);
}
