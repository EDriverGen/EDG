#include "sht30_ref.h"
#include <stddef.h>

static uint8_t sht30_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    uint16_t i;
    uint8_t bit;

    for (i = 0; i < len; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

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


int sht30_init(struct sht30_device *dev, I2C_HandleTypeDef *bus, uint16_t addr) {
    if (!dev) return -1;
    dev->bus = bus; dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev) {
    /* Send soft reset command 0x30A2 */
    uint8_t cmd[2] = {0x30, 0xA2};
    int ret;
    if (!dev || !dev->bus) return -1;
    ret = freertos_i2c_write(dev->bus, dev->addr, cmd, 2);
    if (ret != 0) return ret;
    vTaskDelay(pdMS_TO_TICKS(2));
    return 0;
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent) {
    uint8_t cmd[2] = {0x24, 0x00}; /* single shot, high repeatability */
    uint8_t data[6];
    int ret;
    if (!dev || !temp_mcelsius || !rh_mpercent) return -1;

    ret = freertos_i2c_write(dev->bus, dev->addr, cmd, 2);
    if (ret) return ret;
    vTaskDelay(pdMS_TO_TICKS(20));

    ret = freertos_i2c_read(dev->bus, dev->addr, data, 6);
    if (ret) return ret;
    if (sht30_crc8(&data[0], 2) != data[2]) return -2;
    if (sht30_crc8(&data[3], 2) != data[5]) return -2;

    uint16_t raw_temp = (uint16_t)((data[0] << 8) | data[1]);
    uint16_t raw_hum  = (uint16_t)((data[3] << 8) | data[4]);

    /* temp = -45 + 175 * raw / 65535, in milli-celsius (use 64-bit to avoid overflow) */
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    /* rh = 100 * raw / 65535, in milli-percent */
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
