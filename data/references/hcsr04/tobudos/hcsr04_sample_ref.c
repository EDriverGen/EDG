#include "hcsr04_ref.h"
#include <stdio.h>

int hcsr04_tobudos_main(void)
{
    struct hcsr04_device dev;

    /* Example pins only; adjust to your board wiring. */
    if (hcsr04_init(&dev, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1) < 0) {
        printf("[HC-SR04] init FAILED\n");
        return -1;
    }

    printf("[HC-SR04] initialized\n");
    for (int i = 0; i < 5; i++) {
        int32_t dist;

        if (hcsr04_read_distance_mm(&dev, &dist) == 0) {
            printf("[HC-SR04] sample=%d distance=%d.%d cm\n",
                i + 1, (int)(dist / 10), (int)(dist % 10));
        }
        tos_sleep_ms(500);
    }
    return 0;
}
