#include "tmp421_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int tmp421_freertos_main(void) {
    struct tmp421_device dev;
    tmp421_init(&dev, &hi2c1, TMP421_ADDR_DEFAULT);
    if (tmp421_probe(&dev) != 0) { printf("[TMP421] probe FAILED\n"); return -1; }
    printf("[TMP421] addr=0x%02X probe OK\n", TMP421_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int32_t local_t, remote_t;
        if (tmp421_read_local_temp(&dev, &local_t) == 0 &&
            tmp421_read_remote_temp(&dev, &remote_t) == 0)
            printf("[TMP421] sample=%d local=%d.%03d C remote=%d.%03d C\n", i+1,
                   (int)(local_t/1000), (int)(local_t>=0?local_t%1000:(-local_t)%1000),
                   (int)(remote_t/1000), (int)(remote_t>=0?remote_t%1000:(-remote_t)%1000));
    }
    return 0;
}
