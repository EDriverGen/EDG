/*
 * MCP3008 sample for FreeRTOS + STM32 HAL
 */
#include "mcp3008_ref.h"
#include <stdio.h>

void mcp3008_sample_task(void *pvParameters)
{
    struct mcp3008_device adc;
    extern SPI_HandleTypeDef hspi1;
    (void)pvParameters;

    if (mcp3008_init(&adc, &hspi1, GPIOA, GPIO_PIN_4, 3300) != 0)
    { printf("Init failed\r\n"); vTaskDelete(NULL); return; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == 0)
            printf("CH%d: %d.%03d V\r\n", i, mv/1000, mv%1000);
        else
            printf("CH%d: error\r\n", i);
    }
    vTaskDelete(NULL);
}
