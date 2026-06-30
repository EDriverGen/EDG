#include "ds3231_ref.h"
#include <stdio.h>
extern struct rt_i2c_bus_device *bsp_i2c_handle;

int ds3231_rtthread_main(void) {
    struct ds3231_device dev;
    ds3231_init(&dev, bsp_i2c_handle, DS3231_ADDR_DEFAULT);
    if (ds3231_probe(&dev) != 0) { printf("[DS3231] probe FAILED\n"); return -1; }
    printf("[DS3231] addr=0x%02X probe OK\n", DS3231_ADDR_DEFAULT);
    for (int i = 0; i < 3; i++) {
        struct ds3231_time t; int32_t temp;
        if (ds3231_read_time(&dev, &t) == 0)
            printf("[DS3231] time=20%02d-%02d-%02d %02d:%02d:%02d\n",
                   t.year, t.month, t.date, t.hours, t.minutes, t.seconds);
        if (ds3231_read_temperature(&dev, &temp) == 0)
            printf("[DS3231] temp=%d.%03d C\n",
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
    }
    return 0;
}
