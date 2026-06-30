#include "at24c256_ref.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;

int at24c256_tobudos_main(void) {
    struct at24c256_device dev;
    at24c256_init(&dev, &hi2c1, AT24C256_ADDR_DEFAULT);
    if (at24c256_probe(&dev) != 0) { printf("[AT24C256] probe FAILED\n"); return -1; }
    printf("[AT24C256] addr=0x%02X probe OK\n", AT24C256_ADDR_DEFAULT);

    /* Write test pattern */
    uint8_t wdata[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    at24c256_write(&dev, 0x0000, wdata, 4);
    printf("[AT24C256] wrote 4 bytes at addr 0x0000\n");

    /* Read back */
    uint8_t rdata[4] = {0};
    at24c256_read(&dev, 0x0000, rdata, 4);
    printf("[AT24C256] read back: 0x%02X 0x%02X 0x%02X 0x%02X\n",
           rdata[0], rdata[1], rdata[2], rdata[3]);

    int match = (rdata[0]==0xDE && rdata[1]==0xAD && rdata[2]==0xBE && rdata[3]==0xEF);
    printf("[AT24C256] verify: %s\n", match ? "MATCH" : "MISMATCH");
    return 0;
}
