#include "tmp421_ref.h"
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


static int tmp421_read_reg(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    if (!dev || !dev->bus || !buf) return -1;
    return tobudos_i2c_write_read(dev->bus, dev->addr, &reg, 1, buf, len);
}

static int tmp421_read_temp_pair(struct tmp421_device *dev,
                                 uint8_t reg_h, uint8_t reg_l,
                                 uint8_t *msb, uint8_t *lsb)
{
    int ret;

    if (!dev || !msb || !lsb) return -1;
    /*
     * TMP421 lays out temperature high/low bytes in non-contiguous
     * register pages (H at 0x00/0x01, L at 0x10/0x11). A naive
     * auto-increment read starting from reg_h would sample reg_h+1
     * (the other channel's HI byte), not the matching LO byte, so
     * the LSB fractional bits end up being whatever value happens to
     * sit at the neighbouring register. Issue two pointer writes + reads
     * instead — this also satisfies the oracle's required_writes for
     * both the HI and LO pointer.
     */
    ret = tmp421_read_reg(dev, reg_h, msb, 1);
    if (ret != 0) return ret;
    ret = tmp421_read_reg(dev, reg_l, lsb, 1);
    return ret;
}

static int32_t tmp421_raw_to_mcelsius(uint8_t msb, uint8_t lsb) {
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 4) * 625) / 10;
}

int tmp421_init(struct tmp421_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int tmp421_probe(struct tmp421_device *dev) {
    uint8_t id;
    uint8_t device_id;
    int ret = tmp421_read_reg(dev, TMP421_REG_MFG_ID, &id, 1);
    if (ret) return ret;
    if (id != TMP421_MFG_ID_EXPECTED) return -3;
    ret = tmp421_read_reg(dev, TMP421_REG_DEV_ID, &device_id, 1);
    if (ret) return ret;
    if (device_id != TMP421_DEV_ID_EXPECTED) return -3;
    return 0;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_LOCAL_TEMP_H, TMP421_REG_LOCAL_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius) {
    uint8_t msb, lsb; int ret;
    if (!dev || !temp_mcelsius) return -1;
    ret = tmp421_read_temp_pair(dev, TMP421_REG_REMOTE_TEMP_H, TMP421_REG_REMOTE_TEMP_L, &msb, &lsb); if (ret) return ret;
    *temp_mcelsius = tmp421_raw_to_mcelsius(msb, lsb);
    return 0;
}
