/*
 * DS18B20 sample for ChibiOS
 */
#include "ds18b20_ref.h"
#include "chprintf.h"

void ds18b20_sample_thread(void *arg)
{
    (void)arg;
    struct ds18b20_device sensor;
    ds18b20_init(&sensor, GPIOA, 0);
    while(true){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            chprintf((BaseSequentialStream*)&SD1,"T:%ld.%02ld C\r\n",temp/100,(temp>=0?temp:-temp)%100);
        chThdSleepMilliseconds(1000);
    }
}
