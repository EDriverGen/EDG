/*
 * MCP3008 sample for Zephyr
 */
#include "mcp3008_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcp3008_sample, LOG_LEVEL_INF);

void main(void)
{
    struct mcp3008_device adc;
    const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi1));

    if (mcp3008_init(&adc, spi, NULL, 3300) != 0)
    { LOG_ERR("Init failed"); return; }

    while (1) {
        for (int i = 0; i < 8; i++) {
            uint16_t mv;
            if (mcp3008_read_voltage(&adc, i, &mv) == 0)
                LOG_INF("CH%d: %d.%03d V", i, mv/1000, mv%1000);
        }
        k_msleep(1000);
    }
}
