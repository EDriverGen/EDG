/*
 * ThreadX + STM32 HAL unified stub header for syntax-only compilation tests.
 * Covers: ThreadX kernel (tx_api.h types, thread/timer/semaphore/mutex),
 *         STM32 HAL I2C, GPIO, delay.
 */
#ifndef __THREADX_STUB_H__
#define __THREADX_STUB_H__

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

/* ---------- ThreadX base types ---------- */
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef int INT;
typedef void VOID;
typedef unsigned long long ULONG64;
typedef long LONG;
typedef ULONG ALIGN_TYPE;

/* ---------- ThreadX return codes ---------- */
#define TX_SUCCESS              0x00
#define TX_DELETED              0x01
#define TX_NO_MEMORY            0x10
#define TX_POOL_ERROR           0x02
#define TX_PTR_ERROR            0x03
#define TX_WAIT_ERROR           0x04
#define TX_SIZE_ERROR           0x05
#define TX_GROUP_ERROR          0x06
#define TX_NO_EVENTS            0x07
#define TX_OPTION_ERROR         0x08
#define TX_QUEUE_ERROR          0x09
#define TX_QUEUE_EMPTY          0x0A
#define TX_QUEUE_FULL           0x0B
#define TX_SEMAPHORE_ERROR      0x0C
#define TX_NO_INSTANCE          0x0D
#define TX_THREAD_ERROR         0x0E
#define TX_PRIORITY_ERROR       0x0F
#define TX_MUTEX_ERROR          0x1C
#define TX_NOT_AVAILABLE        0x1D
#define TX_NOT_OWNED            0x1E
#define TX_INHERIT_ERROR        0x1F
#define TX_WAIT_FOREVER         ((ULONG)0xFFFFFFFFUL)
#define TX_NO_WAIT              ((ULONG)0)
#define TX_AUTO_START           1
#define TX_DONT_START           0
#define TX_AUTO_ACTIVATE        1
#define TX_NO_ACTIVATE          0
#define TX_INHERIT              1
#define TX_NO_INHERIT           0
#define TX_TIMER_TICKS_PER_SECOND 100

/* ---------- ThreadX thread ---------- */
typedef struct TX_THREAD_STRUCT {
    ULONG tx_thread_id;
    CHAR *tx_thread_name;
    VOID *tx_thread_stack_start;
    ULONG tx_thread_stack_size;
    UINT tx_thread_priority;
} TX_THREAD;

UINT tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr,
                      VOID (*entry_function)(ULONG), ULONG entry_input,
                      VOID *stack_start, ULONG stack_size,
                      UINT priority, UINT preempt_threshold,
                      ULONG time_slice, UINT auto_start);
UINT tx_thread_delete(TX_THREAD *thread_ptr);
UINT tx_thread_terminate(TX_THREAD *thread_ptr);
void tx_thread_sleep(ULONG timer_ticks);
UINT tx_thread_resume(TX_THREAD *thread_ptr);
UINT tx_thread_suspend(TX_THREAD *thread_ptr);

/* ---------- ThreadX time ---------- */
ULONG tx_time_get(void);
void tx_time_set(ULONG new_time);

/* ---------- ThreadX semaphore ---------- */
typedef struct TX_SEMAPHORE_STRUCT {
    ULONG tx_semaphore_id;
    CHAR *tx_semaphore_name;
    ULONG tx_semaphore_count;
} TX_SEMAPHORE;

UINT tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count);
UINT tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr);
UINT tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option);
UINT tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr);

/* ---------- ThreadX mutex ---------- */
typedef struct TX_MUTEX_STRUCT {
    ULONG tx_mutex_id;
    CHAR *tx_mutex_name;
    ULONG tx_mutex_ownership_count;
} TX_MUTEX;

UINT tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit);
UINT tx_mutex_delete(TX_MUTEX *mutex_ptr);
UINT tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option);
UINT tx_mutex_put(TX_MUTEX *mutex_ptr);

