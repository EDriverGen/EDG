/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for ChibiOS
 */
#include "tmp421_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void tmp421_sample(void)
{
    struct tmp421_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    tmp421_init(&dev, bus, TMP421_ADDR_DEFAULT);

    ret = tmp421_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "TMP421 Remote Temperature Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "TMP421 Remote Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t local_t, remote_t;
    ret = tmp421_read_local_temp(&dev, &local_t);
    if (ret == 0) {
        chprintf((BaseSequentialStream *)&SD1, "TMP421 local: %d.%03d C\r\n", (int)(local_t / 1000), (int)(local_t >= 0 ? local_t % 1000 : (-local_t) % 1000));
    }
    ret = tmp421_read_remote_temp(&dev, &remote_t);
    if (ret == 0) {
        chprintf((BaseSequentialStream *)&SD1, "TMP421 remote: %d.%03d C\r\n", (int)(remote_t / 1000), (int)(remote_t >= 0 ? remote_t % 1000 : (-remote_t) % 1000));
    }
        chThdSleepMilliseconds(1000);
    }
}
