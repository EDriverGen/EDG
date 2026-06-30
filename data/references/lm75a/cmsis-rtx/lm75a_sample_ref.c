#include "lm75a_ref.h"
#include <stdio.h>

void lm75a_sample(void)
{
    I2C_HandleTypeDef i2c;
    struct lm75a_device dev;
    int32_t temp_mc = 0;

    if (lm75a_init(&dev, &i2c, LM75A_DEFAULT_ADDR) == 0 &&
        lm75a_read_temp_mcelsius(&dev, &temp_mc) == 0) {
        printf("LM75A %ld mC\n", (long)temp_mc);
    }
}
