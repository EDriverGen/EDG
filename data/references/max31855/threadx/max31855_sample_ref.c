#include "max31855_ref.h"
#include "tx_api.h"
#include <stdio.h>

extern void platform_spi_cs_select(void *ctx);
extern void platform_spi_cs_deselect(void *ctx);
extern int platform_spi_recv(void *ctx, uint8_t *buf, uint32_t len);

static const struct max31855_spi_ops spi_ops = {
    .cs_select=platform_spi_cs_select,.cs_deselect=platform_spi_cs_deselect,
    .spi_recv=platform_spi_recv };

void max31855_thread(ULONG arg) {
    (void)arg;
    struct max31855_device tc;
    max31855_init(&tc, &spi_ops, NULL);
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            printf("TC: %d.%03d C\n", (int)(t/1000), (int)((t>=0?t:-t)%1000));
        tx_thread_sleep(100);
    }
}
