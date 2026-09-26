/*
 * DS18B20 sample for ThreadX
 */
#include "ds18b20_ref.h"
#include <stdio.h>

extern const struct ds18b20_ow_ops platform_ow_ops;
extern void *platform_ow_ctx;

void ds18b20_sample_entry(ULONG param)
{
    (void)param;
    struct ds18b20_device sensor;
    if(ds18b20_init(&sensor,&platform_ow_ops,platform_ow_ctx)!=0)
    { printf("Init failed\r\n"); return; }
    for(int i=0;i<5;i++){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            printf("T:%ld.%02ld C\r\n",temp/100,(temp>=0?temp:-temp)%100);
        tx_thread_sleep(100);
    }
}
