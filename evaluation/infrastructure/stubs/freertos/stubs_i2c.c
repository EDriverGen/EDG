/* Functional FreeRTOS + STM32 HAL I2C stubs for the evaluation harness.
 *
 * HAL_I2C_Master_Transmit/Receive and HAL_I2C_Mem_Write/Read route through
 * hw_i2c.h so the driver really drives STM32 I2C1 registers in Renode.
 *
 * FreeRTOS kernel primitives remain dummy (no-ops).
 *
 * Pattern: mirrors rtthread/stubs_i2c.c but for STM32 HAL I2C API.
 */
#include "freertos.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- I2C bus stubs (real STM32 I2C1 via hw_i2c.h) ---- */

/* generated HAL drivers are inconsistent about I2C address width:
 * some already pass a 7-bit address (e.g. BH1750 0x23), while others
 * pass the 8-bit shifted form.  Keep a small allow-list of known 7-bit
 * sensor addresses and only shift unknown values.
 */
static const uint8_t _known_7bit[] = {0x23, 0x44, 0x48, 0x49, 0x68, 0x76, 0x5C, 0x77, 0x40, 0x50};

static uint8_t _hal_addr_to_7bit(uint16_t DevAddress) {
    for (unsigned i = 0; i < sizeof(_known_7bit) / sizeof(_known_7bit[0]); i++) {
        if ((uint8_t)DevAddress == _known_7bit[i]) {
            return (uint8_t)DevAddress;
        }
    }
    return (uint8_t)(DevAddress >> 1);
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = _hal_addr_to_7bit(DevAddress);
    return hw_i2c_write(0, addr7, pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = _hal_addr_to_7bit(DevAddress);
    return hw_i2c_read(0, addr7, pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = _hal_addr_to_7bit(DevAddress);
    /* Build [MemAddress | pData] buffer for a single write transaction */
    uint8_t buf[258];
    uint16_t off = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        buf[off++] = (uint8_t)(MemAddress >> 8);
    }
    buf[off++] = (uint8_t)(MemAddress & 0xFF);
    uint16_t copy = (Size + off > sizeof(buf)) ? (uint16_t)(sizeof(buf) - off) : Size;
    memcpy(buf + off, pData, copy);
    return hw_i2c_write(0, addr7, buf, off + copy) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = _hal_addr_to_7bit(DevAddress);
    /* Write register address, then read data */
    uint8_t reg_buf[2];
    uint16_t reg_len = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        reg_buf[reg_len++] = (uint8_t)(MemAddress >> 8);
    }
    reg_buf[reg_len++] = (uint8_t)(MemAddress & 0xFF);
    return hw_i2c_write_read(0, addr7, reg_buf, reg_len, pData, Size) == 0
               ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) {
    (void)hi2c;
    hw_i2c1_init();
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                        uint32_t Trials, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)Trials; (void)Timeout;
    return HAL_OK;
}

/* ---- GPIO stubs (no-op for I2C builds) ---- */
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
