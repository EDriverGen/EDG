#include "dps310_ref.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

int dps310_tobudos_main(void) {
    struct dps310_device dev;
    if (dps310_init(&dev, &hi2c1, DPS310_DEFAULT_ADDR) != 0) {
        printf("[DPS310] init FAILED\n");
        return -1;
    }
    if (dps310_probe(&dev) != 0) { printf("[DPS310] probe FAILED\n"); return -1; }
    printf("[DPS310] addr=0x%02X probe OK\n", DPS310_DEFAULT_ADDR);
    if (dps310_read_calibration(&dev) != 0) {
        printf("[DPS310] calibration read FAILED\n");
        return -1;
    }
    for (int i = 0; i < 5; i++) {
        int32_t pres, temp;
        if (dps310_read_temperature(&dev, &temp) == 0 &&
            dps310_read_pressure(&dev, &pres) == 0)
            printf("[DPS310] sample=%d temp=%d.%02d C pressure=%d.%02d Pa\n", i+1,
                   (int)(temp/1000), (int)((temp>=0?temp:-temp)%1000/10),
                   (int)pres, 0);
    }
    return 0;
}
