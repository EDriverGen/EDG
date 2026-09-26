#include "max31855.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "threadx.h"
#define MAX31855_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define MAX31855_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

static int32_t thermocouple_temp_milliC(uint16_t raw_14)
{
    int32_t sign = (raw_14 >> 13) & 1;
    int32_t magnitude = raw_14 & 0x1FFF;
    if (sign) {
        magnitude = magnitude ^ 0x2000;
    }
    int32_t signed_val = sign * (-8192) + magnitude;
    return (signed_val * 250) / 1000;
}

static int32_t internal_temp_milliC(uint16_t raw_12)
{
    int32_t sign = (raw_12 >> 11) & 1;
    int32_t magnitude = raw_12 & 0x7FF;
    if (sign) {
        magnitude = magnitude ^ 0x800;
    }
    int32_t signed_val = sign * (-2048) + magnitude;
    return (signed_val * 625) / 10000;
}

int max31855_init(struct max31855_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    HAL_Delay(200);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *temp_local_val)
{
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;

    MAX31855_CS_LOW();
    HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 100);
    MAX31855_CS_HIGH();

    uint32_t raw32 = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];

    if (raw32 & 0x00010000) {
        return 1;
    }

    uint16_t raw_14 = (uint16_t)((raw32 >> 18) & 0x3FFF);
    uint16_t raw_12 = (uint16_t)((raw32 >> 4) & 0xFFF);

    *thermocouple_val = thermocouple_temp_milliC(raw_14);
    *temp_local_val = internal_temp_milliC(raw_12);

    return 0;
}
