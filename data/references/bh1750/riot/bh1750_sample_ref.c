/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * BH1750 Light Sensor Driver for RIOT OS
 */
#include "bh1750_ref.h"
#include "periph/i2c.h"
#include "ztimer.h"
#include <stdio.h>

int main(void)
{
    struct bh1750_device dev;
    int ret;

    i2c_t bus = I2C_DEV(0);

    bh1750_init(&dev, bus, BH1750_DEFAULT_ADDR);

    ret = bh1750_probe(&dev);
    if (ret != 0) {
        printf("BH1750 Light Sensor not found!\r\n");
        return 1;
    }

    printf("BH1750 Light Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    uint32_t lux;
    ret = bh1750_read_lux_x100(&dev, &lux);
    if (ret == 0) {
        printf("BH1750: %u.%02u lux\r\n", (unsigned)(lux / 100), (unsigned)(lux % 100));
    }
        ztimer_sleep(ZTIMER_MSEC, 1000);
    }
    return 0;
}
