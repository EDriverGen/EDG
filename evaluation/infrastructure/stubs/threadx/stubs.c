/* ThreadX + STM32 HAL stub implementations */
#include "threadx.h"
#include <stdlib.h>

/* GPIO port instances */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

/* ThreadX thread */
UINT tx_thread_create(TX_THREAD *tp, CHAR *n,
                      VOID (*ef)(ULONG), ULONG ei,
                      VOID *ss, ULONG sz,
                      UINT pri, UINT pt,
                      ULONG ts, UINT as) {
    (void)tp;(void)n;(void)ef;(void)ei;(void)ss;(void)sz;
    (void)pri;(void)pt;(void)ts;(void)as; return TX_SUCCESS;
}
UINT tx_thread_delete(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_thread_terminate(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
void tx_thread_sleep(ULONG t) { (void)t; }
UINT tx_thread_resume(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_thread_suspend(TX_THREAD *tp) { (void)tp; return TX_SUCCESS; }

/* ThreadX time */
ULONG tx_time_get(void) { return 0; }
void tx_time_set(ULONG t) { (void)t; }

/* ThreadX semaphore */
UINT tx_semaphore_create(TX_SEMAPHORE *sp, CHAR *n, ULONG c) {
    (void)sp;(void)n;(void)c; return TX_SUCCESS;
}
UINT tx_semaphore_delete(TX_SEMAPHORE *sp) { (void)sp; return TX_SUCCESS; }
UINT tx_semaphore_get(TX_SEMAPHORE *sp, ULONG w) { (void)sp;(void)w; return TX_SUCCESS; }
UINT tx_semaphore_put(TX_SEMAPHORE *sp) { (void)sp; return TX_SUCCESS; }

/* ThreadX mutex */
UINT tx_mutex_create(TX_MUTEX *mp, CHAR *n, UINT i) {
    (void)mp;(void)n;(void)i; return TX_SUCCESS;
}
UINT tx_mutex_delete(TX_MUTEX *mp) { (void)mp; return TX_SUCCESS; }
UINT tx_mutex_get(TX_MUTEX *mp, ULONG w) { (void)mp;(void)w; return TX_SUCCESS; }
UINT tx_mutex_put(TX_MUTEX *mp) { (void)mp; return TX_SUCCESS; }

/* ThreadX timer */
UINT tx_timer_create(TX_TIMER *tp, CHAR *n, VOID (*ef)(ULONG), ULONG ei,
                     ULONG it, ULONG rt, UINT aa) {
    (void)tp;(void)n;(void)ef;(void)ei;(void)it;(void)rt;(void)aa; return TX_SUCCESS;
}
UINT tx_timer_delete(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_timer_activate(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }
UINT tx_timer_deactivate(TX_TIMER *tp) { (void)tp; return TX_SUCCESS; }

/* ThreadX byte pool */
UINT tx_byte_pool_create(TX_BYTE_POOL *pp, CHAR *n, VOID *ps, ULONG sz) {
    (void)pp;(void)n;(void)ps;(void)sz; return TX_SUCCESS;
}
UINT tx_byte_allocate(TX_BYTE_POOL *pp, VOID **mp, ULONG sz, ULONG w) {
    (void)pp;(void)mp;(void)sz;(void)w; return TX_SUCCESS;
}
UINT tx_byte_release(VOID *mp) { (void)mp; return TX_SUCCESS; }
UINT tx_kernel_enter(void) { return TX_SUCCESS; }

/* STM32 HAL I2C */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t da,
                                          uint8_t *pd, uint16_t sz, uint32_t to) {
    (void)hi2c;(void)da;(void)pd;(void)sz;(void)to; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t da,
                                         uint8_t *pd, uint16_t sz, uint32_t to) {
    (void)hi2c;(void)da;(void)sz;(void)to;
    if(pd && sz>0) memset(pd,0x19,sz); return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t da,
                                    uint16_t ma, uint16_t ms,
                                    uint8_t *pd, uint16_t sz, uint32_t to) {
    (void)hi2c;(void)da;(void)ma;(void)ms;(void)pd;(void)sz;(void)to; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t da,
                                   uint16_t ma, uint16_t ms,
                                   uint8_t *pd, uint16_t sz, uint32_t to) {
    (void)hi2c;(void)da;(void)ma;(void)ms;(void)sz;(void)to;
    if(pd && sz>0) memset(pd,0x19,sz); return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t da,
                                        uint32_t tr, uint32_t to) {
    (void)hi2c;(void)da;(void)tr;(void)to; return HAL_OK;
}

/* STM32 HAL GPIO */
void HAL_GPIO_Init(GPIO_TypeDef *g, GPIO_InitTypeDef *i) { (void)g;(void)i; }
void HAL_GPIO_WritePin(GPIO_TypeDef *g, uint16_t p, GPIO_PinState s) { (void)g;(void)p;(void)s; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *g, uint16_t p) { (void)g;(void)p; return GPIO_PIN_RESET; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *g, uint16_t p) { (void)g;(void)p; }

/* STM32 HAL delay */
void HAL_Delay(uint32_t d) { (void)d; }
uint32_t HAL_GetTick(void) { return 0; }
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

__attribute__((weak)) int main(void) { return 0; }
