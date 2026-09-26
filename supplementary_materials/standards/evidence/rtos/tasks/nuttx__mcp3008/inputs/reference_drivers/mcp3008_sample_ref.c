/*
 * MCP3008 sample for NuttX
 */
#include "mcp3008_ref.h"
#include <stdio.h>

int main(int argc, FAR char *argv[])
{
    struct mcp3008_device adc;
    FAR struct spi_dev_s *spi = up_spiinitialize(0);
    if (!spi) { printf("SPI init failed\n"); return -1; }

    if (mcp3008_init(&adc, spi, 0, 3300) != 0)
    { printf("MCP3008 init failed\n"); return -1; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == 0)
            printf("CH%d: %d.%03d V\n", i, mv/1000, mv%1000);
    }
    return 0;
}
