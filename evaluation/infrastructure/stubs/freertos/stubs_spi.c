/* Functional FreeRTOS + STM32 HAL SPI stubs for the evaluation harness.
 *
 * HAL_SPI_Transmit/Receive/TransmitReceive route through hw_spi.h so
 * the driver really drives STM32 SPI1 registers in Renode.
 *
 * Pattern: mirrors rtthread/stubs_spi.c but for STM32 HAL SPI API.
 */
#include "freertos.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- SPI bus stubs (real STM32 SPI1 via hw_spi.h) ---- */

HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi,
                                          uint8_t *pTxData, uint8_t *pRxData,
                                          uint16_t Size, uint32_t Timeout) {
    (void)hspi; (void)Timeout;
    hw_spi1_cs_lo();
    for (uint16_t i = 0; i < Size; i++) {
        pRxData[i] = hw_spi1_xfer_byte(pTxData[i]);
    }
    hw_spi1_cs_hi();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout) {
    (void)hspi; (void)Timeout;
    hw_spi1_cs_lo();
    for (uint16_t i = 0; i < Size; i++) {
        (void)hw_spi1_xfer_byte(pData[i]);
    }
    hw_spi1_cs_hi();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData,
                                  uint16_t Size, uint32_t Timeout) {
    (void)hspi; (void)Timeout;
    hw_spi1_cs_lo();
    for (uint16_t i = 0; i < Size; i++) {
        pData[i] = hw_spi1_xfer_byte(0x00);
    }
    hw_spi1_cs_hi();
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi) {
    (void)hspi;
    hw_spi1_init();
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *hspi) { (void)hspi; return HAL_OK; }

/* ---- I2C dummy stubs (SPI build doesn't use I2C) ---- */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t a, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)p;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h, uint16_t a, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)s;(void)t; if(p&&s>0) memset(p,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t a, uint16_t m, uint16_t ms, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)m;(void)ms;(void)p;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t a, uint16_t m, uint16_t ms, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)m;(void)ms;(void)s;(void)t; if(p&&s>0) memset(p,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t a, uint32_t tr, uint32_t t) { (void)h;(void)a;(void)tr;(void)t; return HAL_OK; }

/* ---- GPIO stubs (no-op for SPI builds) ---- */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) { (void)GPIOx; (void)GPIO_Init; }
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    (void)GPIOx; (void)GPIO_Pin; (void)PinState;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx; (void)GPIO_Pin; return GPIO_PIN_RESET;
}
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; }

/* ---- FreeRTOS kernel stubs ---- */
BaseType_t xTaskCreate(void (*f)(void*), const char *n, uint16_t ss,
                       void *p, UBaseType_t pr, TaskHandle_t *ph) {
    (void)f;(void)n;(void)ss;(void)p;(void)pr;(void)ph; return pdPASS;
}
void vTaskDelay(TickType_t t) { (void)t; }
void vTaskDelete(TaskHandle_t h) { (void)h; }
TickType_t xTaskGetTickCount(void) { return 0; }
void taskENTER_CRITICAL(void) {}
void taskEXIT_CRITICAL(void) {}
void vTaskSuspend(TaskHandle_t h) { (void)h; }
void vTaskResume(TaskHandle_t h) { (void)h; }

SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void*)1; }
SemaphoreHandle_t xSemaphoreCreateBinary(void) { return (void*)1; }
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t m, UBaseType_t i) { (void)m;(void)i; return (void*)1; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t) { (void)s;(void)t; return pdPASS; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return pdPASS; }
void vSemaphoreDelete(SemaphoreHandle_t s) { (void)s; }

QueueHandle_t xQueueCreate(UBaseType_t l, UBaseType_t sz) { (void)l;(void)sz; return (void*)1; }
BaseType_t xQueueSend(QueueHandle_t q, const void *p, TickType_t t) { (void)q;(void)p;(void)t; return pdPASS; }
BaseType_t xQueueReceive(QueueHandle_t q, void *p, TickType_t t) { (void)q;(void)p;(void)t; return pdPASS; }
void vQueueDelete(QueueHandle_t q) { (void)q; }

void *pvPortMalloc(size_t sz) { return malloc(sz); }
void vPortFree(void *p) { free(p); }

/* ---- HAL delay ---- */
void HAL_Delay(uint32_t d) { (void)d; }
static uint32_t _hal_tick = 0;
uint32_t HAL_GetTick(void) { return _hal_tick += 10; }
void HAL_IncTick(void) { _hal_tick++; }
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

/* ---- printf via UART2 ---- */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
