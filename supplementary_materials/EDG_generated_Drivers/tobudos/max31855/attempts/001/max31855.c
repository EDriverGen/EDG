#include "max31855.h"
#include <stddef.h>

#include "tobudos.h"
#include "stm32f1xx_hal.h"
#define CS_PORT GPIOA
#define CS_PIN GPIO_PIN_4

static void cs_low(struct max31855_dev *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void cs_high(struct max31855_dev *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->hspi = (SPI_HandleTypeDef *)bus_handle;
    dev->cs_port = CS_PORT;
    dev->cs_pin = CS_PIN;
    cs_high(dev);
    HAL_Delay(200);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *temp_local_val) {
    if (!dev || !thermocouple_val || !temp_local_val) return -1;
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    cs_low(dev);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->hspi, tx_buf, rx_buf, 4, 100);
    cs_high(dev);
    if (ret != HAL_OK) return -1;
    uint32_t raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];
    if (raw & 0x00010000) return 1;
    uint16_t raw_14 = (uint16_t)((raw >> 18) & 0x3FFF);
    uint16_t raw_12 = (uint16_t)((raw >> 4) & 0x0FFF);
    int32_t sign_14 = (raw_14 >> 13) & 1;
    int32_t sign_12 = (raw_12 >> 11) & 1;
    int32_t thermocouple_temp = (sign_14 * (-8192) + ((int32_t)(raw_14 & 0x1FFF) ^ (sign_14 * 0x2000))) * 250 / 1000;
    int32_t internal_temp = (sign_12 * (-2048) + ((int32_t)(raw_12 & 0x7FF) ^ (sign_12 * 0x800))) * 625 / 10000;
    *thermocouple_val = thermocouple_temp;
    *temp_local_val = internal_temp;
    return 0;
}
