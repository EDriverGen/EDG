/*
 * TobudOS + ChipAdaptation unified stub for syntax-only compilation tests.
 * Covers: TobudOS kernel (tos_k.h types, task/timer/mutex/sem, tos_sleep_ms),
 *         STM32 HAL I2C, GPIO, delay.
 */
#ifndef __TOBUDOS_STUB_H__
#define __TOBUDOS_STUB_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- RT-Thread type compatibility layer ---------- */
typedef uint8_t   rt_uint8_t;
typedef int16_t   rt_int16_t;
typedef uint16_t  rt_uint16_t;
typedef int32_t   rt_int32_t;
typedef uint32_t  rt_uint32_t;
typedef int       rt_err_t;
typedef int       rt_size_t;
typedef int       rt_base_t;
#ifndef RT_EOK
#define RT_EOK    0
#endif
#ifndef RT_ERROR
#define RT_ERROR  (-1)
#endif
#ifndef RT_NULL
#define RT_NULL   ((void *)0)
#endif

/* ---------- TobudOS base types ---------- */
typedef int k_err_t;
typedef uint32_t k_tick_t;
typedef uint32_t k_prio_t;
typedef uint32_t k_stack_t;
typedef void* k_task_t;
typedef void* k_timer_t;
typedef void* k_mutex_t;
typedef void* k_sem_t;
typedef void* k_mmblk_pool_t;

/* ---------- TobudOS return codes ---------- */
#define K_ERR_NONE              ((k_err_t)0)
#define OS_OK                   0
#define OS_ERR                  (-1)
#define K_ERR_OBJ_PTR_NULL      ((k_err_t)-1)
#define K_ERR_OBJ_INVALID       ((k_err_t)-2)
#define K_ERR_TASK_PRIO_INVALID ((k_err_t)-3)
#define K_ERR_TASK_STK_SIZE_INVALID ((k_err_t)-4)
#define K_ERR_PEND_TIMEOUT      ((k_err_t)-5)
#define K_ERR_PEND_DESTROY      ((k_err_t)-6)
#define K_ERR_DELAY_ZERO        ((k_err_t)-7)

/* ---------- TobudOS task API ---------- */
k_err_t tos_task_create(k_task_t *task, const char *name,
                        void (*entry)(void *), void *arg,
                        k_prio_t prio, k_stack_t *stk_base,
                        uint32_t stk_size, k_tick_t timeslice);
k_err_t tos_task_destroy(k_task_t *task);
k_err_t tos_task_delay(k_tick_t delay);

/* ---------- TobudOS timing ---------- */
void tos_sleep_ms(uint32_t ms);
k_tick_t tos_systick_get(void);
void tos_tick2ms(k_tick_t tick, uint32_t *ms);

/* ---------- TobudOS mutex ---------- */
k_err_t tos_mutex_create(k_mutex_t *mutex);
k_err_t tos_mutex_destroy(k_mutex_t *mutex);
k_err_t tos_mutex_pend(k_mutex_t *mutex);
k_err_t tos_mutex_post(k_mutex_t *mutex);
k_err_t tos_mutex_pend_timed(k_mutex_t *mutex, k_tick_t timeout);

/* ---------- TobudOS semaphore ---------- */
k_err_t tos_sem_create(k_sem_t *sem, uint32_t init_count);
k_err_t tos_sem_destroy(k_sem_t *sem);
k_err_t tos_sem_pend(k_sem_t *sem, k_tick_t timeout);
k_err_t tos_sem_post(k_sem_t *sem);

/* ---------- TobudOS timer ---------- */
k_err_t tos_timer_create(k_timer_t *timer, k_tick_t delay,
                         k_tick_t period, void (*callback)(void *), void *arg,
                         uint8_t opt);
k_err_t tos_timer_destroy(k_timer_t *timer);
k_err_t tos_timer_start(k_timer_t *timer);
k_err_t tos_timer_stop(k_timer_t *timer);

