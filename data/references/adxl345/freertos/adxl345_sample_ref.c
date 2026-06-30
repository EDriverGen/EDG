/*
 * ADXL345 sample for FreeRTOS + STM32 HAL
 */
#include "adxl345_ref.h"
#include <stdio.h>

void adxl345_sample_task(void *pvParameters)
{
    struct adxl345_device acc;
    extern SPI_HandleTypeDef hspi1;
    (void)pvParameters;

    if (adxl345_init(&acc, &hspi1, GPIOA, GPIO_PIN_4, ADXL345_RANGE_2G) != 0)
    { printf("Init failed\r\n"); vTaskDelete(NULL); return; }

    for (int i = 0; i < 10; i++) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            printf("X:%ld Y:%ld Z:%ld mg\r\n", (long)x, (long)y, (long)z);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}
