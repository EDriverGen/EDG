#include "vl53l0x_ref.h"
#include <stdio.h>


int vl53l0x_xiuos_main(void) {
    struct vl53l0x_device dev;
    vl53l0x_init(&dev, "/dev/i2c0", VL53L0X_ADDR_DEFAULT);
    if (vl53l0x_probe(&dev) != 0) { printf("[VL53L0X] probe FAILED\n"); return -1; }
    printf("[VL53L0X] addr=0x%02X probe OK\n", VL53L0X_ADDR_DEFAULT);
    for (int i = 0; i < 3; i++) {
        uint16_t range;
        if (vl53l0x_read_range_mm(&dev, &range) == 0)
            printf("[VL53L0X] range=%u mm\n", (unsigned)range);
    }
    return 0;
}
