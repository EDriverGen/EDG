/*
 * DHT22 sample for OpenHarmony HDF
 */
#include "dht22_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG dht22_sample

int dht22_sample(void)
{
    struct dht22_device sensor;
    if (dht22_init(&sensor, 0) != HDF_SUCCESS)
    { HDF_LOGE("Init failed"); return HDF_FAILURE; }
    for (int i = 0; i < 5; i++) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == HDF_SUCCESS)
            HDF_LOGI("T:%d.%d C  H:%d.%d %%", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        OsalMSleep(DHT22_MIN_INTERVAL_MS);
    }
    return HDF_SUCCESS;
}
