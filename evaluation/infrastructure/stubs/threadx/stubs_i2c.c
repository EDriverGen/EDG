/* Functional ThreadX + STM32 HAL I2C stubs for the evaluation harness.
 *
 * HAL_I2C_* route through hw_i2c.h (I2C1 registers) so
 * the driver really drives STM32 I2C1 in Renode.
 */
#include "threadx.h"
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
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) { return HAL_UART_Transmit(h, p, s, 1000); }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t s) { return HAL_UART_Receive(h, p, s, 1000); }

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
