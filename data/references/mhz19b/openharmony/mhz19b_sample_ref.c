/*
 * MH-Z19B sample for OpenHarmony HDF
 */
#include "mhz19b_ref.h"
#include "hdf_log.h"
#include "osal_time.h"
#define HDF_LOG_TAG mhz19b_sample

int mhz19b_sample(void)
{
    struct mhz19b_device co2;
    if (mhz19b_init(&co2, 1) != HDF_SUCCESS)
    { HDF_LOGE("Init failed"); return HDF_FAILURE; }
    for (int i = 0; i < 5; i++) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == HDF_SUCCESS)
            HDF_LOGI("CO2: %d ppm", ppm);
        OsalMSleep(2000);
    }
    mhz19b_deinit(&co2);
    return HDF_SUCCESS;
}
