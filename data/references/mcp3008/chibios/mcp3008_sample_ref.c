/*
 * MCP3008 sample for ChibiOS
 */
#include "mcp3008_ref.h"
#include "chprintf.h"

void mcp3008_sample_thread(void *arg)
{
    (void)arg;
    struct mcp3008_device adc;
    static const SPIConfig spi_cfg = { NULL, GPIOA, 4,
        SPI_CR1_BR_1 | SPI_CR1_BR_0 };

    mcp3008_init(&adc, &SPID1, &spi_cfg, 3300);

    while (true) {
        for (int i = 0; i < 8; i++) {
            uint16_t mv;
            if (mcp3008_read_voltage(&adc, i, &mv) == 0)
                chprintf((BaseSequentialStream *)&SD1, "CH%d: %d.%03d V\r\n", i, mv/1000, mv%1000);
        }
        chThdSleepMilliseconds(1000);
    }
}
