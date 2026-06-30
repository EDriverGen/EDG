#include "lm75a_ref.h"
#include <stdio.h>

int lm75a_openharmony_main(void) {
    struct lm75a_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    lm75a_init(&dev, bus, LM75A_DEFAULT_ADDR);
    if (lm75a_probe(&dev) != 0) { printf("[LM75A] probe FAILED\n"); return -1; }
    printf("[LM75A] addr=0x%02X probe OK\n", LM75A_DEFAULT_ADDR);
    for (int i = 0; i < 5; i++) {
        int32_t temp;
        if (lm75a_read_temperature(&dev, &temp) == 0)
            printf("[LM75A] sample=%d temp=%d.%03d C\n", i+1,
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000));
    }
    I2cClose(bus);
    return 0;
}
