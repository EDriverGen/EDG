/*
 * DS18B20 sample for TencentOS Tiny
 */
#include "ds18b20_ref.h"
#include <stdio.h>

void ds18b20_sample_task(void *arg)
{
    (void)arg;
    struct ds18b20_device sensor;
    ds18b20_init(&sensor, GPIOA, GPIO_PIN_0);
    for(int i=0;i<5;i++){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            printf("T:%ld.%02ld C\r\n",temp/100,(temp>=0?temp:-temp)%100);
        tos_task_delay(1000);
    }
}
