/*
 * MCP3008 sample for RIOT
 */
#include "mcp3008_ref.h"
#include <stdio.h>

int main(void)
{
    struct mcp3008_device adc;
    mcp3008_init(&adc, SPI_DEV(0), GPIO_PIN(0, 4), 3300);

    while (1) {
        for (int i = 0; i < 8; i++) {
            uint16_t mv;
            if (mcp3008_read_voltage(&adc, i, &mv) == 0)
                printf("CH%d: %d.%03d V\n", i, mv/1000, mv%1000);
        }
        xtimer_msleep(1000);
    }
    return 0;
}
