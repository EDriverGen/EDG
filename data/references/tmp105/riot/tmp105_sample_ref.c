/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * TMP105 Temperature Sensor Driver for RIOT OS
 */
#include "tmp105_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct tmp105_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    tmp105_init(&dev, bus, TMP105_ADDR_DEFAULT);

    ret = tmp105_probe(&dev);
    if (ret != 0) {
        printf("TMP105 Temperature Sensor not found!\r\n");
        return 1;
    }

    printf("TMP105 Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t temp;
    ret = tmp105_read_temperature(&dev, &temp);
    if (ret == 0) {
        printf("TMP105: %d.%03d C\r\n", (int)(temp / 1000), (int)(temp >= 0 ? temp % 1000 : (-temp) % 1000));
    }
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
