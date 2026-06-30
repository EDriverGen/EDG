/*
 * DS18B20 sample for Zephyr
 */
#include "ds18b20_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ds18b20_sample, LOG_LEVEL_INF);

static const struct gpio_dt_spec ow_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(ds18b20), gpios);

void main(void)
{
    struct ds18b20_device sensor;
    if(ds18b20_init(&sensor,&ow_pin)!=0){ LOG_ERR("Init failed"); return; }
    while(1){
        int32_t temp;
        if(ds18b20_read_temp(&sensor,&temp)==0)
            LOG_INF("T:%d.%02d C", (int)(temp/100), (int)((temp>=0?temp:-temp)%100));
        k_msleep(1000);
    }
}
