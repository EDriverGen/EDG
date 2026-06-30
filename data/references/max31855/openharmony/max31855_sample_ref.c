#include "max31855_ref.h"
#include "hdf_log.h"
#include "osal_time.h"
#define HDF_LOG_TAG max31855_sample

void max31855_sample(void) {
    struct max31855_device tc;
    if (max31855_init(&tc, 0, 0) != HDF_SUCCESS) { HDF_LOGE("Init fail"); return; }
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == HDF_SUCCESS)
            HDF_LOGI("TC: %d.%03d C", t/1000, (t>=0?t:-t)%1000);
        OsalMSleep(1000);
    }
    max31855_deinit(&tc);
}
