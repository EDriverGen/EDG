#include "cmsis_rtx.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>

HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi,
                                          uint8_t *pTxData,
                                          uint8_t *pRxData,
                                          uint16_t Size,
                                          uint32_t Timeout)
{
    (void)hspi;
    (void)Timeout;
    hw_spi1_cs_lo();
    for (uint16_t i = 0; i < Size; i++) {
        uint8_t tx = pTxData != 0 ? pTxData[i] : 0x00;
        uint8_t rx = hw_spi1_xfer_byte(tx);
        if (pRxData != 0) {
            pRxData[i] = rx;
        }
    }
    hw_spi1_cs_hi();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi,
                                   uint8_t *pData,
                                   uint16_t Size,
                                   uint32_t Timeout)
{
    return HAL_SPI_TransmitReceive(hspi, pData, 0, Size, Timeout);
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi,
                                  uint8_t *pData,
                                  uint16_t Size,
                                  uint32_t Timeout)
{
    return HAL_SPI_TransmitReceive(hspi, 0, pData, Size, Timeout);
}

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi)
{
    (void)hspi;
    hw_spi1_init();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *hspi)
{
    (void)hspi;
    return HAL_OK;
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

int printf(const char *fmt, ...)
{
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
