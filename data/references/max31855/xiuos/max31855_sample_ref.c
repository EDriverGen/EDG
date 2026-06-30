#include "max31855_ref.h"
#include <transform.h>
#include <stdio.h>

int main(void) {
    struct max31855_device tc;
    if (max31855_init(&tc, "/dev/spi0") != 0) return -1;
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            printf("TC: %d.%03d C\n", (int)(t/1000), (int)((t>=0?t:-t)%1000));
        PrivTaskDelay(1000);
    }
    max31855_deinit(&tc);
    return 0;
}
