/*
 * MH-Z19B sample for NuttX
 */
#include "mhz19b_ref.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, FAR char *argv[])
{
    struct mhz19b_device co2;
    const char *path = (argc > 1) ? argv[1] : "/dev/ttyS1";
    if (mhz19b_init(&co2, path) != 0)
    { printf("Init failed\n"); return -1; }
    for (int i = 0; i < 5; i++) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            printf("CO2: %d ppm\n", ppm);
        sleep(2);
    }
    mhz19b_deinit(&co2);
    return 0;
}
