#include "bh1750_ref.h"
#include <stdio.h>

int bh1750_openharmony_main(void) {
    struct bh1750_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    bh1750_init(&dev, bus, BH1750_DEFAULT_ADDR);
    if (bh1750_probe(&dev) != 0) { printf("[BH1750] probe FAILED\n"); return -1; }
    printf("[BH1750] addr=0x%02X probe OK\n", BH1750_DEFAULT_ADDR);
    for (int i = 0; i < 5; i++) {
        uint32_t lux;
        if (bh1750_read_lux_x100(&dev, &lux) == 0)
            printf("[BH1750] sample=%d lux=%u.%02u\n", i+1, (unsigned)(lux/100), (unsigned)(lux%100));
    }
    I2cClose(bus);
    return 0;
}
