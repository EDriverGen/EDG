#include "max31855.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#define MAX31855_CS_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define MAX31855_CS_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

int max31855_init(struct max31855_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = bus_handle;
    HAL_Delay(200);
    return 0;
}

static int32_t extract_thermocouple_temp(uint32_t raw32)
{
    int32_t raw_14 = (int32_t)((raw32 >> 18) & 0x3FFF);
    if (raw_14 & 0x2000) {
        raw_14 |= ~0x3FFF;
    }
    return (raw_14 * 250) / 1000;
}

static int32_t extract_internal_temp(uint32_t raw32)
{
    int32_t raw_12 = (int32_t)((raw32 >> 4) & 0xFFF);
    if (raw_12 & 0x800) {
        raw_12 |= ~0xFFF;
    }
    return (raw_12 * 625) / 10000;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc, int32_t *local)
{
    if (dev == NULL || tc == NULL || local == NULL) return -1;
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    
    MAX31855_CS_LOW();
    HAL_Delay(1);
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 100);
    MAX31855_CS_HIGH();
    
    if (ret != HAL_OK) return -1;
    
    uint32_t raw32 = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) |
                     ((uint32_t)rx_buf[2] << 8) | rx_buf[3];
    
    if (raw32 & 0x00010000) {
        return 1;
    }
    
    *tc = extract_thermocouple_temp(raw32);
    *local = extract_internal_temp(raw32);
    return 0;
}
