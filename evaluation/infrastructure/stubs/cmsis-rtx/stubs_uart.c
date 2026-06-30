#include "cmsis_rtx.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)
{
    (void)huart;
    hw_uart_bus_init();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart)
{
    (void)huart;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;
    if (pData == 0 && Size != 0) {
        return HAL_ERROR;
    }
    for (uint16_t i = 0; i < Size; i++) {
        hw_uart_bus_write_byte(pData[i]);
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    (void)Timeout;
    if (pData == 0 && Size != 0) {
        return HAL_ERROR;
    }
    for (uint16_t i = 0; i < Size; i++) {
        if (hw_uart_bus_read_byte(&pData[i]) != 0) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

osStatus_t osDelay(uint32_t ticks) { (void)ticks; return osOK; }
uint32_t osKernelGetTickCount(void) { static uint32_t t; return t += 10; }
uint32_t osKernelGetTickFreq(void) { return 1000; }
osMutexId_t osMutexNew(const void *attr) { (void)attr; return (void *)1; }
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) { (void)mutex_id; (void)timeout; return osOK; }
osStatus_t osMutexRelease(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }
osStatus_t osMutexDelete(osMutexId_t mutex_id) { (void)mutex_id; return osOK; }
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
