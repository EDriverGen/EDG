/* FreeRTOS + STM32 HAL stub implementations */
#include "freertos.h"
#include <stdlib.h>

/* GPIO port instances */
static GPIO_TypeDef _gpioa, _gpiob, _gpioc, _gpiod;
GPIO_TypeDef *GPIOA = &_gpioa;
GPIO_TypeDef *GPIOB = &_gpiob;
GPIO_TypeDef *GPIOC = &_gpioc;
GPIO_TypeDef *GPIOD = &_gpiod;

/* FreeRTOS stubs */
BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName,
                       uint16_t usStackDepth, void *pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask) {
    (void)pxTaskCode; (void)pcName; (void)usStackDepth;
    (void)pvParameters; (void)uxPriority; (void)pxCreatedTask;
    return pdPASS;
}
void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }
void vTaskDelete(TaskHandle_t xTaskToDelete) { (void)xTaskToDelete; }
TickType_t xTaskGetTickCount(void) { return 0; }
void taskENTER_CRITICAL(void) {}
void taskEXIT_CRITICAL(void) {}

SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void*)1; }
SemaphoreHandle_t xSemaphoreCreateBinary(void) { return (void*)1; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t t) { (void)s; (void)t; return pdPASS; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return pdPASS; }
void vSemaphoreDelete(SemaphoreHandle_t s) { (void)s; }

/* STM32 HAL I2C stubs */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                          uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)pData; (void)Size; (void)Timeout;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                         uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)pData; (void)Size; (void)Timeout;
    if (pData && Size > 0) memset(pData, 0x19, Size);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)MemAddress; (void)MemAddSize;
    (void)pData; (void)Size; (void)Timeout;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    (void)hi2c; (void)DevAddress; (void)MemAddress; (void)MemAddSize;
    (void)Size; (void)Timeout;
    if (pData && Size > 0) memset(pData, 0x19, Size);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *hi2c) { (void)hi2c; return HAL_OK; }

/* STM32 HAL GPIO stubs */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) { (void)GPIOx; (void)GPIO_Init; }
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    (void)GPIOx; (void)GPIO_Pin; (void)PinState;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx; (void)GPIO_Pin; return GPIO_PIN_RESET;
}
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; }

/* STM32 HAL delay stubs */
void HAL_Delay(uint32_t Delay) { (void)Delay; }
uint32_t HAL_GetTick(void) { return 0; }

__attribute__((weak)) int main(void) { return 0; }
