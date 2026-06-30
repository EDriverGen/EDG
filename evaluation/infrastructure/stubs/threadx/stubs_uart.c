/* Functional ThreadX + STM32 HAL UART stubs for the evaluation harness.
 *
 * HAL_UART_* route through hw_uart_bus.h (USART1 registers) so
 * the driver really drives STM32 USART1 in Renode.
 */
#include "threadx.h"
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
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) {
    return HAL_UART_Transmit(h, p, s, 1000);
}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) {
    return HAL_UART_Receive(h, p, s, 1000);
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
