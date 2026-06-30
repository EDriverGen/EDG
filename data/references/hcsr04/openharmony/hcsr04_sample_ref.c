#include "hcsr04_ref.h"
#include <stdio.h>

int hcsr04_openharmony_main(void)
{
    struct hcsr04_device dev;

    /* Example GPIO numbers only; adjust to your board wiring. */
    if (hcsr04_init(&dev, 0, 1) != HDF_SUCCESS) {
        printf("[HC-SR04] init FAILED\n");
        return -1;
    }

    printf("[HC-SR04] initialized\n");
    for (int i = 0; i < 5; i++) {
        int32_t dist;

        if (hcsr04_read_distance_mm(&dev, &dist) == HDF_SUCCESS) {
            printf("[HC-SR04] sample=%d distance=%d.%d cm\n",
                i + 1, (int)(dist / 10), (int)(dist % 10));
        }
        OsalMSleep(500);
    }
    return 0;
}
