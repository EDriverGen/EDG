/* Functional ToBudOS + STM32 HAL I2C stubs for the evaluation harness.
 *
 * HAL_I2C_* route through hw_i2c.h (I2C1 registers) so
 * the driver really drives STM32 I2C1 in Renode.
 */
#include "tobudos.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ---- I2C bus stubs (real STM32 I2C1 via hw_i2c.h) ---- */

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = (uint8_t)(DevAddress >> 1);
    return hw_i2c_write(0, addr7, pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = (uint8_t)(DevAddress >> 1);
    return hw_i2c_read(0, addr7, pData, Size) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = (uint8_t)(DevAddress >> 1);
    /* Honour MemAddSize: 16-bit EEPROMs (AT24Cxx >= 4Kbit) emit a 2-byte
     * memory pointer (high byte first) in front of the write payload. */
    uint8_t buf[258];
    uint16_t off = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        buf[off++] = (uint8_t)(MemAddress >> 8);
    }
    buf[off++] = (uint8_t)(MemAddress & 0xFF);
    uint16_t copy = (Size + off > sizeof(buf)) ? (uint16_t)(sizeof(buf) - off) : Size;
    if (pData && copy) memcpy(buf + off, pData, copy);
    return hw_i2c_write(0, addr7, buf, (uint16_t)(off + copy)) == 0 ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)Timeout;
    uint8_t addr7 = (uint8_t)(DevAddress >> 1);
    /* Honour MemAddSize: 16-bit EEPROMs (AT24Cxx >= 4Kbit) emit a 2-byte
     * memory pointer (high byte first) before the restart-read phase. */
    uint8_t reg_buf[2];
    uint16_t reg_len = 0;
    if (MemAddSize == I2C_MEMADD_SIZE_16BIT) {
        reg_buf[reg_len++] = (uint8_t)(MemAddress >> 8);
    }
    reg_buf[reg_len++] = (uint8_t)(MemAddress & 0xFF);
    return hw_i2c_write_read(0, addr7, reg_buf, reg_len, pData, Size) == 0
               ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) { (void)hi2c; hw_i2c1_init(); return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                        uint32_t Trials, uint32_t Timeout) {
    (void)hi2c;(void)DevAddress;(void)Trials;(void)Timeout; return HAL_OK;
}

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
