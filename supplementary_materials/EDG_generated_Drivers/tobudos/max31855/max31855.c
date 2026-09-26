#include "max31855.h"
#include <stdint.h>

#include "tobudos.h"
#include "stm32f1xx_hal.h"
#define MAX31855_CS_PORT GPIOA
#define MAX31855_CS_PIN GPIO_PIN_4

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    HAL_Delay(200);
    return 0;
}

static int32_t extract_thermocouple_temp(uint32_t raw) {
    int32_t raw_14 = (int32_t)((raw >> 18) & 0x3FFF);
    if (raw_14 & 0x2000) {
        raw_14 |= ~0x3FFF;
    }
    return (raw_14 * 250) / 1000;
}

static int32_t extract_internal_temp(uint32_t raw) {
    int32_t raw_12 = (int32_t)((raw >> 4) & 0xFFF);
    if (raw_12 & 0x800) {
        raw_12 |= ~0xFFF;
    }
    return (raw_12 * 625) / 10000;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *temp_local_val) {
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;

    HAL_GPIO_WritePin(MAX31855_CS_PORT, MAX31855_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 100);
    HAL_GPIO_WritePin(MAX31855_CS_PORT, MAX31855_CS_PIN, GPIO_PIN_SET);

    uint32_t raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];

    if (raw & 0x10000) {
        return 1;
    }

    *thermocouple_val = extract_thermocouple_temp(raw);
    *temp_local_val = extract_internal_temp(raw);
    return 0;
}
