/*
 * MH-Z19B sample for ChibiOS
 */
#include "mhz19b_ref.h"
#include "chprintf.h"

void mhz19b_sample_thread(void *arg)
{
    (void)arg;
    struct mhz19b_device co2;
    static const SerialConfig uart_cfg = { 9600, 0, 0, 0 };
    mhz19b_init(&co2, &SD2, &uart_cfg);

    while (true) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            chprintf((BaseSequentialStream *)&SD1, "CO2: %d ppm\r\n", ppm);
        chThdSleepMilliseconds(2000);
    }
}
