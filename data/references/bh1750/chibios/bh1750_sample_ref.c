/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Driver for ChibiOS
 */
#include "bh1750_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void bh1750_sample(void)
{
    struct bh1750_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    bh1750_init(&dev, bus, BH1750_DEFAULT_ADDR);

    ret = bh1750_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "BH1750 Light Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "BH1750 Light Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    uint32_t lux;
    ret = bh1750_read_lux_x100(&dev, &lux);
    if (ret == 0) {
        chprintf((BaseSequentialStream *)&SD1, "BH1750: %u.%02u lux\r\n", (unsigned)(lux / 100), (unsigned)(lux % 100));
    }
        chThdSleepMilliseconds(1000);
    }
}
