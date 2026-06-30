#include "vl53l0x_ref.h"
#include <stdio.h>
int vl53l0x_openharmony_main(void) {
    struct vl53l0x_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    vl53l0x_init(&dev, bus, VL53L0X_ADDR_DEFAULT);
    if (vl53l0x_probe(&dev) != 0) { printf("[VL53L0X] probe FAILED\n"); return -1; }
    printf("[VL53L0X] addr=0x%02X probe OK\n", VL53L0X_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        uint16_t range;
        if (vl53l0x_read_range_mm(&dev, &range) == 0)
            printf("[VL53L0X] sample=%d range=%u mm\n", i+1, (unsigned)range);
    }
    I2cClose(bus);
    return 0;
}
