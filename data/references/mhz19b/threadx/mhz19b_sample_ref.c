/*
 * MH-Z19B sample for ThreadX
 */
#include "mhz19b_ref.h"
#include <stdio.h>

extern const struct mhz19b_uart_ops platform_uart_ops;
extern void *platform_uart_ctx;

void mhz19b_sample_entry(ULONG param)
{
    (void)param;
    struct mhz19b_device co2;
    if (mhz19b_init(&co2, &platform_uart_ops, platform_uart_ctx) != 0)
    { printf("Init failed\r\n"); return; }
    for (int i = 0; i < 5; i++) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            printf("CO2: %d ppm\r\n", ppm);
        tx_thread_sleep(200);
    }
}
