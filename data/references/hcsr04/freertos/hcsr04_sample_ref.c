#include "hcsr04_ref.h"
#include <stdio.h>

int hcsr04_freertos_main(void) {
    struct hcsr04_device dev;
    /* Example pins only; adjust to your board wiring. */
    if (hcsr04_init(&dev, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1) < 0) {
        printf("[HC-SR04] init FAILED
");
        return -1;
    }
    printf("[HC-SR04] initialized
");
    for (int i = 0; i < 5; i++) {
        int32_t dist;
        if (hcsr04_read_distance_mm(&dev, &dist) == 0)
            printf("[HC-SR04] sample=%d distance=%d.%d cm
", i+1,
                   (int)(dist/10), (int)(dist%10));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return 0;
}
