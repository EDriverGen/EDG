#include "mhz19b.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define CMD_READ_CO2 0x86
#define RESPONSE_START_BYTE 0xFF
#define RESPONSE_COMMAND_BYTE 0x86
#define RESPONSE_LENGTH 9

static uint8_t calculate_checksum(const uint8_t *data, uint8_t len) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_dev *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw) {
    if (!dev || !raw) return -1;
    
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9] = {0};
    
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)dev->bus_handle;
    
    // Send command
    HAL_StatusTypeDef status = HAL_UART_Transmit(huart, cmd, 9, 1000);
    if (status != HAL_OK) return -1;
    
    // Receive response
    status = HAL_UART_Receive(huart, resp, 9, 1000);
    if (status != HAL_OK) return -1;
    
    // Validate response
    if (resp[0] != RESPONSE_START_BYTE) return -1;
    if (resp[1] != RESPONSE_COMMAND_BYTE) return -1;
    
    uint8_t calc_checksum = calculate_checksum(resp, 8);
    if (calc_checksum != resp[8]) return -1;
    
    // Decode CO2 value
    *raw = (int32_t)((uint16_t)resp[2] << 8) | resp[3];
    return 0;
}