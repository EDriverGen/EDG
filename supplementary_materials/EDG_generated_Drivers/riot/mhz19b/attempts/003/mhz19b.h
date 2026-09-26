#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>

typedef struct {
    uart_t uart_dev;
} mhz19b_t;

void mhz19b_init(mhz19b_t *dev, uart_t uart);
int32_t mhz19b_read_co2(mhz19b_t *dev, int32_t *raw);

#endif /* MHZ19B_H */