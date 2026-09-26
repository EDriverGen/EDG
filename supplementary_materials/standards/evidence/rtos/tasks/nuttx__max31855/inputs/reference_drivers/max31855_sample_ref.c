#include "max31855_ref.h"
#include <stdio.h>
#include <unistd.h>

int max31855_main(int argc, FAR char *argv[])
{
    struct max31855_device tc; FAR struct spi_dev_s *spi = stm32_spibus_initialize(1);
    if (!spi) { printf("SPI fail\n"); return -1; }
    max31855_init(&tc, spi, SPIDEV_USER(0));
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            printf("TC: %d.%03d C\n", (int)(t/1000), (int)((t>=0?t:-t)%1000));
        sleep(1);
    }
    return 0;
}
