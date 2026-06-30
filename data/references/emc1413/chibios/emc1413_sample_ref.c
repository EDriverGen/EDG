/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 Temperature Sensor Driver for ChibiOS
 */
#include "emc1413_ref.h"
#include "hal.h"
#include "ch.h"
#include "chprintf.h"

void emc1413_sample(void)
{
    struct emc1413_device dev;
    int ret;

    I2CDriver *bus = &I2CD1;

    emc1413_init(&dev, bus, EMC1413_ADDR_DEFAULT);

    ret = emc1413_probe(&dev);
    if (ret != 0) {
        chprintf((BaseSequentialStream *)&SD1, "EMC1413 Temperature Sensor not found!\r\n");
        return;
    }

    chprintf((BaseSequentialStream *)&SD1, "EMC1413 Temperature Sensor detected\r\n");

    int i;
    for (i = 0; i < 10; i++) {
    int32_t int_t, ext1_t, ext2_t;
    ret = emc1413_read_internal_temp(&dev, &int_t);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "EMC1413 internal: %d.%03d C\r\n", (int)(int_t/1000), (int)(int_t>=0?int_t%1000:(-int_t)%1000));
    ret = emc1413_read_external1_temp(&dev, &ext1_t);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "EMC1413 ext1: %d.%03d C\r\n", (int)(ext1_t/1000), (int)(ext1_t>=0?ext1_t%1000:(-ext1_t)%1000));
    ret = emc1413_read_external2_temp(&dev, &ext2_t);
    if (ret == 0) chprintf((BaseSequentialStream *)&SD1, "EMC1413 ext2: %d.%03d C\r\n", (int)(ext2_t/1000), (int)(ext2_t>=0?ext2_t%1000:(-ext2_t)%1000));
        chThdSleepMilliseconds(1000);
    }
}
