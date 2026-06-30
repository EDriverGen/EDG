/* Functional FreeRTOS + STM32 HAL GPIO stubs for the evaluation harness.
 *
 * HAL_GPIO_ReadPin / WritePin drive STM32F103 GPIO registers (IDR, BSRR)
 * directly so that the Renode pulse-injector slave (gpio_pulse_injector.py)
 * can observe and control pin state.
 *
 * Pattern: mirrors rtthread/stubs_gpio.c but for STM32 HAL GPIO API.
 */
#include "freertos.h"
#include "hw_uart.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- STM32F103 GPIO registers ---- */
#define RCC_APB2ENR  (*(volatile uint32_t*)0x40021018)
#define GPIOA_BASE   0x40010800
#define GPIOB_BASE   0x40010C00
#define GPIOC_BASE   0x40011000
#define GPIO_CRL(b)  (*(volatile uint32_t*)((b) + 0x00))
#define GPIO_CRH(b)  (*(volatile uint32_t*)((b) + 0x04))
#define GPIO_IDR(b)  (*(volatile uint32_t*)((b) + 0x08))
#define GPIO_ODR(b)  (*(volatile uint32_t*)((b) + 0x0C))
#define GPIO_BSRR(b) (*(volatile uint32_t*)((b) + 0x10))

/* RCC_APB2ENR bits: IOPAEN=2, IOPBEN=3, IOPCEN=4 */
#define IOPAEN (1U << 2)
#define IOPBEN (1U << 3)
#define IOPCEN (1U << 4)

/* Static GPIO "instances" for HAL-style driver code that does e.g. GPIOB->IDR.
 * The real register-level operations below bypass these and go direct to MMIO,
 * but generated code that uses the pointer form (huart->Instance = USART1)
 * often also casts GPIOB to a direct struct. Declaring externally-visible
 * instances keeps the linker happy. */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

/* Map HAL GPIOx pointer → MMIO base */
static uint32_t _port_to_base(GPIO_TypeDef *GPIOx) {
    /* Support both symbolic pointers (GPIOA/B/C) and raw MMIO casts */
    uintptr_t p = (uintptr_t)GPIOx;
    if (GPIOx == GPIOA || p == GPIOA_BASE) return GPIOA_BASE;
    if (GPIOx == GPIOB || p == GPIOB_BASE) return GPIOB_BASE;
    if (GPIOx == GPIOC || p == GPIOC_BASE) return GPIOC_BASE;
    return 0;
}

static void _ensure_clocks(GPIO_TypeDef *GPIOx) {
    uintptr_t p = (uintptr_t)GPIOx;
    if      (GPIOx == GPIOA || p == GPIOA_BASE) RCC_APB2ENR |= IOPAEN;
    else if (GPIOx == GPIOB || p == GPIOB_BASE) RCC_APB2ENR |= IOPBEN;
    else if (GPIOx == GPIOC || p == GPIOC_BASE) RCC_APB2ENR |= IOPCEN;
}

/* ---- HAL GPIO stubs (real MMIO operations) ---- */

/* Decompose STM32 HAL GPIO_MODE_* and map to STM32F103 CRL/CRH nibble.
 *
 * HAL Mode encoding (per stm32f1xx_hal_gpio.h):
 *   bits[1:0] = IO direction : 00=input, 01=output, 10=alternate, 11=analog
 *   bit[4]    = open-drain flag (only meaningful for output / alternate)
 *   bits[20:16] = EXTI trigger (RISING/FALLING/RISING_FALLING) — irrelevant here
 *
 * STM32F103 CRL/CRH nibble (CNF[3:2] MODE[1:0]):
 *   Input floating          CNF=01 MODE=00 -> 0x4
 *   Input pull-up/pull-down CNF=10 MODE=00 -> 0x8  (actual up/down set via ODR)
 *   Output PP 10 MHz        CNF=00 MODE=01 -> 0x1
 *   Output OD 10 MHz        CNF=01 MODE=01 -> 0x5
 *   Alternate PP 10 MHz     CNF=10 MODE=01 -> 0x9
 *   Alternate OD 10 MHz     CNF=11 MODE=01 -> 0xD
 *   Analog                  CNF=00 MODE=00 -> 0x0
 *
 * The Renode GPIO injectors classify a pin as "output" iff CRL/CRH MODE bits
 * are non-zero, so we must emit MODE=01 for ANY output/AF variant. This keeps
 * the stub generalizable across drivers that pick PP, OD or AF — no driver
 * needs to be rewritten to match stub limitations. */
