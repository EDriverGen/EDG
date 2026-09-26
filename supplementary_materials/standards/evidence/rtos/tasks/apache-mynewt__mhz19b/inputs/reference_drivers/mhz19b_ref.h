#ifndef MHZ19B_APACHE_MYNEWT_REF_H
#define MHZ19B_APACHE_MYNEWT_REF_H

#include "os/mynewt.h"
#include <stdint.h>

#define MHZ19B_BAUD_RATE     9600U
#define MHZ19B_FRAME_LEN     9U
#define MHZ19B_CMD_READ_CO2  0x86U
#define MHZ19B_CMD_CALIBRATE 0x87U
#define MHZ19B_CMD_ABC       0x79U
#define MHZ19B_START_BYTE    0xFFU
#define MHZ19B_SENSOR_NUM    0x01U

struct mhz19b_device {
    int uart_num;
};

int mhz19b_init(struct mhz19b_device *dev, int uart_num);
int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm);
int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable);
int mhz19b_calibrate_zero(struct mhz19b_device *dev);

#endif
