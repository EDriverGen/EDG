/*
 * MH-Z19B sample for RIOT
 */
#include "mhz19b_ref.h"
#include <stdio.h>

int main(void)
{
    struct mhz19b_device co2;
    mhz19b_init(&co2, UART_DEV(1));

    while (1) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            printf("CO2: %d ppm\n", ppm);
        xtimer_msleep(2000);
    }
    return 0;
}
