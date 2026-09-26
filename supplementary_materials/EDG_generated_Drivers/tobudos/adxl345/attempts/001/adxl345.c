#include "adxl345.h"
#include <stddef.h>

#include "tobudos.h"
#include "stm32f1xx_hal.h"
#define ADXL345_CS_PORT GPIOA
#define ADXL345_CS_PIN GPIO_PIN_4

static void cs_low(struct adxl345_dev *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void cs_high(struct adxl345_dev *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static int spi_write(struct adxl345_dev *dev, uint8_t *data, uint16_t len) {
    uint8_t rx[2];
    cs_low(dev);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->spi, data, rx, len, 100);
    cs_high(dev);
    return (ret == HAL_OK) ? 0 : -1;
}

static int spi_write_then_read(struct adxl345_dev *dev, uint8_t cmd, uint8_t *rx, uint16_t len) {
    uint8_t tx[7];
    tx[0] = cmd;
    for (uint16_t i = 1; i <= len; i++) tx[i] = 0;
    cs_low(dev);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->spi, tx, rx, len + 1, 100);
    cs_high(dev);
    if (ret != HAL_OK) return -1;
    for (uint16_t i = 0; i < len; i++) rx[i] = rx[i + 1];
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, void *bus_handle) {
    dev->spi = (SPI_HandleTypeDef *)bus_handle;
    dev->cs_port = ADXL345_CS_PORT;
    dev->cs_pin = ADXL345_CS_PIN;
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    uint8_t cmd[2];
    cmd[0] = 0x2D;
    cmd[1] = 0x00;
    if (spi_write(dev, cmd, 2) != 0) return -1;

    cmd[0] = 0x2D;
    cmd[1] = 0x08;
    if (spi_write(dev, cmd, 2) != 0) return -1;

    HAL_Delay(12);
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t rx[6];
    if (spi_write_then_read(dev, 0xF2, rx, 6) != 0) return -1;
    int16_t x = (int16_t)(rx[0] | (rx[1] << 8));
    int16_t y = (int16_t)(rx[2] | (rx[3] << 8));
    int16_t z = (int16_t)(rx[4] | (rx[5] << 8));
    *ax = x;
    *ay = y;
    *az = z;
    return 0;
}