static uint32_t _hal_mode_to_crx_nibble(uint32_t Mode, uint32_t Pull) {
    uint32_t dir    = Mode & 0x3u;
    uint32_t is_od  = (Mode >> 4) & 0x1u;
    switch (dir) {
        case 0x1u: return is_od ? 0x5u : 0x1u; /* output */
        case 0x2u: return is_od ? 0xDu : 0x9u; /* alternate function */
        case 0x3u: return 0x0u;                /* analog */
        default:   return (Pull != 0u) ? 0x8u : 0x4u; /* input */
    }
}

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    if (!GPIOx || !GPIO_Init) return;
    _ensure_clocks(GPIOx);
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return;
    uint32_t nibble = _hal_mode_to_crx_nibble(GPIO_Init->Mode, GPIO_Init->Pull);
    /* Configure each pin selected in the mask */
    for (int pin = 0; pin < 16; pin++) {
        if (!(GPIO_Init->Pin & (1U << pin))) continue;
        int shift = (pin < 8 ? pin : pin - 8) * 4;
        volatile uint32_t *reg = (pin < 8)
            ? (volatile uint32_t*)(base + 0x00)   /* CRL */
            : (volatile uint32_t*)(base + 0x04);  /* CRH */
        uint32_t v = *reg;
        v &= ~(0xFU << shift);
        v |= (nibble << shift);
        *reg = v;
    }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return;
    if (PinState != GPIO_PIN_RESET) {
        GPIO_BSRR(base) = GPIO_Pin;       /* set bits */
    } else {
        GPIO_BSRR(base) = (uint32_t)GPIO_Pin << 16; /* reset bits */
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return GPIO_PIN_RESET;
    return (GPIO_IDR(base) & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return;
    GPIO_ODR(base) ^= GPIO_Pin;
}

/* ---- I2C dummy stubs ---- */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t a, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)p;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h, uint16_t a, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)s;(void)t; if(p&&s>0) memset(p,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t a, uint16_t m, uint16_t ms, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)m;(void)ms;(void)p;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t a, uint16_t m, uint16_t ms, uint8_t *p, uint16_t s, uint32_t t) { (void)h;(void)a;(void)m;(void)ms;(void)s;(void)t; if(p&&s>0) memset(p,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h, uint16_t a, uint32_t tr, uint32_t t) { (void)h;(void)a;(void)tr;(void)t; return HAL_OK; }

/* ---- SPI dummy stubs ---- */
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *h, uint8_t *tx, uint8_t *rx, uint16_t s, uint32_t t) { (void)h;(void)tx;(void)s;(void)t; if(rx&&s>0) memset(rx,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) { (void)h;(void)d;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) { (void)h;(void)s;(void)t; if(d&&s>0) memset(d,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *h) { (void)h; return HAL_OK; }

/* ---- UART dummy stubs ---- */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) { (void)h;(void)d;(void)s;(void)t; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *d, uint16_t s, uint32_t t) { (void)h;(void)s;(void)t; if(d&&s>0) memset(d,0,s); return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *h) { (void)h; return HAL_OK; }

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

/* ---- Device-specific delay_us (GPIO sensor drivers) ---- */
void dht22_delay_us(uint32_t us) { (void)us; }
void ds18b20_delay_us(uint32_t us) { (void)us; }
void hcsr04_delay_us(uint32_t us) { (void)us; }
uint32_t hcsr04_get_us_tick(void) { static uint32_t t = 0; return t += 1; }
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
