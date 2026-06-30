#include "sht30_ref.h"
#include <stdio.h>
extern struct device __fake_i2c_dev;
#define bsp_i2c_handle (&__fake_i2c_dev)

int sht30_zephyr_main(void) {
    struct sht30_device dev;
    sht30_init(&dev, &__fake_i2c_dev, SHT30_ADDR_DEFAULT);
    if (sht30_probe(&dev) != 0) { printf("[SHT30] probe FAILED\n"); return -1; }
    printf("[SHT30] addr=0x%02X probe OK\n", SHT30_ADDR_DEFAULT);
    for (int i = 0; i < 3; i++) {
        int32_t temp, rh;
        if (sht30_read(&dev, &temp, &rh) == 0)
            printf("[SHT30] temp=%d.%03d C rh=%d.%03d %%\n",
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000),
                   (int)(rh/1000), (int)(rh%1000));
    }
    return 0;
}
