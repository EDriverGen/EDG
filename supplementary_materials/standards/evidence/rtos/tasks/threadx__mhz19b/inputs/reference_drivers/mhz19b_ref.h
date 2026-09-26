/*
 * MH-Z19B CO2 sensor driver for ThreadX (HAL-agnostic)
 */
#ifndef MHZ19B_REF_H
#define MHZ19B_REF_H

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

struct mhz19b_uart_ops
{
    int (*uart_send)(void *ctx, const uint8_t *data, uint16_t len);
    int (*uart_recv)(void *ctx, uint8_t *data, uint16_t len, uint32_t timeout_ms);
};

struct mhz19b_device
{
    const struct mhz19b_uart_ops *ops;
    void *ctx;
};

int mhz19b_init(struct mhz19b_device *dev, const struct mhz19b_uart_ops *ops, void *ctx);
int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm);
int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable);

#ifdef __cplusplus
}
#endif
#endif
