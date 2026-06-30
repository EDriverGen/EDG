#include "sht30_ref.h"
#include <stdio.h>
int sht30_openharmony_main(void) {
    struct sht30_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    sht30_init(&dev, bus, SHT30_ADDR_DEFAULT);
    if (sht30_probe(&dev) != 0) { printf("[SHT30] probe FAILED\n"); return -1; }
    printf("[SHT30] addr=0x%02X probe OK\n", SHT30_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int32_t temp, rh;
        if (sht30_read(&dev, &temp, &rh) == 0)
            printf("[SHT30] sample=%d temp=%d.%03d C rh=%d.%03d %%\n", i+1,
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000),
                   (int)(rh/1000), (int)(rh%1000));
    }
    I2cClose(bus);
    return 0;
}
