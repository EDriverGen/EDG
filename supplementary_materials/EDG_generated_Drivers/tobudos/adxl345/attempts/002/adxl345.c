#include "adxl345.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "tobudos.h"
#define ADXL345_CS_PIN GPIO_PIN_4
#define ADXL345_CS_PORT GPIOA

static void cs_select(void) {
    HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_RESET);
}

static void cs_deselect(void) {
    HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_SET);
}

static int spi_write(struct adxl345_dev *dev, uint8_t reg, uint8_t data) {
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx[2] = {reg, data};
    uint8_t rx[2] = {0};
    cs_select();
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx, rx, 2, 100);
    cs_deselect();
    return (ret == HAL_OK) ? 0 : -1;
}

static int spi_read_burst(struct adxl345_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx[1] = {reg};
    uint8_t rx[1] = {0};
    cs_select();
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx, rx, 1, 100);
    if (ret != HAL_OK) {
        cs_deselect();
        return -1;
    }
    ret = HAL_SPI_TransmitReceive(hspi, buf, buf, len, 100);
    cs_deselect();
    return (ret == HAL_OK) ? 0 : -1;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    cs_deselect();
    HAL_Delay(1);
    if (spi_write(dev, 0x2D, 0x00) != 0) return -1;
    if (spi_write(dev, 0x2D, 0x08) != 0) return -1;
    HAL_Delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t buf[6];
    if (spi_read_burst(dev, 0xF2, buf, 6) != 0) return -1;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
