#include "bme280_ref.h"
#include <stdio.h>
extern void *g_bme280_i2c_context;
extern const struct bme280_i2c_ops g_bme280_i2c_ops;
int bme280_threadx_main(void) {
    struct bme280_device dev;
    int ret;
    bme280_init(&dev, g_bme280_i2c_context, &g_bme280_i2c_ops, BME280_ADDR_DEFAULT);
    if (bme280_probe(&dev) != 0) { printf("[BME280] probe FAILED\n"); return -1; }
    printf("[BME280] addr=0x%02X probe OK\n", BME280_ADDR_DEFAULT);
    ret = bme280_read_calibration(&dev);
    if (ret != 0) { printf("[BME280] calibration FAILED (%d)\n", ret); return -1; }
    for (int i = 0; i < 3; i++) {
        int32_t temp; uint32_t press, hum;
        if (bme280_read(&dev, &temp, &press, &hum) == 0)
            printf("[BME280] temp=%d.%03d C press=%u Pa hum=%u.%03u %%\n",
                   (int)(temp/1000), (int)(temp>=0?temp%1000:(-temp)%1000),
                   (unsigned)press, (unsigned)(hum/1000), (unsigned)(hum%1000));
    }
    return 0;
}
