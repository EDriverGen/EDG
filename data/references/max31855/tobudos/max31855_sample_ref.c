#include "max31855_ref.h"
#include <stdio.h>

void max31855_sample_task(void *arg) {
    (void)arg;
    struct max31855_device tc; extern SPI_HandleTypeDef hspi1;
    max31855_init(&tc, &hspi1, GPIOA, GPIO_PIN_4);
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            printf("TC: %d.%03d C\r\n", (int)(t/1000), (int)((t>=0?t:-t)%1000));
        tos_task_delay(1000);
    }
}
