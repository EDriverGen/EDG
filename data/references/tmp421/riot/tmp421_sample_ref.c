/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * TMP421 Remote Temperature Sensor Driver for RIOT OS
 */
#include "tmp421_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct tmp421_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    tmp421_init(&dev, bus, TMP421_ADDR_DEFAULT);

    ret = tmp421_probe(&dev);
    if (ret != 0) {
        printf("TMP421 Remote Temperature Sensor not found!\r\n");
        return 1;
    }

    printf("TMP421 Remote Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t local_t, remote_t;
    ret = tmp421_read_local_temp(&dev, &local_t);
    if (ret == 0) {
        printf("TMP421 local: %d.%03d C\r\n", (int)(local_t / 1000), (int)(local_t >= 0 ? local_t % 1000 : (-local_t) % 1000));
    }
    ret = tmp421_read_remote_temp(&dev, &remote_t);
    if (ret == 0) {
        printf("TMP421 remote: %d.%03d C\r\n", (int)(remote_t / 1000), (int)(remote_t >= 0 ? remote_t % 1000 : (-remote_t) % 1000));
    }
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
