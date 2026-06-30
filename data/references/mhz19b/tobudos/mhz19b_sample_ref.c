/*
 * MH-Z19B sample for TencentOS Tiny
 */
#include "mhz19b_ref.h"
#include <stdio.h>

void mhz19b_sample_task(void *arg)
{
    (void)arg;
    struct mhz19b_device co2;
    extern UART_HandleTypeDef huart2;

    if (mhz19b_init(&co2, &huart2) != 0)
    { printf("Init failed\r\n"); return; }
    for (int i = 0; i < 5; i++) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            printf("CO2: %d ppm\r\n", ppm);
        tos_task_delay(2000);
    }
}
