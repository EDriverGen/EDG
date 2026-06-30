/*
 * MCP3008 sample for TencentOS Tiny
 */
#include "mcp3008_ref.h"
#include <stdio.h>

void mcp3008_sample_task(void *arg)
{
    (void)arg;
    struct mcp3008_device adc;
    extern SPI_HandleTypeDef hspi1;

    if (mcp3008_init(&adc, &hspi1, GPIOA, GPIO_PIN_4, 3300) != 0)
    { printf("Init failed\r\n"); return; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == 0)
            printf("CH%d: %d.%03d V\r\n", i, mv/1000, mv%1000);
    }
}
