/*
 * MCP3008 sample for XiUOS
 */
#include "mcp3008_ref.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    struct mcp3008_device adc;
    const char *path = (argc > 1) ? argv[1] : "/dev/spi0";

    if (mcp3008_init(&adc, path, 3300) != 0)
    { printf("Init failed\n"); return -1; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == 0)
            printf("CH%d: %d.%03d V\n", i, mv/1000, mv%1000);
    }
    mcp3008_deinit(&adc);
    return 0;
}
