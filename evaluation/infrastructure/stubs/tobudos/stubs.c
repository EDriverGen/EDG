/* TobudOS + STM32 HAL stub implementations */
#include "tobudos.h"
#include <stdlib.h>

/* GPIO port instances */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

/* TobudOS task */
k_err_t tos_task_create(k_task_t *t, const char *n, void (*e)(void*), void *a,
                        k_prio_t p, k_stack_t *s, uint32_t sz, k_tick_t ts) {
    (void)t;(void)n;(void)e;(void)a;(void)p;(void)s;(void)sz;(void)ts; return K_ERR_NONE;
}
k_err_t tos_task_destroy(k_task_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_task_delay(k_tick_t d) { (void)d; return K_ERR_NONE; }
void tos_sleep_ms(uint32_t ms) { (void)ms; }
k_tick_t tos_systick_get(void) { return 0; }
void tos_tick2ms(k_tick_t tick, uint32_t *ms) { (void)tick; if(ms) *ms = 0; }

/* TobudOS mutex */
k_err_t tos_mutex_create(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_destroy(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_pend(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_post(k_mutex_t *m) { (void)m; return K_ERR_NONE; }
k_err_t tos_mutex_pend_timed(k_mutex_t *m, k_tick_t t) { (void)m;(void)t; return K_ERR_NONE; }

/* TobudOS semaphore */
k_err_t tos_sem_create(k_sem_t *s, uint32_t c) { (void)s;(void)c; return K_ERR_NONE; }
k_err_t tos_sem_destroy(k_sem_t *s) { (void)s; return K_ERR_NONE; }
k_err_t tos_sem_pend(k_sem_t *s, k_tick_t t) { (void)s;(void)t; return K_ERR_NONE; }
k_err_t tos_sem_post(k_sem_t *s) { (void)s; return K_ERR_NONE; }

/* TobudOS timer */
k_err_t tos_timer_create(k_timer_t *t, k_tick_t d, k_tick_t p,
                         void (*cb)(void*), void *a, uint8_t o) {
    (void)t;(void)d;(void)p;(void)cb;(void)a;(void)o; return K_ERR_NONE;
}
k_err_t tos_timer_destroy(k_timer_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_timer_start(k_timer_t *t) { (void)t; return K_ERR_NONE; }
k_err_t tos_timer_stop(k_timer_t *t) { (void)t; return K_ERR_NONE; }

/* TobudOS memory */
void *tos_mmheap_alloc(size_t sz) { return malloc(sz); }
void *tos_mmheap_calloc(size_t n, size_t sz) { return calloc(n, sz); }
void tos_mmheap_free(void *p) { free(p); }

/* TobudOS kernel */
k_err_t tos_knl_init(void) { return K_ERR_NONE; }
k_err_t tos_knl_start(void) { return K_ERR_NONE; }

/* TOS HAL wrappers */
int tos_hal_uart_init(hal_uart_t *u, int p) { (void)u;(void)p; return 0; }
int tos_hal_uart_write(hal_uart_t *u, const uint8_t *b, size_t s, uint32_t t) {
    (void)u;(void)b;(void)s;(void)t; return (int)s;
}
int tos_hal_uart_read(hal_uart_t *u, uint8_t *b, size_t s, uint32_t t) {
    (void)u;(void)s;(void)t; if(b && s) memset(b,0x19,s); return (int)s;
}

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
