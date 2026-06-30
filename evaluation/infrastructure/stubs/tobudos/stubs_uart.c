/* Functional ToBudOS + STM32 HAL UART stubs for the evaluation harness.
 *
 * HAL_UART_* route through hw_uart_bus.h (USART1 registers) so
 * the driver really drives STM32 USART1 in Renode.
 */
#include "tobudos.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ---- UART bus stubs (real STM32 USART1 via hw_uart_bus.h) ---- */

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout) {
    (void)huart; (void)Timeout;
    for (uint16_t i = 0; i < Size; i++) {
        hw_uart_bus_write_byte(pData[i]);
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout) {
    (void)huart; (void)Timeout;
    uint16_t got = 0;
    for (uint16_t i = 0; i < Size; i++) {
        if (hw_uart_bus_read_byte(&pData[i]) != 0) break;
        got++;
    }
    return (got == Size) ? HAL_OK : HAL_TIMEOUT;
}

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart) {
    (void)huart; hw_uart_bus_init(); return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart) { (void)huart; return HAL_OK; }

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

/* ---- GPIO stubs ---- */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

void HAL_GPIO_Init(GPIO_TypeDef *g, GPIO_InitTypeDef *i) { (void)g;(void)i; }
void HAL_GPIO_WritePin(GPIO_TypeDef *g, uint16_t p, GPIO_PinState s) { (void)g;(void)p;(void)s; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *g, uint16_t p) { (void)g;(void)p; return GPIO_PIN_RESET; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *g, uint16_t p) { (void)g;(void)p; }

/* ---- ToBudOS kernel stubs ---- */
k_err_t tos_task_create(k_task_t *t, const char *n, void (*e)(void*), void *a,
                        k_prio_t p, k_stack_t *s, uint32_t sz, k_tick_t ts) {
    (void)t;(void)n;(void)e;(void)a;(void)p;(void)s;(void)sz;(void)ts; return K_ERR_NONE;
}
k_err_t tos_task_destroy(k_task_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_task_delay(k_tick_t d) { (void)d; return K_ERR_NONE; }
void tos_sleep_ms(uint32_t ms) { (void)ms; }
k_tick_t tos_systick_get(void) { return 0; }
void tos_tick2ms(k_tick_t tick, uint32_t *ms) { (void)tick; if(ms) *ms = 0; }

k_err_t tos_mutex_create(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_destroy(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_pend(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_post(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_pend_timed(k_mutex_t *m, k_tick_t t) { (void)m;(void)t; return K_ERR_NONE; }

k_err_t tos_sem_create(k_sem_t *s, uint32_t c) { (void)s;(void)c; return K_ERR_NONE; }
k_err_t tos_sem_destroy(k_sem_t *s) { (void)s; return K_ERR_NONE; }
k_err_t tos_sem_pend(k_sem_t *s, k_tick_t t) { (void)s;(void)t; return K_ERR_NONE; }
k_err_t tos_sem_post(k_sem_t *s) { (void)s; return K_ERR_NONE; }

k_err_t tos_timer_create(k_timer_t *t, k_tick_t d, k_tick_t p,
                         void (*cb)(void*), void *a, uint8_t o) {
    (void)t;(void)d;(void)p;(void)cb;(void)a;(void)o; return K_ERR_NONE;
}
k_err_t tos_timer_destroy(k_timer_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_timer_start(k_timer_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_timer_stop(k_timer_t *t) { (void)t; return K_ERR_NONE; }

void *tos_mmheap_alloc(size_t sz) { return malloc(sz); }
void *tos_mmheap_calloc(size_t n, size_t sz) { return calloc(n, sz); }
void tos_mmheap_free(void *p) { free(p); }

k_err_t tos_knl_init(void) { return K_ERR_NONE; }
k_err_t tos_knl_start(void) { return K_ERR_NONE; }

int tos_hal_uart_init(hal_uart_t *u, int p) { (void)u;(void)p; return 0; }
int tos_hal_uart_write(hal_uart_t *u, const uint8_t *b, size_t s, uint32_t t) { (void)u;(void)b;(void)s;(void)t; return (int)s; }
int tos_hal_uart_read(hal_uart_t *u, uint8_t *b, size_t s, uint32_t t) { (void)u;(void)s;(void)t; if(b&&s) memset(b,0,s); return (int)s; }

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
