/*
 * FreeRTOS + STM32 HAL unified stub header for syntax-only compilation tests.
 * Covers: FreeRTOS kernel types, task/semaphore API,
 *         STM32 HAL I2C, GPIO, delay.
 * On Windows (case-insensitive FS), FreeRTOS.h == freertos.h = this file.
 */
#ifndef __FREERTOS_STUB_H__
#define __FREERTOS_STUB_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- RT-Thread type compatibility layer ---------- */
/* Allows test harness code (from test_vectors.json) that uses RT-Thread
   types to compile against FreeRTOS drivers without modification. */
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

/* ---------- FreeRTOS types ---------- */
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
typedef void* TaskHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* QueueHandle_t;
typedef void* TimerHandle_t;
typedef void* EventGroupHandle_t;
typedef uint32_t EventBits_t;
typedef void (*TaskFunction_t)(void *);

#define pdTRUE   ((BaseType_t) 1)
#define pdFALSE  ((BaseType_t) 0)
#define pdPASS   pdTRUE
#define pdFAIL   pdFALSE
#define portMAX_DELAY ((TickType_t) 0xFFFFFFFFUL)
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))
#define portTICK_PERIOD_MS      ((TickType_t)1)
#define configMINIMAL_STACK_SIZE ((uint16_t)128)
#define tskIDLE_PRIORITY ((UBaseType_t) 0U)

/* ---------- FreeRTOS task API ---------- */
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char *pcName,
                       uint16_t usStackDepth, void *pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask);
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelete(TaskHandle_t xTaskToDelete);
TickType_t xTaskGetTickCount(void);
void taskENTER_CRITICAL(void);
void taskEXIT_CRITICAL(void);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);

/* ---------- FreeRTOS semaphore/mutex ---------- */
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateBinary(void);
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount);
BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t sem);
void vSemaphoreDelete(SemaphoreHandle_t sem);

/* ---------- FreeRTOS queue ---------- */
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
void vQueueDelete(QueueHandle_t xQueue);

/* ---------- FreeRTOS timer ---------- */
TimerHandle_t xTimerCreate(const char *pcTimerName, TickType_t xTimerPeriodInTicks,
                           UBaseType_t uxAutoReload, void *pvTimerID,
                           void (*pxCallbackFunction)(TimerHandle_t));
BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);

/* ---------- FreeRTOS event group ---------- */
EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToWaitFor,
                                BaseType_t xClearOnExit, BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait);

/* ---------- FreeRTOS memory ---------- */
void *pvPortMalloc(size_t xSize);
void vPortFree(void *pv);

/* ---------- STM32 HAL status ---------- */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef enum { RESET = 0, SET = 1 } FlagStatus;
#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY 0xFFFFFFFFU
#endif

/* ---------- STM32 HAL I2C ---------- */
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

#define SPI_MODE_MASTER   0x00000104U
#define SPI_MODE_SLAVE    0x00000000U
#define SPI_DIRECTION_2LINES     0x00000000U
#define SPI_DATASIZE_8BIT        0x00000700U
#define SPI_POLARITY_LOW         0x00000000U
#define SPI_POLARITY_HIGH        0x00000002U
#define SPI_PHASE_1EDGE          0x00000000U
#define SPI_PHASE_2EDGE          0x00000001U
#define SPI_NSS_SOFT             0x00000200U
#define SPI_FIRSTBIT_MSB         0x00000000U
#define SPI_FIRSTBIT_LSB         0x00000008U
#define SPI_BAUDRATEPRESCALER_2  0x00000000U
#define SPI_BAUDRATEPRESCALER_4  0x00000008U
#define SPI_BAUDRATEPRESCALER_8  0x00000010U
#define SPI_BAUDRATEPRESCALER_16 0x00000018U
#define SPI_BAUDRATEPRESCALER_32 0x00000020U
#define SPI_BAUDRATEPRESCALER_64 0x00000028U

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
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

#define UART_WORDLENGTH_8B    0x00000000U
#define UART_WORDLENGTH_9B    0x00001000U
#define UART_STOPBITS_1       0x00000000U
#define UART_STOPBITS_2       0x00002000U
#define UART_PARITY_NONE      0x00000000U
#define UART_PARITY_EVEN      0x00000400U
#define UART_PARITY_ODD       0x00000600U
#define UART_MODE_TX_RX       0x0000000CU
#define UART_HWCONTROL_NONE   0x00000000U
#define UART_OVERSAMPLING_16  0x00000000U

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

#define GPIO_MODE_INPUT    0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_OUTPUT_OD 0x00000011U
#define GPIO_MODE_AF_PP    0x00000002U
#define GPIO_MODE_AF_OD    0x00000012U
#define GPIO_NOPULL  0x00000000U
#define GPIO_PULLUP  0x00000001U
#define GPIO_PULLDOWN 0x00000002U
#define GPIO_SPEED_FREQ_LOW    0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM 0x00000001U
#define GPIO_SPEED_FREQ_HIGH   0x00000003U

/* ---------- STM32 HAL delay ---------- */
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
void HAL_IncTick(void);
HAL_StatusTypeDef HAL_Init(void);

/* ---------- Misc ---------- */
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __FREERTOS_STUB_H__ */
