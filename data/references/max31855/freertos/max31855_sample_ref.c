/*
 * MAX31855 sample for FreeRTOS + STM32 HAL
 */
#include "max31855_ref.h"
#include <stdio.h>

void max31855_sample_task(void *pvParameters)
{
    struct max31855_device tc;
    extern SPI_HandleTypeDef hspi1;
    (void)pvParameters;

    if (max31855_init(&tc, &hspi1, GPIOA, GPIO_PIN_4) != 0)
    { printf("Init failed\r\n"); vTaskDelete(NULL); return; }

    for (int i = 0; i < 5; i++) {
        int32_t tc_temp;
        if (max31855_read_thermocouple(&tc, &tc_temp) == 0)
            printf("TC: %d.%03d C\r\n", (int)(tc_temp/1000),
                   (int)((tc_temp>=0?tc_temp:-tc_temp)%1000));
        else
            printf("Read error\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}
