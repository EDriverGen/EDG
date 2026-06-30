/*
 * ADXL345 sample for OpenHarmony HDF
 */
#include "adxl345_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG adxl345_sample

int adxl345_sample(void)
{
    struct adxl345_device acc;
    if (adxl345_init(&acc, 0, 0, ADXL345_RANGE_2G) != HDF_SUCCESS)
    { HDF_LOGE("Init failed"); return HDF_FAILURE; }
    for (int i = 0; i < 10; i++) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == HDF_SUCCESS)
            HDF_LOGI("X:%d Y:%d Z:%d mg", x, y, z);
        OsalMSleep(100);
    }
    adxl345_deinit(&acc);
    return HDF_SUCCESS;
}
