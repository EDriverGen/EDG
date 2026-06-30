#include "cmsis_rtx.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    return hw_i2c_write(0, (uint8_t)(DevAddress >> 1), pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    return hw_i2c_read(0, (uint8_t)(DevAddress >> 1), pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t buf[258];
    uint16_t off = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        buf[off++] = (uint8_t)(MemAddress >> 8);
    }
    buf[off++] = (uint8_t)(MemAddress & 0xFF);
    uint16_t copy = (Size + off > sizeof(buf)) ? (uint16_t)(sizeof(buf) - off) : Size;
    memcpy(buf + off, pData, copy);
    return hw_i2c_write(0, (uint8_t)(DevAddress >> 1), buf, off + copy) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t reg[2];
    uint16_t reg_len = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        reg[reg_len++] = (uint8_t)(MemAddress >> 8);
    }
    reg[reg_len++] = (uint8_t)(MemAddress & 0xFF);
    return hw_i2c_write_read(0, (uint8_t)(DevAddress >> 1), reg, reg_len, pData, Size) == 0
               ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) { (void)hi2c; hw_i2c1_init(); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                        uint32_t Trials, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)Trials; (void)Timeout; return HAL_OK;
}

osStatus_t osDelay(uint32_t ticks) { (void)ticks; return osOK; }
uint32_t osKernelGetTickCount(void) { static uint32_t t; return t += 10; }
uint32_t osKernelGetTickFreq(void) { return 1000; }
osMutexId_t osMutexNew(const void *attr) { (void)attr; return (void *)1; }
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) { (void)mutex_id; (void)timeout; return osOK; }
osStatus_t osMutexRelease(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }
osStatus_t osMutexDelete(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }

static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) { (void)GPIOx; (void)GPIO_Init; }
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) { (void)GPIOx; (void)GPIO_Pin; (void)PinState; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; return GPIO_PIN_RESET; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; }
void HAL_Delay(uint32_t Delay) { (void)Delay; }
uint32_t HAL_GetTick(void) { return osKernelGetTickCount(); }
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

int printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) {
        hw_uart2_putc(buf[i]);
    }
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