/* ---------- ThreadX timer ---------- */
typedef struct TX_TIMER_STRUCT {
    ULONG tx_timer_id;
    CHAR *tx_timer_name;
} TX_TIMER;

UINT tx_timer_create(TX_TIMER *timer_ptr, CHAR *name_ptr,
                     VOID (*expiration_function)(ULONG),
                     ULONG expiration_input, ULONG initial_ticks,
                     ULONG reschedule_ticks, UINT auto_activate);
UINT tx_timer_delete(TX_TIMER *timer_ptr);
UINT tx_timer_activate(TX_TIMER *timer_ptr);
UINT tx_timer_deactivate(TX_TIMER *timer_ptr);

/* ---------- ThreadX byte pool ---------- */
typedef struct TX_BYTE_POOL_STRUCT {
    ULONG tx_byte_pool_id;
    CHAR *tx_byte_pool_name;
} TX_BYTE_POOL;

UINT tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr,
                         VOID *pool_start, ULONG pool_size);
UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr,
                      ULONG memory_size, ULONG wait_option);
UINT tx_byte_release(VOID *memory_ptr);

/* ---------- ThreadX kernel ---------- */
UINT tx_kernel_enter(void);

/* ---------- STM32 HAL status ---------- */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef enum { RESET = 0, SET = 1 } FlagStatus;
#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY 0xFFFFFFFFU
#endif

/* ---------- STM32 HAL I2C ---------- */
typedef struct {
    uint32_t ClockSpeed;
    uint32_t DutyCycle;
    uint32_t OwnAddress1;
    uint32_t AddressingMode;
    uint32_t DualAddressMode;
    uint32_t OwnAddress2;
    uint32_t GeneralCallMode;
    uint32_t NoStretchMode;
} I2C_InitTypeDef;
typedef struct I2C_HandleTypeDef {
    void *Instance;
    I2C_InitTypeDef Init;
    uint8_t *pBuffPtr;
    uint16_t XferSize;
    volatile uint16_t XferCount;
    uint32_t State;
    uint32_t ErrorCode;
} I2C_HandleTypeDef;

#define I2C1 ((void *)0x40005400UL)
#define I2C_DUTYCYCLE_2           0x00000000U
#define I2C_ADDRESSINGMODE_7BIT   0x00000001U
#define I2C_DUALADDRESS_DISABLE   0x00000000U
#define I2C_GENERALCALL_DISABLE   0x00000000U
#define I2C_NOSTRETCH_DISABLE     0x00000000U

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
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)
#define GPIO_PIN_All ((uint16_t)0xFFFF)

#define GPIO_MODE_INPUT     0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_OUTPUT_OD 0x00000011U
#define GPIO_NOPULL   0x00000000U
#define GPIO_PULLUP   0x00000001U
#define GPIO_PULLDOWN 0x00000002U

#define GPIO_SPEED_FREQ_LOW    0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM 0x00000001U
#define GPIO_SPEED_FREQ_HIGH   0x00000003U

/* ---------- STM32 HAL SPI ---------- */
typedef struct SPI_HandleTypeDef {
    void *Instance;
    uint32_t Init_BaudRatePrescaler;
    uint32_t Init_Direction;
    uint32_t Init_CLKPolarity;
    uint32_t Init_CLKPhase;
    uint32_t Init_DataSize;
    uint32_t Init_NSS;
    uint32_t Init_FirstBit;
    uint32_t Init_Mode;
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

/* ---------- STM32 HAL UART ---------- */
typedef struct UART_HandleTypeDef {
    void *Instance;
    uint32_t Init_BaudRate;
    uint32_t Init_WordLength;
    uint32_t Init_StopBits;
    uint32_t Init_Parity;
    uint32_t Init_Mode;
    uint32_t Init_HwFlowCtl;
    uint32_t Init_OverSampling;
} UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

/* ---------- STM32 HAL delay ---------- */
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_Init(void);

/* ---------- Misc ---------- */
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

#define EIO 5
static inline void __NOP(void) { __asm__ volatile ("nop"); }

#ifdef __cplusplus
}
#endif

#endif /* __THREADX_STUB_H__ */
