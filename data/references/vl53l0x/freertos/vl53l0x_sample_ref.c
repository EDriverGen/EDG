#include "vl53l0x_ref.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;

int vl53l0x_freertos_main(void) {
    struct vl53l0x_device dev;
    vl53l0x_init(&dev, &hi2c1, VL53L0X_ADDR_DEFAULT);
    if (vl53l0x_probe(&dev) != 0) { printf("[VL53L0X] probe FAILED\n"); return -1; }
    printf("[VL53L0X] addr=0x%02X probe OK\n", VL53L0X_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        uint16_t range;
        if (vl53l0x_read_range_mm(&dev, &range) == 0)
            printf("[VL53L0X] sample=%d range=%u mm\n", i+1, (unsigned)range);
    }
    return 0;
}
