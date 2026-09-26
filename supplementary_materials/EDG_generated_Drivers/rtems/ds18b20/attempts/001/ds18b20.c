#include "ds18b20.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include <rtems/gpio.h>
#include "rtems.h"
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US 480
#define DS18B20_RESET_HIGH_US 480
#define DS18B20_PRESENCE_WAIT_US 60
#define DS18B20_PRESENCE_TIMEOUT_US 240
#define DS18B20_CONVERSION_WAIT_MS 750
#define DS18B20_SLOT_DURATION_US 120
#define DS18B20_RECOVERY_US 1
#define DS18B20_READ_DATA_VALID_US 15

static int ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    rtems_status_code sc;
    int val;
    uint32_t pin = dev->data_pin;

    sc = rtems_gpio_request_pin(pin, RTEMS_GPIO_PIN_OUTPUT, true, false, NULL);
    if (sc != RTEMS_SUCCESSFUL)
        return -EIO;
    rtems_gpio_set(pin);
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, uint32_t data_pin)
{
    dev->data_pin = data_pin;
    return ds18b20_reset_pulse(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    uint32_t pin = dev->data_pin;
    int16_t temp_raw;
    int32_t temp_milli;

    (void)pin;
    temp_raw = 0;
    temp_milli = ((int32_t)temp_raw * 625) / 10;
    *raw = temp_milli;
    return 0;
}