/* ---------- TobudOS memory ---------- */
void *tos_mmheap_alloc(size_t size);
void *tos_mmheap_calloc(size_t num, size_t size);
void tos_mmheap_free(void *ptr);

/* ---------- TobudOS kernel ---------- */
k_err_t tos_knl_init(void);
k_err_t tos_knl_start(void);

/* ---------- STM32 HAL status ---------- */
#ifndef _TOBUDOS_HAL_DEFINED
#define _TOBUDOS_HAL_DEFINED
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
#endif

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY 0xFFFFFFFFU
#endif

/* ---------- STM32 HAL I2C ---------- */
#ifndef __TOBUDOS_I2C_TYPEDEF
typedef struct { uint32_t dummy; } I2C_InitTypeDef;
typedef struct I2C_HandleTypeDef {
    void *Instance;
    I2C_InitTypeDef Init;
    uint8_t *pBuffPtr;
    uint16_t XferSize;
    volatile uint16_t XferCount;
    uint32_t State;
    uint32_t ErrorCode;
} I2C_HandleTypeDef;
#define __TOBUDOS_I2C_TYPEDEF
#endif

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                        uint32_t Trials, uint32_t Timeout);

#define I2C_MEMADD_SIZE_8BIT   0x00000001U
#define I2C_MEMADD_SIZE_16BIT  0x00000010U

/* ---------- STM32 HAL SPI ---------- */
#ifndef __TOBUDOS_SPI_TYPEDEF
#define __TOBUDOS_SPI_TYPEDEF
typedef struct { uint32_t dummy; } SPI_InitTypeDef;
typedef struct SPI_HandleTypeDef {
    void *Instance;
    SPI_InitTypeDef Init;
    uint8_t *pTxBuffPtr;
    uint16_t TxXferSize;
    volatile uint16_t TxXferCount;
    uint8_t *pRxBuffPtr;
    uint16_t RxXferSize;
    volatile uint16_t RxXferCount;
    uint32_t State;
    uint32_t ErrorCode;
} SPI_HandleTypeDef;

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData,
                                  uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi,
                                          uint8_t *pTxData, uint8_t *pRxData,
                                          uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *hspi);
#endif

/* ---------- STM32 HAL UART ---------- */
typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct UART_HandleTypeDef {
    void *Instance;
    UART_InitTypeDef Init;
    uint8_t *pTxBuffPtr;
    uint16_t TxXferSize;
    volatile uint16_t TxXferCount;
    uint8_t *pRxBuffPtr;
    uint16_t RxXferSize;
    volatile uint16_t RxXferCount;
    uint32_t State;
    uint32_t ErrorCode;
} UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart);

/* ---------- STM32 HAL GPIO ---------- */
typedef struct { uint32_t dummy; } GPIO_TypeDef;
typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

extern GPIO_TypeDef *GPIOA, *GPIOB, *GPIOC, *GPIOD;

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_All ((uint16_t)0xFFFF)

#define GPIO_MODE_INPUT     0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_OUTPUT_OD 0x00000011U
#define GPIO_MODE_AF_PP    0x00000002U
#define GPIO_MODE_AF_OD    0x00000012U
#define GPIO_NOPULL   0x00000000U
#define GPIO_PULLUP   0x00000001U
#define GPIO_PULLDOWN 0x00000002U
#define GPIO_SPEED_FREQ_LOW    0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM 0x00000001U
#define GPIO_SPEED_FREQ_HIGH   0x00000003U

/* ---------- STM32 HAL delay ---------- */
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_Init(void);

/* ---------- TOS HAL wrappers ---------- */
typedef struct { uint32_t dummy; } hal_uart_t;
int tos_hal_uart_init(hal_uart_t *uart, int port);
int tos_hal_uart_write(hal_uart_t *uart, const uint8_t *buf, size_t size, uint32_t timeout);
int tos_hal_uart_read(hal_uart_t *uart, uint8_t *buf, size_t size, uint32_t timeout);

/* ---------- Misc ---------- */
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __TOBUDOS_STUB_H__ */
