#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>

struct mhz19b_dev {
    int uart_num;
};

int mhz19b_init(struct mhz19b_dev *dev, int uart_num);
int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw);

#endif