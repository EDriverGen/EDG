#include "emc1413_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int emc1413_tobudos_main(void) {
    struct emc1413_device dev;
    emc1413_init(&dev, &hi2c1, EMC1413_ADDR_DEFAULT);
    if (emc1413_probe(&dev) != 0) { printf("[EMC1413] probe FAILED\n"); return -1; }
    printf("[EMC1413] addr=0x%02X probe OK\n", EMC1413_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int32_t int_t, ext1_t, ext2_t;
        if (emc1413_read_internal_temp(&dev, &int_t) == 0 &&
            emc1413_read_external1_temp(&dev, &ext1_t) == 0 &&
            emc1413_read_external2_temp(&dev, &ext2_t) == 0)
            printf("[EMC1413] sample=%d internal=%d.%03d C ext1=%d.%03d C ext2=%d.%03d C\n", i+1,
                   (int)(int_t/1000), (int)(int_t>=0?int_t%1000:(-int_t)%1000),
                   (int)(ext1_t/1000), (int)(ext1_t>=0?ext1_t%1000:(-ext1_t)%1000),
                   (int)(ext2_t/1000), (int)(ext2_t>=0?ext2_t%1000:(-ext2_t)%1000));
    }
    return 0;
}
