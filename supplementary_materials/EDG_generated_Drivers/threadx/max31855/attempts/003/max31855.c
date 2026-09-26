#include "max31855.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#include "threadx.h"
#define MAX31855_CS_LOW()   // CS is managed by board context, not driver
#define MAX31855_CS_HIGH()

static int32_t decode_thermocouple(uint16_t raw_14) {
    int16_t signed_val;
    if (raw_14 & 0x2000) {
        signed_val = (int16_t)(raw_14 | 0xC000);
    } else {
        signed_val = (int16_t)raw_14;
    }
    return ((int32_t)signed_val * 250) / 1000;
}

static int32_t decode_internal(uint16_t raw_12) {
    int16_t signed_val;
    if (raw_12 & 0x0800) {
        signed_val = (int16_t)(raw_12 | 0xF000);
    } else {
        signed_val = (int16_t)raw_12;
    }
    return ((int32_t)signed_val * 625) / 10000;
}

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    HAL_Delay(200);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *temp_local_val) {
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)dev->bus_handle;
    
    if (HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, 4, 100) != HAL_OK) {
        return -1;
    }
    
    uint32_t raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];
    
    if (raw & 0x00010000) {
        return 1;
    }
    
    uint16_t raw_therm = (uint16_t)((raw >> 18) & 0x3FFF);
    uint16_t raw_int = (uint16_t)((raw >> 4) & 0x0FFF);
    
    *thermocouple_val = decode_thermocouple(raw_therm);
    *temp_local_val = decode_internal(raw_int);
    
    return 0;
}
