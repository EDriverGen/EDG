/*
 * MH-Z19B CO2 sensor driver for OpenHarmony HDF (UART)
 */
#ifndef MHZ19B_REF_H
#define MHZ19B_REF_H

#include "hdf_base.h"
#include "uart_if.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MHZ19B_BAUD_RATE      9600
#define MHZ19B_FRAME_LEN      9
#define MHZ19B_CMD_READ_CO2   0x86
#define MHZ19B_CMD_CALIBRATE  0x87
#define MHZ19B_CMD_ABC        0x79
#define MHZ19B_START_BYTE     0xFF
#define MHZ19B_SENSOR_NUM     0x01

struct mhz19b_device
{
    DevHandle uart_handle;
};

int32_t mhz19b_init(struct mhz19b_device *dev, uint32_t port);
void mhz19b_deinit(struct mhz19b_device *dev);
int32_t mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm);
int32_t mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable);

#ifdef __cplusplus
}
#endif
#endif
