#include "bh1750_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int bh1750_tobudos_main(void) {
    struct bh1750_device dev;
    bh1750_init(&dev, &hi2c1, BH1750_DEFAULT_ADDR);
    if (bh1750_probe(&dev) != 0) { printf("[BH1750] probe FAILED\n"); return -1; }
    printf("[BH1750] addr=0x%02X probe OK\n", BH1750_DEFAULT_ADDR);
    for (int i = 0; i < 5; i++) {
        uint32_t lux;
        if (bh1750_read_lux_x100(&dev, &lux) == 0)
            printf("[BH1750] sample=%d lux=%u.%02u\n", i+1, (unsigned)(lux/100), (unsigned)(lux%100));
    }
    return 0;
}
