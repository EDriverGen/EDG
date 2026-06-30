/* Functional ThreadX + STM32 HAL GPIO stubs for the evaluation harness.
 *
 * HAL_GPIO_ReadPin / WritePin drive STM32F103 GPIO registers (IDR, BSRR)
 * directly so Renode pulse-injector slave can observe/control pin state.
 */
#include "threadx.h"
#include "hw_uart.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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

#define IOPAEN (1U << 2)
#define IOPBEN (1U << 3)
#define IOPCEN (1U << 4)

static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

static uint32_t _port_to_base(GPIO_TypeDef *GPIOx) {
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

/* ---- HAL GPIO stubs (real MMIO) ---- */

/* See freertos/stubs_gpio.c for the rationale.
 * Identical helper keeps ThreadX stub future-proof against drivers that choose
 * OUTPUT_OD / AF variants (currently its adapter picks OUTPUT_PP, but a
 * generated driver may take the idiomatic 1-Wire open-drain path).
 */
static uint32_t _hal_mode_to_crx_nibble(uint32_t Mode, uint32_t Pull) {
    uint32_t dir    = Mode & 0x3u;
    uint32_t is_od  = (Mode >> 4) & 0x1u;
    switch (dir) {
        case 0x1u: return is_od ? 0x5u : 0x1u;
        case 0x2u: return is_od ? 0xDu : 0x9u;
        case 0x3u: return 0x0u;
        default:   return (Pull != 0u) ? 0x8u : 0x4u;
    }
}

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    if (!GPIOx || !GPIO_Init) return;
    _ensure_clocks(GPIOx);
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return;
    uint32_t nibble = _hal_mode_to_crx_nibble(GPIO_Init->Mode, GPIO_Init->Pull);
    for (int pin = 0; pin < 16; pin++) {
        if (!(GPIO_Init->Pin & (1U << pin))) continue;
        int shift = (pin < 8 ? pin : pin - 8) * 4;
        volatile uint32_t *reg = (pin < 8)
            ? (volatile uint32_t*)(base + 0x00)
            : (volatile uint32_t*)(base + 0x04);
        uint32_t v = *reg;
        v &= ~(0xFU << shift);
        v |= (nibble << shift);
        *reg = v;
    }
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    uint32_t base = _port_to_base(GPIOx);
    if (base == 0) return;
    if (PinState != GPIO_PIN_RESET)
        GPIO_BSRR(base) = GPIO_Pin;
    else
        GPIO_BSRR(base) = (uint32_t)GPIO_Pin << 16;
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
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) { return HAL_UART_Transmit(h, p, s, 1000); }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) { return HAL_UART_Receive(h, p, s, 1000); }

/* ---- ThreadX kernel stubs ---- */
UINT tx_thread_create(TX_THREAD *tp, CHAR *n, VOID (*ef)(ULONG), ULONG ei,
                      VOID *ss, ULONG sz, UINT pri, UINT pt, ULONG ts, UINT as) {
    (void)tp;(void)n;(void)ef;(void)ei;(void)ss;(void)sz;(void)pri;(void)pt;(void)ts;(void)as; return TX_SUCCESS;
}
UINT tx_thread_delete(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_thread_terminate(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
void tx_thread_sleep(ULONG t) { (void)t; }
UINT tx_thread_resume(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_thread_suspend(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }

ULONG tx_time_get(void) { return 0; }
void tx_time_set(ULONG t) { (void)t; }

UINT tx_semaphore_create(TX_SEMAPHORE *sp, CHAR *n, ULONG c) { (void)sp;(void)n;(void)c; return TX_SUCCESS; }
UINT tx_semaphore_delete(TX_SEMAPHORE *sp) { (void)sp; return TX_SUCCESS; }
UINT tx_semaphore_get(TX_SEMAPHORE *sp, ULONG w) { (void)sp;(void)w; return TX_SUCCESS; }
UINT tx_semaphore_put(TX_SEMAPHORE *sp) { (void)sp; return TX_SUCCESS; }

UINT tx_mutex_create(TX_MUTEX *mp, CHAR *n, UINT i) { (void)mp;(void)n;(void)i; return TX_SUCCESS; }
UINT tx_mutex_delete(TX_MUTEX *mp) { (void)mp; return TX_SUCCESS; }
UINT tx_mutex_get(TX_MUTEX *mp, ULONG w) { (void)mp;(void)w; return TX_SUCCESS; }
UINT tx_mutex_put(TX_MUTEX *mp) { (void)mp; return TX_SUCCESS; }

UINT tx_timer_create(TX_TIMER *tp, CHAR *n, VOID (*ef)(ULONG), ULONG ei,
                     ULONG it, ULONG rt, UINT aa) {
    (void)tp;(void)n;(void)ef;(void)ei;(void)it;(void)rt;(void)aa; return TX_SUCCESS;
}
UINT tx_timer_delete(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_timer_activate(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_timer_deactivate(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }

UINT tx_byte_pool_create(TX_BYTE_POOL *pp, CHAR *n, VOID *ps, ULONG sz) { (void)pp;(void)n;(void)ps;(void)sz; return TX_SUCCESS; }
UINT tx_byte_allocate(TX_BYTE_POOL *pp, VOID **mp, ULONG sz, ULONG w) { (void)pp;(void)mp;(void)sz;(void)w; return TX_SUCCESS; }
UINT tx_byte_release(VOID *mp) { (void)mp; return TX_SUCCESS; }
UINT tx_kernel_enter(void) { return TX_SUCCESS; }

/* ---- HAL delay ---- */
void HAL_Delay(uint32_t d) { (void)d; }
static uint32_t _hal_tick = 0;
uint32_t HAL_GetTick(void) { return _hal_tick += 10; }
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

/* ---- printf via UART2 ---- */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
