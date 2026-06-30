#include "tmp105_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int tmp105_tobudos_main(void) {
    struct tmp105_device dev;
    tmp105_init(&dev, &hi2c1, TMP105_ADDR_DEFAULT);
    if (tmp105_probe(&dev) != 0) { printf("[TMP105] probe FAILED\n"); return -1; }
    printf("[TMP105] addr=0x%02X probe OK\n", TMP105_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int32_t temp;
        if (tmp105_read_temperature(&dev, &temp) == 0)
            printf("[TMP105] sample=%d temp=%d.%03d C\n", i+1,
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
    }
    return 0;
}
