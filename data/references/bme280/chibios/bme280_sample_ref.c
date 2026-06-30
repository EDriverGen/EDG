#include "bme280_ref.h"
#include <stdio.h>
extern I2CDriver I2CD1;
#define bsp_i2c_handle (&I2CD1)

int bme280_chibios_main(void) {
    struct bme280_device dev;
    bme280_init(&dev, &I2CD1, BME280_ADDR_DEFAULT);
    if (bme280_probe(&dev) != 0) { printf("[BME280] probe FAILED\n"); return -1; }
    printf("[BME280] addr=0x%02X probe OK\n", BME280_ADDR_DEFAULT);
    bme280_read_calibration(&dev);
    for (int i = 0; i < 3; i++) {
        int32_t temp; uint32_t press, hum;
        if (bme280_read(&dev, &temp, &press, &hum) == 0)
            printf("[BME280] temp=%d.%03d C press=%u Pa hum=%u.%03u %%\n",
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000),
                   (unsigned)press, (unsigned)(hum/1000), (unsigned)(hum%1000));
    }
    return 0;
}
