/*
 * MH-Z19B sample for FreeRTOS + STM32 HAL
 */
#include "mhz19b_ref.h"
#include <stdio.h>

void mhz19b_sample_task(void *pvParameters)
{
    struct mhz19b_device co2;
    extern UART_HandleTypeDef huart2;
    (void)pvParameters;

    if (mhz19b_init(&co2, &huart2) != 0)
    { printf("Init failed\r\n"); vTaskDelete(NULL); return; }

    for (int i = 0; i < 5; i++) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            printf("CO2: %d ppm\r\n", ppm);
        else
            printf("Read error\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
}
