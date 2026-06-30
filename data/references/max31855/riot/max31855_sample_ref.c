#include "max31855_ref.h"
#include "xtimer.h"
#include <stdio.h>

int main(void) {
    struct max31855_device tc;
    max31855_init(&tc, SPI_DEV(0), GPIO_PIN(0, 4));
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            printf("TC: %d.%03d C\n", (int)(t/1000), (int)((t>=0?t:-t)%1000));
        xtimer_sleep(1);
    }
    return 0;
}
