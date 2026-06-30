/*
 * DS18B20 sample for OpenHarmony HDF
 */
#include "ds18b20_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG ds18b20_sample

int ds18b20_sample(void)
{
    struct ds18b20_device sensor;
    if(ds18b20_init(&sensor,0)!=HDF_SUCCESS){ HDF_LOGE("Init failed"); return HDF_FAILURE; }
    for(int i=0;i<5;i++){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==HDF_SUCCESS)
            HDF_LOGI("T:%ld.%02ld C",temp/100,(temp>=0?temp:-temp)%100);
        OsalMSleep(1000);
    }
    return HDF_SUCCESS;
}
