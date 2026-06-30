/*
 * DS18B20 sample for FreeRTOS + STM32 HAL
 */
#include "ds18b20_ref.h"
#include <stdio.h>

void ds18b20_sample_task(void *pvParameters)
{
    struct ds18b20_device sensor;
    (void)pvParameters;
    if(ds18b20_init(&sensor, GPIOA, GPIO_PIN_0)!=0)
    { printf("Init failed\r\n"); vTaskDelete(NULL); return; }
    for(int i=0;i<5;i++){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            printf("T:%ld.%02ld C\r\n",temp/100,(temp>=0?temp:-temp)%100);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}
