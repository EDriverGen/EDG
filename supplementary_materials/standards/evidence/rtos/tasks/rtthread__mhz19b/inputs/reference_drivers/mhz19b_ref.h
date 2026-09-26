/*
 * MH-Z19B CO2 sensor driver for RT-Thread (UART)
 */
#ifndef DRIVERS_INCLUDE_MHZ19B_H_
#define DRIVERS_INCLUDE_MHZ19B_H_

#include <rtthread.h>
#include <rtdevice.h>

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
    rt_device_t serial;
    const char *uart_name;
};

rt_err_t mhz19b_init(struct mhz19b_device *dev, const char *uart_name);
rt_err_t mhz19b_read_co2(struct mhz19b_device *dev, rt_uint16_t *ppm);
rt_err_t mhz19b_set_abc(struct mhz19b_device *dev, rt_uint8_t enable);
rt_err_t mhz19b_calibrate_zero(struct mhz19b_device *dev);

#ifdef __cplusplus
}
#endif
#endif
