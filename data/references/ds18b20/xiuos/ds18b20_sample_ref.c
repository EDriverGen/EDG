/*
 * DS18B20 sample for XiUOS
 */
#include "ds18b20_ref.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    struct ds18b20_device sensor;
    const char *path=(argc>1)?argv[1]:"/dev/gpio0";
    if(ds18b20_init(&sensor,path)!=0){ printf("Init failed\n"); return -1; }
    for(int i=0;i<5;i++){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            printf("T:%ld.%02ld C\n",temp/100,(temp>=0?temp:-temp)%100);
        PrivTaskDelay(1000);
    }
    ds18b20_deinit(&sensor); return 0;
}
