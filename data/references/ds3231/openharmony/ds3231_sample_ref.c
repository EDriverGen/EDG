#include "ds3231_ref.h"
#include <stdio.h>
int ds3231_openharmony_main(void) {
    struct ds3231_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    ds3231_init(&dev, bus, DS3231_ADDR_DEFAULT);
    if (ds3231_probe(&dev) != 0) { printf("[DS3231] probe FAILED\n"); return -1; }
    printf("[DS3231] addr=0x%02X probe OK\n", DS3231_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        struct ds3231_time t;
        int32_t temp;
        if (ds3231_read_time(&dev, &t) == 0)
            printf("[DS3231] sample=%d time=20%02d-%02d-%02d %02d:%02d:%02d\n", i+1,
                   t.year, t.month, t.date, t.hours, t.minutes, t.seconds);
        if (ds3231_read_temperature(&dev, &temp) == 0)
            printf("[DS3231] sample=%d temp=%d.%03d C\n", i+1,
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
    }
    I2cClose(bus);
    return 0;
}
