/*
 * MCP3008 sample for ThreadX
 */
#include "mcp3008_ref.h"
#include <stdio.h>

/* Platform-specific SPI ops must be provided */
extern const struct mcp3008_spi_ops platform_spi_ops;
extern void *platform_spi_ctx;

void mcp3008_sample_entry(ULONG param)
{
    (void)param;
    struct mcp3008_device adc;

    if (mcp3008_init(&adc, &platform_spi_ops, platform_spi_ctx, 3300) != 0)
    { printf("Init failed\r\n"); return; }

    for (int i = 0; i < 8; i++) {
        uint16_t mv;
        if (mcp3008_read_voltage(&adc, i, &mv) == 0)
            printf("CH%d: %d.%03d V\r\n", i, mv/1000, mv%1000);
    }
}
