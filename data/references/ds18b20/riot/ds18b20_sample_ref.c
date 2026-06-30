/*
 * DS18B20 sample for RIOT
 */
#include "ds18b20_ref.h"
#include <stdio.h>

int main(void)
{
    struct ds18b20_device sensor;
    ds18b20_init(&sensor, GPIO_PIN(0, 0));
    while(1){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            printf("T:%ld.%02ld C\n",temp/100,(temp>=0?temp:-temp)%100);
        xtimer_msleep(1000);
    }
    return 0;
}
