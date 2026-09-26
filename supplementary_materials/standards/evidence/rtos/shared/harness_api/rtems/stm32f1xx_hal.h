#ifndef __STM32F1XX_HAL_H
#define __STM32F1XX_HAL_H
#include <stdint.h>
#include "hw_spi.h"
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef struct { void *Instance; } SPI_HandleTypeDef;
typedef struct { void *Instance; } I2C_HandleTypeDef;
typedef struct { void *Instance; } GPIO_TypeDef;
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_4 4
#define GPIOA ((GPIO_TypeDef *)0x40010800)
static inline HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *h) { (void)h; return HAL_OK; }
static inline HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) {
    (void)h; (void)t;
    for (uint16_t i = 0; i < s; i++) hw_spi1_xfer_byte(d[i]);
    return HAL_OK;
}
static inline HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) {
    (void)h; (void)t;
    for (uint16_t i = 0; i < s; i++) d[i] = hw_spi1_xfer_byte(0x00);
    return HAL_OK;
}
static inline void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, uint8_t s) { (void)p;(void)pin;(void)s; }
static inline void HAL_Delay(uint32_t ms) { (void)ms; }
#endif
