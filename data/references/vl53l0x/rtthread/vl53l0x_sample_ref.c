#include "vl53l0x_ref.h"
#include <stdio.h>
extern struct rt_i2c_bus_device *bsp_i2c_handle;

int vl53l0x_rtthread_main(void) {
    struct vl53l0x_device dev;
    vl53l0x_init(&dev, bsp_i2c_handle, VL53L0X_ADDR_DEFAULT);
    if (vl53l0x_probe(&dev) != 0) { printf("[VL53L0X] probe FAILED\n"); return -1; }
    printf("[VL53L0X] addr=0x%02X probe OK\n", VL53L0X_ADDR_DEFAULT);
    for (int i = 0; i < 3; i++) {
        uint16_t range;
        if (vl53l0x_read_range_mm(&dev, &range) == 0)
            printf("[VL53L0X] range=%u mm\n", (unsigned)range);
    }
    return 0;
}
