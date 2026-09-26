# Raw RTOS/Bus Pack: rt-thread / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rt-thread/components/drivers/include/drivers/dev_pin.h`

```c
/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-01-20     Bernard      the first version
 * 2017-10-20      ZYH          add mode open drain and input pull down
 */

#ifndef DEV_PIN_H__
#define DEV_PIN_H__

#include <rtthread.h>

/**
 * @defgroup    group_drivers_pin Pin
 * @brief       Pin driver api
 * @ingroup     group_device_driver
 *
 * <b>Example</b>
 * @code {.c}
 * #include <rtthread.h>
 * #include <rtdevice.h>
 *
 *
 * #ifndef BEEP_PIN_NUM
 *     #define BEEP_PIN_NUM            35  // PB0
 * #endif
 * #ifndef KEY0_PIN_NUM
 *     #define KEY0_PIN_NUM            55  // PD8
 * #endif
 * #ifndef KEY1_PIN_NUM
 *     #define KEY1_PIN_NUM            56  // PD9
 * #endif
 *
 * void beep_on(void *args)
 * {
 *     rt_kprintf("turn on beep!\n");
 *
 *     rt_pin_write(BEEP_PIN_NUM, PIN_HIGH);
 * }
 *
 * void beep_off(void *args)
 * {
 *     rt_kprintf("turn off beep!\n");
 *
 *     rt_pin_write(BEEP_PIN_NUM, PIN_LOW);
 * }
 *
 * static void pin_beep_sample(void)
 * {
 *     rt_pin_mode(BEEP_PIN_NUM, PIN_MODE_OUTPUT);
 *     rt_pin_write(BEEP_PIN_NUM, PIN_LOW);
 *
 *     rt_pin_mode(KEY0_PIN_NUM, PIN_MODE_INPUT_PULLUP);
 *     rt_pin_attach_irq(KEY0_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
 *     rt_pin_irq_enable(KEY0_PIN_NUM, PIN_IRQ_ENABLE);
 *
 *
 *     rt_pin_mode(KEY1_PIN_NUM, PIN_MODE_INPUT_PULLUP);
 *     rt_pin_attach_irq(KEY1_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_off, RT_NULL);
 *     rt_pin_irq_enable(KEY1_PIN_NUM, PIN_IRQ_ENABLE);
 * }
 *
 * MSH_CMD_EXPORT(pin_beep_sample, pin beep sample);
 * @endcode
 */

/*!
 * @addtogroup group_drivers_pin
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef RT_USING_DM
#include <drivers/pic.h>

struct rt_pin_irqchip
{
    struct rt_pic parent;

    int irq;
    rt_base_t pin_range[2];
};

struct rt_pin_irq_hdr;
#endif /* RT_USING_DM */

/**
 * @brief pin device structure
 */
struct rt_device_pin
{
    struct rt_device parent;
#ifdef RT_USING_DM
    /* MUST keep the order member after parent */
    struct rt_pin_irqchip irqchip;
    /* Fill by DM */
    rt_base_t pin_start;
    rt_size_t pin_nr;
    rt_list_t list;
    struct rt_pin_irq_hdr *legacy_isr;
#endif /* RT_USING_DM */
    const struct rt_pin_ops *ops;
};

#define PIN_NONE                (-RT_EEMPTY)

#define PIN_LOW                 0x00 /*!< low level */
#define PIN_HIGH                0x01 /*!< high level */

#define PIN_MODE_OUTPUT         0x00 /*!< output mode */
#define PIN_MODE_INPUT          0x01 /*!< input mode */
#define PIN_MODE_INPUT_PULLUP   0x02 /*!< input mode with pull-up */
#define PIN_MODE_INPUT_PULLDOWN 0x03 /*!< input mode with pull-down */
#define PIN_MODE_OUTPUT_OD      0x04 /*!< output mode with open-drain */

#ifdef RT_USING_PINCTRL
enum
{
    PIN_CONFIG_BIAS_BUS_HOLD,
    PIN_CONFIG_BIAS_DISABLE,
    PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
    PIN_CONFIG_BIAS_PULL_DOWN,
    PIN_CONFIG_BIAS_PULL_PIN_DEFAULT,
    PIN_CONFIG_BIAS_PULL_UP,
    PIN_CONFIG_DRIVE_OPEN_DRAIN,
    PIN_CONFIG_DRIVE_OPEN_SOURCE,
    PIN_CONFIG_DRIVE_PUSH_PULL,
    PIN_CONFIG_DRIVE_STRENGTH,
    PIN_CONFIG_DRIVE_STRENGTH_UA,
    PIN_CONFIG_INPUT_DEBOUNCE,
    PIN_CONFIG_INPUT_ENABLE,
    PIN_CONFIG_INPUT_SCHMITT,
    PIN_CONFIG_INPUT_SCHMITT_ENABLE,
    PIN_CONFIG_MODE_LOW_POWER,
    PIN_CONFIG_MODE_PWM,
    PIN_CONFIG_OUTPUT,
    PIN_CONFIG_OUTPUT_ENABLE,
    PIN_CONFIG_OUTPUT_IMPEDANCE_OHMS,
    PIN_CONFIG_PERSIST_STATE,
    PIN_CONFIG_POWER_SOURCE,
    PIN_CONFIG_SKEW_DELAY,
    PIN_CONFIG_SLEEP_HARDWARE_STATE,
    PIN_CONFIG_SLEW_RATE,
    PIN_CONFIG_END = 0x7f,
    PIN_CONFIG_MAX = 0xff,
};
#endif /* RT_USING_PINCTRL */

#define PIN_IRQ_MODE_RISING             0x00 /*!< rising edge trigger */
#define PIN_IRQ_MODE_FALLING            0x01 /*!< falling edge trigger */
#define PIN_IRQ_MODE_RISING_FALLING     0x02 /*!< rising and falling edge trigger */
#define PIN_IRQ_MODE_HIGH_LEVEL         0x03 /*!< high level trigger */
#define PIN_IRQ_MODE_LOW_LEVEL          0x04 /*!< low level trigger */

#define PIN_IRQ_DISABLE                 0x00 /*!< disable irq */
#define PIN_IRQ_ENABLE                  0x01 /*!< enable irq */

#define PIN_IRQ_PIN_NONE                PIN_NONE /*!< no pin irq */

/**
 * @brief pin mode structure
 */
struct rt_device_pin_mode
{
    rt_base_t pin;
    rt_uint8_t mode; /* e.g. PIN_MODE_OUTPUT */
};

/**
 * @brief pin value structure
 */
struct rt_device_pin_value
{
    rt_base_t pin;
    rt_uint8_t value; /* PIN_LOW or PIN_HIGH */
};

/**
 * @brief pin irq structure
 */
struct rt_pin_irq_hdr
{
    rt_base_t        pin;
    rt_uint8_t       mode; /* e.g. PIN_IRQ_MODE_RISING */
    void (*hdr)(void *args);
    void             *args;
};

#ifdef RT_USING_PINCTRL
/**
 * @brief pin control configure structure
 */
struct rt_pin_ctrl_conf_params
{
    const char *propname;
    rt_uint32_t param;
    rt_uint32_t default_value;
};
#endif /* RT_USING_PINCTRL */

/**
 * @brief pin device operations
 */
struct rt_pin_ops
{
    void (*pin_mode)(struct rt_device *device, rt_base_t pin, rt_uint8_t mode);
    void (*pin_write)(struct rt_device *device, rt_base_t pin, rt_uint8_t value);
    rt_ssize_t  (*pin_read)(struct rt_device *device, rt_base_t pin);
    rt_err_t (*pin_attach_irq)(struct rt_device *device, rt_base_t pin,
            rt_uint8_t mode, void (*hdr)(void *args), void *args);
    rt_err_t (*pin_detach_irq)(struct rt_device *device, rt_base_t pin);
    rt_err_t (*pin_irq_enable)(struct rt_device *device, rt_base_t pin, rt_uint8_t enabled);
    rt_base_t (*pin_get)(const char *name);
    rt_err_t (*pin_debounce)(struct rt_device *device, rt_base_t pin, rt_uint32_t debounce);
#ifdef RT_USING_DM
    rt_err_t (*pin_irq_mode)(struct rt_device *device, rt_base_t pin, rt_uint8_t mode);
    rt_ssize_t (*pin_parse)(struct rt_device *device, struct rt_ofw_cell_args *args, rt_uint32_t *flags);
#endif
#ifdef RT_USING_PINCTRL
    rt_err_t (*pin_ctrl_confs_apply)(struct rt_device *device, void *fw_conf_np);
    rt_err_t (*pin_ctrl_gpio_request)(struct rt_device *device, rt_base_t gpio, rt_uint32_t flags);
#endif /* RT_USING_PINCTRL */
};

/**
 * @brief register a pin device
 * @param name the name of pin device
 * @param ops the operations of pin device
 * @param user_data the user data of pin device
 * @return int error code
 */
int rt_device_pin_register(const char *name, const struct rt_pin_ops *ops, void *user_data);

/**
 * @brief set pin mode
 * @param pin the pin number
 * @param mode the pin mode
 */
void rt_pin_mode(rt_base_t pin, rt_uint8_t mode);

/**
 * @brief write pin value
 * @param pin the pin number
 * @param value the pin value
 */
void rt_pin_write(rt_base_t pin, rt_ssize_t value);

/**
 * @brief read pin value
 * @param pin the pin number
 * @return rt_ssize_t the pin value
 */
rt_ssize_t rt_pin_read(rt_base_t pin);

/**
 * @brief get pin number by name
 * @param name the pin name
 * @return rt_base_t the pin number
 */
rt_base_t rt_pin_get(const char *name);

/**
 * @brief bind the pin interrupt callback function
 * @param pin the pin number
 * @param mode the irq mode
 * @param hdr the irq callback function
 * @param args the argument of the callback function
 * @return rt_err_t error code
 */
rt_err_t rt_pin_attach_irq(rt_base_t pin, rt_uint8_t mode,
                           void (*hdr)(void *args), void  *args);

/**
 * @brief detach the pin interrupt callback function
 * @param pin the pin number
 * @return rt_err_t error code
 */
rt_err_t rt_pin_detach_irq(rt_base_t pin);

/**
 * @brief enable or disable the pin interrupt
 * @param pin the pin number
 * @param enabled PIN_IRQ_ENABLE or PIN_IRQ_DISABLE
 * @return rt_err_t error code
 */
rt_err_t rt_pin_irq_enable(rt_base_t pin, rt_uint8_t enabled);

/**
 * @brief set the pin's debounce time
 * @param pin the pin number
 * @param debounce time
 * @return rt_err_t error code
 */
rt_err_t rt_pin_debounce(rt_base_t pin, rt_uint32_t debounce);

#ifdef RT_USING_DM
rt_ssize_t rt_pin_get_named_pin(struct rt_device *dev, const char *propname, int index,
        rt_uint8_t *out_mode, rt_uint8_t *out_value);
rt_ssize_t rt_pin_get_named_pin_count(struct rt_device *dev, const char *propname);

#ifdef RT_USING_OFW
rt_ssize_t rt_ofw_get_named_pin(struct rt_ofw_node *np, const char *propname, int index,
        rt_uint8_t *out_mode, rt_uint8_t *out_value);
rt_ssize_t rt_ofw_get_named_pin_count(struct rt_ofw_node *np, const char *propname);
#endif
#endif /* RT_USING_DM */

#ifdef RT_USING_PINCTRL
rt_ssize_t rt_pin_ctrl_confs_lookup(struct rt_device *device, const char *name);
rt_err_t rt_pin_ctrl_confs_apply(struct rt_device *device, int index);
rt_err_t rt_pin_ctrl_confs_apply_by_name(struct rt_device *device, const char *name);
#endif /* RT_USING_PINCTRL */

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif
```

## Source: `data/rtos/rt-thread/components/drivers/include/rtdevice.h`

```c
/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-08     bernard      first version.
 * 2014-07-12     bernard      Add workqueue implementation.
 */

#ifndef __RT_DEVICE_H__
#define __RT_DEVICE_H__

#include <rtdef.h>
#include <rtthread.h>
#include <drivers/core/driver.h>
#include <drivers/core/bus.h>

#include <drivers/classes/block.h>
#include <drivers/classes/char.h>
#include <drivers/classes/graphic.h>
#include <drivers/classes/mtd.h>
#include <drivers/classes/net.h>

#include "ipc/ringbuffer.h"
#include "ipc/completion.h"
#include "ipc/dataqueue.h"
#include "ipc/workqueue.h"
#include "ipc/condvar.h"
#include "ipc/waitqueue.h"
#include "ipc/pipe.h"
#include "ipc/poll.h"
#include "ipc/ringblk_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RT_DEVICE(device)            ((rt_device_t)device)

#ifdef RT_USING_DM
#include "drivers/core/dm.h"
#include "drivers/core/numa.h"
#include "drivers/core/power.h"
#include "drivers/core/power_domain.h"
#include "drivers/platform.h"

#ifdef RT_USING_GRAPHIC
#include "drivers/graphic.h"
#ifdef RT_GRAPHIC_BACKLIGHT
#include "drivers/backlight.h"
#endif /* RT_GRAPHIC_BACKLIGHT */
#endif /* RT_USING_GRAPHIC */

#ifdef RT_USING_ATA
#ifdef RT_ATA_AHCI
#include "drivers/ahci.h"
#endif /* RT_ATA_AHCI */
#endif /* RT_USING_ATA */

#ifdef RT_USING_LED
#include "drivers/led.h"
#endif /* RT_USING_LED */

#ifdef RT_USING_INPUT
#include "drivers/input.h"
#ifdef RT_INPUT_UAPI
#include "drivers/input_uapi.h"
#endif
#endif /* RT_USING_INPUT */

#ifdef RT_USING_MBOX
#include "drivers/mailbox.h"
#endif /* RT_USING_MBOX */

#ifdef RT_USING_HWSPINLOCK
#include "drivers/hwspinlock.h"
#endif /* RT_USING_HWSPINLOCK */

#ifdef RT_USING_BLK
#include "drivers/blk.h"
#endif /* RT_USING_BLK */

#ifdef RT_USING_DMA
#include "drivers/dma.h"
#endif /* RT_USING_DMA */

#include "drivers/iio.h"

#ifdef RT_USING_NVME
#include "drivers/nvme.h"
#endif /* RT_USING_NVME */

#ifdef RT_USING_OFW
#include "drivers/ofw.h"
#include "drivers/ofw_fdt.h"
#include "drivers/ofw_io.h"
#include "drivers/ofw_irq.h"
#include "drivers/ofw_raw.h"
#endif /* RT_USING_OFW */

#ifdef RT_USING_PHYE
#include "drivers/phye.h"
#endif /* RT_USING_PHYE */

#ifdef RT_USING_PIC
#include "drivers/pic.h"
#endif /* RT_USING_PIC */

#ifdef RT_USING_PCI
#include "drivers/pci.h"
#ifdef RT_PCI_MSI
#include "drivers/pci_msi.h"
#endif /* RT_PCI_MSI */
#ifdef RT_PCI_ENDPOINT
#include "drivers/pci_endpoint.h"
#endif /* RT_PCI_ENDPOINT */
#endif /* RT_USING_PCI */

#ifdef RT_USING_REGULATOR
#include "drivers/regulator.h"
#endif /* RT_USING_REGULATOR */

#ifdef RT_USING_RESET
#include "drivers/reset.h"
#endif /* RT_USING_RESET */

#ifdef RT_USING_SCSI
#include "drivers/scsi.h"
#endif /* RT_USING_SCSI */

#ifdef RT_MFD_SYSCON
#include "drivers/syscon.h"
#endif /* RT_MFD_SYSCON */

#ifdef RT_USING_THERMAL
#include "drivers/thermal.h"
#endif /* RT_USING_THERMAL */

#ifdef RT_USING_FIRMWARE
#ifdef RT_FIRMWARE_ARM_SCMI
#include "drivers/scmi.h"
#endif /* RT_FIRMWARE_ARM_SCMI */
#endif /* RT_USING_FIRMWARE */

#ifdef RT_USING_HWCACHE
#include "drivers/hwcache.h"
#endif /* RT_USING_HWCACHE */

#ifdef RT_USING_POWER_SUPPLY
#include "drivers/power_supply.h"
#endif /* RT_USING_POWER_SUPPLY */

#ifdef RT_USING_NVMEM
#include "drivers/nvmem.h"
#endif /* RT_USING_NVMEM */
#endif /* RT_USING_DM */

#ifdef RT_USING_RTC
#include "drivers/dev_rtc.h"
#ifdef RT_USING_ALARM
#include "drivers/dev_alarm.h"
#endif /* RT_USING_ALARM */
#endif /* RT_USING_RTC */

#ifdef RT_USING_SPI
#include "drivers/dev_spi.h"
#endif /* RT_USING_SPI */

#ifdef RT_USING_MTD_NOR
#include "drivers/mtd_nor.h"
#endif /* RT_USING_MTD_NOR */

#ifdef RT_USING_MTD_NAND
#include "drivers/mtd_nand.h"
#endif /* RT_USING_MTD_NAND */

#ifdef RT_USING_USB_DEVICE
#include "drivers/usb_device.h"
#endif /* RT_USING_USB_DEVICE */

#ifdef RT_USING_USB_HOST
#include "drivers/usb_host.h"
#endif /* RT_USING_USB_HOST */

#ifdef RT_USING_SERIAL
#ifdef RT_USING_SERIAL_V2
#include "drivers/dev_serial_v2.h"
#else
#include "drivers/dev_serial.h"
#endif /* RT_USING_SERIAL_V2 */
#ifdef RT_USING_SERIAL_BYPASS
#include "drivers/serial_bypass.h"
#endif /* RT_USING_SERIAL_BYPASS */
#endif /* RT_USING_SERIAL */

#ifdef RT_USING_I2C
#include "drivers/dev_i2c.h"

#ifdef RT_USING_I2C_BITOPS
#include "drivers/dev_i2c_bit_ops.h"
#endif /* RT_USING_I2C_BITOPS */

#ifdef RT_USING_DM
#include "drivers/dev_i2c_dm.h"
#endif /* RT_USING_DM */
#endif /* RT_USING_I2C */

#if defined(RT_USING_PHY) || defined(RT_USING_PHY_V2)
#include "drivers/phy.h"
#endif /* RT_USING_PHY || RT_USING_PHY_V2 */

#ifdef RT_USING_SDIO
#include "drivers/dev_mmcsd_core.h"
#include "drivers/dev_sd.h"
#include "drivers/dev_sdio.h"
#if defined(RT_USING_DM) && defined(RT_USING_SDHCI)
#include "drivers/dev_sdhci.h"
#include "drivers/dev_sdhci_host.h"
#endif /* RT_USING_DM && RT_USING_SDHCI */
#endif /* RT_USING_SDIO */


#ifdef RT_USING_WDT
#include "drivers/dev_watchdog.h"
#endif /* RT_USING_WDT */

#ifdef RT_USING_PIN
#include "drivers/dev_pin.h"
#endif /* RT_USING_PIN */

#ifdef RT_USING_SENSOR
#ifdef RT_USING_SENSOR_V2
#include "drivers/sensor_v2.h"
#else
#include "drivers/sensor.h"
#endif /* RT_USING_SENSOR_V2 */
#endif /* RT_USING_SENSOR */

#ifdef RT_USING_CAN
#include "drivers/dev_can.h"
#endif /* RT_USING_CAN */

#ifdef RT_USING_CLOCK_TIME
#include "drivers/clock_time.h"
#endif /* RT_USING_CLOCK_TIME */

#ifdef RT_USING_AUDIO
#include "drivers/dev_audio.h"
#endif /* RT_USING_AUDIO */

#ifdef RT_USING_ADC
#include "drivers/adc.h"
#endif /* RT_USING_ADC */

#ifdef RT_USING_DAC
#include "drivers/dac.h"
#endif /* RT_USING_DAC */

#ifdef RT_USING_PWM
#include "drivers/dev_pwm.h"
#endif /* RT_USING_PWM */

#ifdef RT_USING_PM
#include "drivers/pm.h"
#endif /* RT_USING_PM */

#ifdef RT_USING_WIFI
#include "drivers/wlan.h"
#endif /* RT_USING_WIFI */

#ifdef MTD_USING_NOR
#include "drivers/mtdnor.h"
#endif /* MTD_USING_NOR */

#ifdef MTD_USING_NAND
#include "drivers/mtdnand.h"
#endif /* MTD_USING_NAND */

#ifdef RT_USING_HWCRYPTO
#include "drivers/crypto.h"
#endif /* RT_USING_HWCRYPTO */

#ifdef RT_USING_PULSE_ENCODER
#include "drivers/pulse_encoder.h"
#endif /* RT_USING_PULSE_ENCODER */

#ifdef RT_USING_INPUT_CAPTURE
#include "drivers/rt_inputcapture.h"
#endif /* RT_USING_INPUT_CAPTURE */

#ifdef RT_USING_TOUCH
#include "drivers/dev_touch.h"
#endif

#ifdef RT_USING_DEV_BUS
#include "drivers/rt_dev_bus.h"
#endif

#ifdef RT_USING_LCD
#include "drivers/lcd.h"
#endif

#ifdef RT_USING_CLK
#include "drivers/clk.h"
#endif /* RT_USING_CLK */

#ifdef __cplusplus
}
#endif

#endif /* __RT_DEVICE_H__ */
```

## Source: `data/rtos/rt-thread/components/drivers/pin/dev_pin.c`

```c
/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-01-20     Bernard      the first version
 * 2021-02-06     Meco Man     fix RT_ENOSYS code in negative
 * 2022-04-29     WangQiang    add pin operate command in MSH
 */

#include <drivers/dev_pin.h>

static struct rt_device_pin _hw_pin;
static rt_ssize_t _pin_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct rt_device_pin_value *value;
    struct rt_device_pin *pin = (struct rt_device_pin *)dev;

    /* check parameters */
    RT_ASSERT(pin != RT_NULL);

    value = (struct rt_device_pin_value *)buffer;
    if (value == RT_NULL || size != sizeof(*value))
        return 0;

    value->value = pin->ops->pin_read(dev, value->pin);
    return size;
}

static rt_ssize_t _pin_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct rt_device_pin_value *value;
    struct rt_device_pin *pin = (struct rt_device_pin *)dev;

    /* check parameters */
    RT_ASSERT(pin != RT_NULL);

    value = (struct rt_device_pin_value *)buffer;
    if (value == RT_NULL || size != sizeof(*value))
        return 0;

    pin->ops->pin_write(dev, (rt_base_t)value->pin, (rt_base_t)value->value);

    return size;
}

static rt_err_t _pin_control(rt_device_t dev, int cmd, void *args)
{
    struct rt_device_pin_mode *mode;
    struct rt_device_pin *pin = (struct rt_device_pin *)dev;

    /* check parameters */
    RT_ASSERT(pin != RT_NULL);

    mode = (struct rt_device_pin_mode *)args;
    if (mode == RT_NULL)
        return -RT_ERROR;

    pin->ops->pin_mode(dev, (rt_base_t)mode->pin, (rt_base_t)mode->mode);

    return 0;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops pin_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _pin_read,
    _pin_write,
    _pin_control
};
#endif

int rt_device_pin_register(const char *name, const struct rt_pin_ops *ops, void *user_data)
{
    _hw_pin.parent.type         = RT_Device_Class_Pin;
    _hw_pin.parent.rx_indicate  = RT_NULL;
    _hw_pin.parent.tx_complete  = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    _hw_pin.parent.ops          = &pin_ops;
#else
    _hw_pin.parent.init         = RT_NULL;
    _hw_pin.parent.open         = RT_NULL;
    _hw_pin.parent.close        = RT_NULL;
    _hw_pin.parent.read         = _pin_read;
    _hw_pin.parent.write        = _pin_write;
    _hw_pin.parent.control      = _pin_control;
#endif

    _hw_pin.ops                 = ops;
    _hw_pin.parent.user_data    = user_data;

    /* register a character device */
    rt_device_register(&_hw_pin.parent, name, RT_DEVICE_FLAG_RDWR);

    return 0;
}

rt_err_t rt_pin_attach_irq(rt_base_t pin, rt_uint8_t mode,
                           void (*hdr)(void *args), void *args)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    if (_hw_pin.ops->pin_attach_irq)
    {
        return _hw_pin.ops->pin_attach_irq(&_hw_pin.parent, pin, mode, hdr, args);
    }
    return -RT_ENOSYS;
}

rt_err_t rt_pin_detach_irq(rt_base_t pin)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    if (_hw_pin.ops->pin_detach_irq)
    {
        return _hw_pin.ops->pin_detach_irq(&_hw_pin.parent, pin);
    }
    return -RT_ENOSYS;
}

rt_err_t rt_pin_irq_enable(rt_base_t pin, rt_uint8_t enabled)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    if (_hw_pin.ops->pin_irq_enable)
    {
        return _hw_pin.ops->pin_irq_enable(&_hw_pin.parent, pin, enabled);
    }
    return -RT_ENOSYS;
}

rt_err_t rt_pin_debounce(rt_base_t pin, rt_uint32_t debounce)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    if (_hw_pin.ops->pin_debounce)
    {
        return _hw_pin.ops->pin_debounce(&_hw_pin.parent, pin, debounce);
    }
    return -RT_ENOSYS;
}

/* RT-Thread Hardware PIN APIs */
void rt_pin_mode(rt_base_t pin, rt_uint8_t mode)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    _hw_pin.ops->pin_mode(&_hw_pin.parent, pin, mode);
}

void rt_pin_write(rt_base_t pin, rt_ssize_t value)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    _hw_pin.ops->pin_write(&_hw_pin.parent, pin, value);
}

rt_ssize_t rt_pin_read(rt_base_t pin)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);
    return _hw_pin.ops->pin_read(&_hw_pin.parent, pin);
}

/* Get pin number by name, such as PA.0, P0.12 */
rt_base_t rt_pin_get(const char *name)
{
    RT_ASSERT(_hw_pin.ops != RT_NULL);

    if (_hw_pin.ops->pin_get == RT_NULL)
    {
        return -RT_ENOSYS;
    }
    return _hw_pin.ops->pin_get(name);
}

#ifdef RT_USING_FINSH
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <finsh.h>
#include <msh_parse.h>

/*
 * convert function for port name
 */
static rt_base_t _pin_cmd_conv(const char *name)
{
    return rt_pin_get(name);
}

static void _pin_cmd_print_usage(void)
{
    rt_kprintf("pin [option] GPIO\n");
    rt_kprintf("     num:      get pin number from hardware pin\n");
    rt_kprintf("     mode:     set pin mode to output/input/input_pullup/input_pulldown/output_od\n");
    rt_kprintf("               e.g. MSH >pin mode GPIO output\n");
    rt_kprintf("     read:     read pin level of hardware pin\n");
    rt_kprintf("               e.g. MSH >pin read GPIO\n");
    rt_kprintf("     write:    write pin level(high/low or on/off) to hardware pin\n");
    rt_kprintf("               e.g. MSH >pin write GPIO high\n");
    rt_kprintf("     help:     this help list\n");
    rt_kprintf("GPIO e.g.:");
    rt_pin_get(" ");
}

/* e.g. MSH >pin num PA.16 */
static void _pin_cmd_get(int argc, char *argv[])
{
    rt_base_t pin;
    if (argc < 3)
    {
        _pin_cmd_print_usage();
        return;
    }
    pin = _pin_cmd_conv(argv[2]);
    if (pin < 0)
    {
        rt_kprintf("Parameter invalid : %s!\n", argv[2]);
        _pin_cmd_print_usage();
        return ;
    }
    rt_kprintf("%s : %d\n", argv[2], pin);
}

/* e.g. MSH >pin mode PA.16 output */
static void _pin_cmd_mode(int argc, char *argv[])
{
    rt_base_t pin;
    rt_base_t mode;
    if (argc < 4)
    {
        _pin_cmd_print_usage();
        return;
    }
    if (!msh_isint(argv[2]))
    {
        pin = _pin_cmd_conv(argv[2]);
        if (pin < 0)
        {
            rt_kprintf("Parameter invalid : %s!\n", argv[2]);
            _pin_cmd_print_usage();
            return;
        }
    }
    else
    {
        pin = atoi(argv[2]);
    }
    if (0 == rt_strcmp("output", argv[3]))
    {
        mode = PIN_MODE_OUTPUT;
    }
    else if (0 == rt_strcmp("input", argv[3]))
    {
        mode = PIN_MODE_INPUT;
    }
    else if (0 == rt_strcmp("input_pullup", argv[3]))
    {
        mode = PIN_MODE_INPUT_PULLUP;
    }
    else if (0 == rt_strcmp("input_pulldown", argv[3]))
    {
        mode = PIN_MODE_INPUT_PULLDOWN;
    }
    else if (0 == rt_strcmp("output_od", argv[3]))
    {
        mode = PIN_MODE_OUTPUT_OD;
    }
    else
    {
        _pin_cmd_print_usage();
        return;
    }

    rt_pin_mode(pin, mode);
}

/* e.g. MSH >pin read PA.16 */
static void _pin_cmd_read(int argc, char *argv[])
{
    rt_base_t pin;
    rt_uint8_t value;
    if (argc < 3)
    {
        _pin_cmd_print_usage();
        return;
    }
    if (!msh_isint(argv[2]))
    {
        pin = _pin_cmd_conv(argv[2]);
        if (pin < 0)
        {
            rt_kprintf("Parameter invalid : %s!\n", argv[2]);
            _pin_cmd_print_usage();
            return;
        }
    }
    else
    {
        pin = atoi(argv[2]);
    }
    value = rt_pin_read(pin);
    if (value == PIN_HIGH)
    {
        rt_kprintf("pin[%d] = high\n", pin);
    }
    else
    {
        rt_kprintf("pin[%d] = low\n", pin);
    }
}

/* e.g. MSH >pin write PA.16 high */
static void _pin_cmd_write(int argc, char *argv[])
{
    rt_base_t pin;
    rt_uint8_t value;
    if (argc < 4)
    {
        _pin_cmd_print_usage();
        return;
    }
    if (!msh_isint(argv[2]))
    {
        pin = _pin_cmd_conv(argv[2]);
        if (pin < 0)
        {
            rt_kprintf("Parameter invalid : %s!\n", argv[2]);
            _pin_cmd_print_usage();
            return;
        }
    }
    else
    {
        pin = atoi(argv[2]);
    }
    if ((0 == rt_strcmp("high", argv[3])) || (0 == rt_strcmp("on", argv[3])))
    {
        value = PIN_HIGH;
    }
    else if ((0 == rt_strcmp("low", argv[3])) || (0 == rt_strcmp("off", argv[3])))
    {
        value = PIN_LOW;
    }
    else
    {
        _pin_cmd_print_usage();
        return;
    }
    rt_pin_write(pin, value);
}

static void _pin_cmd(int argc, char *argv[])
{
    if (argc < 3)
    {
        _pin_cmd_print_usage();
        return ;
    }
    if (0 == rt_strcmp("num", argv[1]))
    {
        _pin_cmd_get(argc, argv);
    }
    else if (0 == rt_strcmp("mode", argv[1]))
    {
        _pin_cmd_mode(argc, argv);
    }
    else if (0 == rt_strcmp("read", argv[1]))
    {
        _pin_cmd_read(argc, argv);
    }
    else if (0 == rt_strcmp("write", argv[1]))
    {
        _pin_cmd_write(argc, argv);
    }
    else
    {
        _pin_cmd_print_usage();
        return;
    }
}
MSH_CMD_EXPORT_ALIAS(_pin_cmd, pin, pin [option]);
#endif /* RT_USING_FINSH */
```

## Source: `data/rtos/rt-thread/include/rtthread.h`

```c

/* excerpt lines 1-60 */
/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-18     Bernard      the first version
 * 2006-04-26     Bernard      add semaphore APIs
 * 2006-08-10     Bernard      add version information
 * 2007-01-28     Bernard      rename RT_OBJECT_Class_Static to RT_Object_Class_Static
 * 2007-03-03     Bernard      clean up the definitions to rtdef.h
 * 2010-04-11     yi.qiu       add module feature
 * 2013-06-24     Bernard      add rt_kprintf re-define when not use RT_USING_CONSOLE.
 * 2016-08-09     ArdaFu       add new thread and interrupt hook.
 * 2018-11-22     Jesven       add all cpu's lock and ipi handler
 * 2021-02-28     Meco Man     add RT_KSERVICE_USING_STDLIB
 * 2021-11-14     Meco Man     add rtlegacy.h for compatibility
 * 2022-06-04     Meco Man     remove strnlen
 * 2023-05-20     Bernard      add rtatomic.h header file to included files.
 * 2023-06-30     ChuShicheng  move debug check from the rtdebug.h
 * 2023-10-16     Shell        Support a new backtrace framework
 * 2023-12-10     xqyjlj       fix spinlock in up
 * 2024-01-25     Shell        Add rt_susp_list for IPC primitives
 * 2024-03-10     Meco Man     move std libc related functions to rtklibc
 */

#ifndef __RT_THREAD_H__
#define __RT_THREAD_H__

#include <rtconfig.h>
#include <rtdef.h>
#include <rtservice.h>
#include <rtm.h>
#include <rtatomic.h>
#include <rtklibc.h>
#ifdef RT_USING_LEGACY
#include <rtlegacy.h>
#endif
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif /* RT_USING_FINSH */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
int entry(void);
#endif

/*
 * kernel object interface
 */
struct rt_object_information *
rt_object_get_information(enum rt_object_class_type type);
int rt_object_get_length(enum rt_object_class_type type);
int rt_object_get_pointers(enum rt_object_class_type type, rt_object_t *pointers, int maxlen);

void rt_object_init(struct rt_object         *object,

/* excerpt lines 77-193 */
#ifdef RT_USING_HOOK
void rt_object_attach_sethook(void (*hook)(struct rt_object *object));
void rt_object_detach_sethook(void (*hook)(struct rt_object *object));
void rt_object_trytake_sethook(void (*hook)(struct rt_object *object));
void rt_object_take_sethook(void (*hook)(struct rt_object *object));
void rt_object_put_sethook(void (*hook)(struct rt_object *object));
#endif /* RT_USING_HOOK */

/**
 * @addtogroup group_clock_management
 * @{
 */

/*
 * clock & timer interface
 */
rt_tick_t rt_tick_get(void);
rt_tick_t rt_tick_get_delta(rt_tick_t base);
void rt_tick_set(rt_tick_t tick);
void rt_tick_increase(void);
void rt_tick_increase_tick(rt_tick_t tick);
rt_tick_t  rt_tick_from_millisecond(rt_int32_t ms);
rt_tick_t rt_tick_get_millisecond(void);
#ifdef RT_USING_HOOK
void rt_tick_sethook(void (*hook)(void));
#endif /* RT_USING_HOOK */

void rt_system_timer_init(void);
void rt_system_timer_thread_init(void);

void rt_timer_init(rt_timer_t  timer,
                   const char *name,
                   void (*timeout)(void *parameter),
                   void       *parameter,
                   rt_tick_t   time,
                   rt_uint8_t  flag);
rt_err_t rt_timer_detach(rt_timer_t timer);
#ifdef RT_USING_HEAP
rt_timer_t rt_timer_create(const char *name,
                           void (*timeout)(void *parameter),
                           void       *parameter,
                           rt_tick_t   time,
                           rt_uint8_t  flag);
rt_err_t rt_timer_delete(rt_timer_t timer);
#endif /* RT_USING_HEAP */
rt_err_t rt_timer_start(rt_timer_t timer);
rt_err_t rt_timer_stop(rt_timer_t timer);
rt_err_t rt_timer_control(rt_timer_t timer, int cmd, void *arg);
rt_tick_t rt_timer_next_timeout_tick(void);
void rt_timer_check(void);
#ifdef RT_USING_HOOK
void rt_timer_enter_sethook(void (*hook)(struct rt_timer *timer));
void rt_timer_exit_sethook(void (*hook)(struct rt_timer *timer));
#endif /* RT_USING_HOOK */

/**@}*/

/*
 * thread interface
 */
rt_err_t rt_thread_init(struct rt_thread *thread,
                        const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
                        rt_uint8_t        priority,
                        rt_uint32_t       tick);
rt_err_t rt_thread_detach(rt_thread_t thread);
#ifdef RT_USING_HEAP
rt_thread_t rt_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void       *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t  priority,
                             rt_uint32_t tick);
rt_err_t rt_thread_delete(rt_thread_t thread);
#endif /* RT_USING_HEAP */
rt_err_t rt_thread_close(rt_thread_t thread);
rt_thread_t rt_thread_self(void);
rt_thread_t rt_thread_find(char *name);
rt_err_t rt_thread_startup(rt_thread_t thread);
rt_err_t rt_thread_yield(void);
rt_err_t rt_thread_delay(rt_tick_t tick);
rt_err_t rt_thread_delay_until(rt_tick_t *tick, rt_tick_t inc_tick);
rt_err_t rt_thread_mdelay(rt_int32_t ms);
rt_err_t rt_thread_control(rt_thread_t thread, int cmd, void *arg);
rt_err_t rt_thread_suspend(rt_thread_t thread);
rt_err_t rt_thread_suspend_with_flag(rt_thread_t thread, int suspend_flag);
rt_err_t rt_thread_resume(rt_thread_t thread);
#ifdef RT_USING_SMART
rt_err_t rt_thread_wakeup(rt_thread_t thread);
void rt_thread_wakeup_set(struct rt_thread *thread, rt_wakeup_func_t func, void* user_data);
#endif /* RT_USING_SMART */
rt_err_t rt_thread_get_name(rt_thread_t thread, char *name, rt_uint8_t name_size);
#ifdef RT_USING_CPU_USAGE_TRACER
rt_uint8_t rt_thread_get_usage(rt_thread_t thread);
#endif /* RT_USING_CPU_USAGE_TRACER */
#ifdef RT_USING_SIGNALS
void rt_thread_alloc_sig(rt_thread_t tid);
void rt_thread_free_sig(rt_thread_t tid);
int  rt_thread_kill(rt_thread_t tid, int sig);
#endif /* RT_USING_SIGNALS */
#ifdef RT_USING_HOOK
void rt_thread_suspend_sethook(void (*hook)(rt_thread_t thread));
void rt_thread_resume_sethook (void (*hook)(rt_thread_t thread));

/**
 * @ingroup group_thread_management
 *
 * @brief Sets a hook function when a thread is initialized.
 *
 * @param thread is the target thread that initializing
 */
typedef void (*rt_thread_inited_hookproto_t)(rt_thread_t thread);
RT_OBJECT_HOOKLIST_DECLARE(rt_thread_inited_hookproto_t, rt_thread_inited);


/* excerpt lines 400-447 */
 * @{
 */

/**
 * Suspend list - A basic building block for IPC primitives which interacts with
 *                scheduler directly. Its API is similar to a FIFO list.
 *
 * Note: don't use in application codes directly
 */
void rt_susp_list_print(rt_list_t *list);
/* reserve thread error while resuming it */
#define RT_THREAD_RESUME_RES_THR_ERR (-1)
struct rt_thread *rt_susp_list_dequeue(rt_list_t *susp_list, rt_err_t thread_error);
rt_err_t rt_susp_list_resume_all(rt_list_t *susp_list, rt_err_t thread_error);
rt_err_t rt_susp_list_resume_all_irq(rt_list_t *susp_list,
                                     rt_err_t thread_error,
                                     struct rt_spinlock *lock);

/* suspend and enqueue */
rt_err_t rt_thread_suspend_to_list(rt_thread_t thread, rt_list_t *susp_list, int ipc_flags, int suspend_flag);
/* only for a suspended thread, and caller must hold the scheduler lock */
rt_err_t rt_susp_list_enqueue(rt_list_t *susp_list, rt_thread_t thread, int ipc_flags);

/**
 * @addtogroup group_semaphore Semaphore
 * @{
 */

#ifdef RT_USING_SEMAPHORE
/*
 * semaphore interface
 */
rt_err_t rt_sem_init(rt_sem_t    sem,
                     const char *name,
                     rt_uint32_t value,
                     rt_uint8_t  flag);
rt_err_t rt_sem_detach(rt_sem_t sem);
#ifdef RT_USING_HEAP
rt_sem_t rt_sem_create(const char *name, rt_uint32_t value, rt_uint8_t flag);
rt_err_t rt_sem_delete(rt_sem_t sem);
#endif /* RT_USING_HEAP */

rt_err_t rt_sem_take(rt_sem_t sem, rt_int32_t timeout);
rt_err_t rt_sem_take_interruptible(rt_sem_t sem, rt_int32_t timeout);
rt_err_t rt_sem_take_killable(rt_sem_t sem, rt_int32_t timeout);
rt_err_t rt_sem_trytake(rt_sem_t sem);
rt_err_t rt_sem_release(rt_sem_t sem);
rt_err_t rt_sem_control(rt_sem_t sem, int cmd, void *arg);

/* excerpt lines 644-699 */
                           rt_size_t size,
                           rt_int32_t *prio,
                           rt_int32_t timeout,
                           int suspend_flag);
#endif /* RT_USING_MESSAGEQUEUE_PRIORITY */
#endif /* RT_USING_MESSAGEQUEUE */

/**@}*/

/* defunct */
void rt_thread_defunct_init(void);
void rt_thread_defunct_enqueue(rt_thread_t thread);
rt_thread_t rt_thread_defunct_dequeue(void);
void rt_defunct_execute(void);

/*
 * spinlock
 */
struct rt_spinlock;

void rt_spin_lock_init(struct rt_spinlock *lock);
void rt_spin_lock(struct rt_spinlock *lock);
void rt_spin_unlock(struct rt_spinlock *lock);
rt_base_t rt_spin_lock_irqsave(struct rt_spinlock *lock);
void rt_spin_unlock_irqrestore(struct rt_spinlock *lock, rt_base_t level);

/**@}*/

#ifdef RT_USING_DEVICE
/**
 * @addtogroup group_device_driver
 * @{
 */

/*
 * device (I/O) system interface
 */
rt_device_t rt_device_find(const char *name);

rt_err_t rt_device_register(rt_device_t dev,
                            const char *name,
                            rt_uint16_t flags);
rt_err_t rt_device_unregister(rt_device_t dev);

#ifdef RT_USING_HEAP
rt_device_t rt_device_create(int type, int attach_size);
void rt_device_destroy(rt_device_t device);
#endif /* RT_USING_HEAP */

rt_err_t
rt_device_set_rx_indicate(rt_device_t dev,
                          rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size));
rt_err_t
rt_device_set_tx_complete(rt_device_t dev,
                          rt_err_t (*tx_done)(rt_device_t dev, void *buffer));


/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/rt-thread/include/rthw.h`

```c
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-18     Bernard      the first version
 * 2006-04-25     Bernard      add rt_hw_context_switch_interrupt declaration
 * 2006-09-24     Bernard      add rt_hw_context_switch_to declaration
 * 2012-12-29     Bernard      add rt_hw_exception_install declaration
 * 2017-10-17     Hichard      add some macros
 * 2018-11-17     Jesven       add rt_hw_spinlock_t
 *                             add smp support
 * 2019-05-18     Bernard      add empty definition for not enable cache case
 * 2023-09-15     xqyjlj       perf rt_hw_interrupt_disable/enable
 * 2023-10-16     Shell        Support a new backtrace framework
 */

#ifndef __RT_HW_H__
#define __RT_HW_H__

#include <rtdef.h>

#if defined (RT_USING_CACHE) || defined(RT_USING_SMP) || defined(RT_HW_INCLUDE_CPUPORT)
#include <cpuport.h> /* include spinlock, cache ops, etc. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Some macros define
 */
#ifndef HWREG64
#define HWREG64(x)          (*((volatile rt_uint64_t *)(x)))
#endif
#ifndef HWREG32
#define HWREG32(x)          (*((volatile rt_uint32_t *)(x)))
#endif
#ifndef HWREG16
#define HWREG16(x)          (*((volatile rt_uint16_t *)(x)))
#endif
#ifndef HWREG8
#define HWREG8(x)           (*((volatile rt_uint8_t *)(x)))
#endif

#ifndef RT_CPU_CACHE_LINE_SZ
#define RT_CPU_CACHE_LINE_SZ    32
#endif

enum RT_HW_CACHE_OPS
{
    RT_HW_CACHE_FLUSH      = 0x01,
    RT_HW_CACHE_INVALIDATE = 0x02,
};

/*
 * CPU interfaces
 */
#ifdef RT_USING_CACHE

#ifdef RT_USING_SMART
#include <cache.h>
#endif

void rt_hw_cpu_icache_enable(void);
void rt_hw_cpu_icache_disable(void);
rt_base_t rt_hw_cpu_icache_status(void);
void rt_hw_cpu_icache_ops(int ops, void* addr, int size);

void rt_hw_cpu_dcache_enable(void);
void rt_hw_cpu_dcache_disable(void);
rt_base_t rt_hw_cpu_dcache_status(void);
void rt_hw_cpu_dcache_ops(int ops, void* addr, int size);
#else

/* define cache ops as empty */
#define rt_hw_cpu_icache_enable(...)
#define rt_hw_cpu_icache_disable(...)
#define rt_hw_cpu_icache_ops(...)
#define rt_hw_cpu_dcache_enable(...)
#define rt_hw_cpu_dcache_disable(...)
#define rt_hw_cpu_dcache_ops(...)

#define rt_hw_cpu_icache_status(...) 0
#define rt_hw_cpu_dcache_status(...) 0

#endif

void rt_hw_cpu_reset(void);
void rt_hw_cpu_shutdown(void);

const char *rt_hw_cpu_arch(void);

rt_uint8_t *rt_hw_stack_init(void       *entry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *exit);

#ifdef RT_USING_HW_STACK_GUARD
void rt_hw_stack_guard_init(rt_thread_t thread);
#endif

/*
 * Interrupt handler definition
 */
typedef void (*rt_isr_handler_t)(int vector, void *param);

struct rt_irq_desc
{
    rt_isr_handler_t handler;
    void            *param;

#ifdef RT_USING_INTERRUPT_INFO
    char             name[RT_NAME_MAX];
    rt_uint32_t      counter;
#ifdef RT_USING_SMP
    rt_ubase_t       cpu_counter[RT_CPUS_NR];
#endif
#endif
};

/*
 * Interrupt interfaces
 */
void rt_hw_interrupt_init(void);
void rt_hw_interrupt_mask(int vector);
void rt_hw_interrupt_umask(int vector);
rt_isr_handler_t rt_hw_interrupt_install(int              vector,
                                         rt_isr_handler_t handler,
                                         void            *param,
                                         const char      *name);
void rt_hw_interrupt_uninstall(int              vector,
                               rt_isr_handler_t handler,
                               void            *param);

#ifdef RT_USING_SMP
rt_base_t rt_hw_local_irq_disable(void);
void rt_hw_local_irq_enable(rt_base_t level);

rt_base_t rt_cpus_lock(void);
void rt_cpus_unlock(rt_base_t level);

#define rt_hw_interrupt_disable rt_cpus_lock
#define rt_hw_interrupt_enable rt_cpus_unlock
#else
rt_base_t rt_hw_interrupt_disable(void);
void rt_hw_interrupt_enable(rt_base_t level);

#define rt_hw_local_irq_disable rt_hw_interrupt_disable
#define rt_hw_local_irq_enable rt_hw_interrupt_enable

#endif /*RT_USING_SMP*/
rt_bool_t rt_hw_interrupt_is_disabled(void);

/*
 * Context interfaces
 */
#ifdef RT_USING_SMP
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread);
void rt_hw_context_switch_to(rt_ubase_t to, struct rt_thread *to_thread);
void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread);
#else
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);
void rt_hw_context_switch_interrupt(rt_ubase_t from, rt_ubase_t to, rt_thread_t from_thread, rt_thread_t to_thread);
#endif /*RT_USING_SMP*/

/**
 * Hardware Layer Backtrace Service
 */
struct rt_hw_backtrace_frame {
    rt_uintptr_t fp;
    rt_uintptr_t pc;
};

rt_err_t rt_hw_backtrace_frame_get(rt_thread_t thread, struct rt_hw_backtrace_frame *frame);

rt_err_t rt_hw_backtrace_frame_unwind(rt_thread_t thread, struct rt_hw_backtrace_frame *frame);

void rt_hw_console_output(const char *str);

void rt_hw_show_memory(rt_uint32_t addr, rt_size_t size);

/*
 * Exception interfaces
 */
void rt_hw_exception_install(rt_err_t (*exception_handle)(void *context));

/*
 * delay interfaces
 */
void rt_hw_us_delay(rt_uint32_t us);

int rt_hw_cpu_id(void);

#if defined(RT_USING_SMP) || defined(RT_USING_AMP)
/**
 *  ipi function
 */
void rt_hw_ipi_send(int ipi_vector, unsigned int cpu_mask);
#endif

#ifdef RT_USING_SMP

void rt_hw_spin_lock_init(rt_hw_spinlock_t *lock);
void rt_hw_spin_lock(rt_hw_spinlock_t *lock);
void rt_hw_spin_unlock(rt_hw_spinlock_t *lock);

extern rt_hw_spinlock_t _cpus_lock;

#define __RT_HW_SPIN_LOCK_INITIALIZER(lockname) {0}

#define __RT_HW_SPIN_LOCK_UNLOCKED(lockname)    \
    (rt_hw_spinlock_t) __RT_HW_SPIN_LOCK_INITIALIZER(lockname)

#define RT_DEFINE_HW_SPINLOCK(x)  rt_hw_spinlock_t x = __RT_HW_SPIN_LOCK_UNLOCKED(x)

/**
 * boot secondary cpu
 */
void rt_hw_secondary_cpu_up(void);

/**
 * secondary cpu idle function
 */
void rt_hw_secondary_cpu_idle_exec(void);

#else /* !RT_USING_SMP */

#define RT_DEFINE_HW_SPINLOCK(x)    rt_ubase_t x

#define rt_hw_spin_lock(lock)     *(lock) = rt_hw_interrupt_disable()
#define rt_hw_spin_unlock(lock)   rt_hw_interrupt_enable(*(lock))


#endif /* RT_USING_SMP */

#ifndef RT_USING_CACHE
    #define rt_hw_isb()
    #define rt_hw_dmb()
    #define rt_hw_dsb()
#endif /* RT_USING_CACHE */

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/rt-thread/include/rttypes.h`

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-18     Shell        Separate the basic types from rtdef.h
 */

#ifndef __RT_TYPES_H__
#define __RT_TYPES_H__

#include <rtconfig.h>

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#ifndef RT_USING_NANO
#include <sys/types.h>
#include <sys/errno.h>
#if defined(RT_USING_SIGNALS) || defined(RT_USING_SMART)
#include <sys/signal.h>
#endif /* defined(RT_USING_SIGNALS) || defined(RT_USING_SMART) */
#endif /* RT_USING_NANO */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * RT-Thread basic data types definition
 */

#if defined(_WIN64) || defined(__x86_64__)
#ifndef ARCH_CPU_64BIT
#define ARCH_CPU_64BIT
#endif // ARCH_CPU_64BIT
#endif // defined(_WIN64) || defined(__x86_64__)

typedef int                             rt_bool_t;      /**< boolean type */

#ifndef RT_USING_ARCH_DATA_TYPE
#ifdef RT_USING_LIBC
typedef int8_t                          rt_int8_t;      /**<  8bit integer type */
typedef int16_t                         rt_int16_t;     /**< 16bit integer type */
typedef int32_t                         rt_int32_t;     /**< 32bit integer type */
typedef uint8_t                         rt_uint8_t;     /**<  8bit unsigned integer type */
typedef uint16_t                        rt_uint16_t;    /**< 16bit unsigned integer type */
typedef uint32_t                        rt_uint32_t;    /**< 32bit unsigned integer type */
typedef int64_t                         rt_int64_t;     /**< 64bit integer type */
typedef uint64_t                        rt_uint64_t;    /**< 64bit unsigned integer type */
#else
typedef signed   char                   rt_int8_t;      /**<  8bit integer type */
typedef signed   short                  rt_int16_t;     /**< 16bit integer type */
typedef signed   int                    rt_int32_t;     /**< 32bit integer type */
typedef unsigned char                   rt_uint8_t;     /**<  8bit unsigned integer type */
typedef unsigned short                  rt_uint16_t;    /**< 16bit unsigned integer type */
typedef unsigned int                    rt_uint32_t;    /**< 32bit unsigned integer type */
#ifdef ARCH_CPU_64BIT
typedef signed long                     rt_int64_t;     /**< 64bit integer type */
typedef unsigned long                   rt_uint64_t;    /**< 64bit unsigned integer type */
#else
typedef signed long long                rt_int64_t;     /**< 64bit integer type */
typedef unsigned long long              rt_uint64_t;    /**< 64bit unsigned integer type */
#endif /* ARCH_CPU_64BIT */
#endif /* RT_USING_LIBC */
#endif /* RT_USING_ARCH_DATA_TYPE */

#ifdef ARCH_CPU_64BIT
typedef rt_int64_t                      rt_base_t;      /**< Nbit CPU related data type */
typedef rt_uint64_t                     rt_ubase_t;     /**< Nbit unsigned CPU related data type */
#else
typedef rt_int32_t                      rt_base_t;      /**< Nbit CPU related data type */
typedef rt_uint32_t                     rt_ubase_t;     /**< Nbit unsigned CPU related data type */
#endif

#if defined(RT_USING_LIBC) && !defined(RT_USING_NANO)
typedef size_t                          rt_size_t;      /**< Type for size number */
typedef ssize_t                         rt_ssize_t;     /**< Used for a count of bytes or an error indication */
typedef intptr_t                        rt_intptr_t;    /**< Type for signed pointer length integer */
typedef uintptr_t                       rt_uintptr_t;   /**< Type for unsigned pointer length integer */
#else
typedef rt_ubase_t                      rt_size_t;      /**< Type for size number */
typedef rt_base_t                       rt_ssize_t;     /**< Used for a count of bytes or an error indication */
typedef rt_base_t                      rt_intptr_t;    /**< Type for signed pointer length integer */
typedef rt_ubase_t                       rt_uintptr_t;   /**< Type for unsigned pointer length integer */
#endif /* defined(RT_USING_LIBC) && !defined(RT_USING_NANO) */

typedef rt_base_t                       rt_err_t;       /**< Type for error number */
typedef rt_uint32_t                     rt_tick_t;      /**< Type for tick count */
typedef rt_base_t                       rt_flag_t;      /**< Type for flags */
typedef rt_ubase_t                      rt_dev_t;       /**< Type for device */
typedef rt_base_t                       rt_off_t;       /**< Type for offset */

#if defined(RT_USING_STDC_ATOMIC) && __STDC_VERSION__ < 201112L
#undef RT_USING_STDC_ATOMIC
#warning Not using C11 or beyond! Maybe you should change the -std option on your compiler
#endif

#ifdef __cplusplus
    typedef rt_uint8_t rt_atomic8_t;
    typedef rt_uint16_t rt_atomic16_t;
    typedef rt_base_t rt_atomic_t;
#else
    #if defined(RT_USING_STDC_ATOMIC)
        #include <stdatomic.h>
        typedef _Atomic(rt_uint8_t) rt_atomic8_t;
        typedef _Atomic(rt_uint16_t) rt_atomic16_t;
        typedef _Atomic(rt_base_t) rt_atomic_t;
    #elif defined(RT_USING_HW_ATOMIC)
        typedef rt_uint8_t rt_atomic8_t;
        typedef rt_uint16_t rt_atomic16_t;
        typedef rt_base_t rt_atomic_t;
    #else
        typedef rt_uint8_t rt_atomic8_t;
        typedef rt_uint16_t rt_atomic16_t;
        typedef rt_base_t rt_atomic_t;
    #endif /* RT_USING_STDC_ATOMIC */
#endif /* __cplusplus */

/* boolean type definitions */
#define RT_TRUE                         1               /**< boolean true  */
#define RT_FALSE                        0               /**< boolean fails */

/* null pointer definition */
#define RT_NULL                         0

/**
 * Double List structure
 */
struct rt_list_node
{
    struct rt_list_node *next;                          /**< point to next node. */
    struct rt_list_node *prev;                          /**< point to prev node. */
};
typedef struct rt_list_node rt_list_t;                  /**< Type for lists. */

/**
 * Single List structure
 */
struct rt_slist_node
{
    struct rt_slist_node *next;                         /**< point to next node. */
};
typedef struct rt_slist_node rt_slist_t;                /**< Type for single list. */

/**
 * Lock-less Single List structure
 */
struct rt_lockless_slist_node
{
    rt_atomic_t next;                                   /**< point to next node. */
};
typedef struct rt_lockless_slist_node rt_ll_slist_t;    /**< Type for lock-les single list. */

/**
 * Spinlock
 */
#ifdef RT_USING_SMP
#include <cpuport.h> /* for spinlock from arch */

struct rt_spinlock
{
    rt_hw_spinlock_t lock;
#ifdef RT_USING_DEBUG
    rt_uint32_t critical_level;
#endif /* RT_USING_DEBUG */
#if defined(RT_DEBUGING_SPINLOCK)
    void *owner;
    void *pc;
#endif /* RT_DEBUGING_SPINLOCK */
};

#ifndef RT_SPINLOCK_INIT
#define RT_SPINLOCK_INIT {{0}} /* can be overridden by cpuport.h */
#endif /* RT_SPINLOCK_INIT */

#else /* !RT_USING_SMP */

struct rt_spinlock
{
#ifdef RT_USING_DEBUG
    rt_uint32_t critical_level;
#endif /* RT_USING_DEBUG */
    rt_ubase_t lock;
};
#define RT_SPINLOCK_INIT {0}
#endif /* RT_USING_SMP */
#if defined(RT_DEBUGING_SPINLOCK) && defined(RT_USING_SMP)

    #define __OWNER_MAGIC ((void *)0xdeadbeaf)

    #if defined(__GNUC__)
    #define __GET_RETURN_ADDRESS __builtin_return_address(0)
    #else /* !__GNUC__ */
    #define __GET_RETURN_ADDRESS RT_NULL
    #endif /* __GNUC__ */

    #define _SPIN_LOCK_DEBUG_OWNER(lock)                  \
        do                                                \
        {                                                 \
            struct rt_thread *_curthr = rt_thread_self(); \
            if (_curthr != RT_NULL)                       \
            {                                             \
                (lock)->owner = _curthr;                  \
                (lock)->pc = __GET_RETURN_ADDRESS;        \
            }                                             \
        } while (0)

    #define _SPIN_UNLOCK_DEBUG_OWNER(lock) \
        do                                 \
        {                                  \
            (lock)->owner = __OWNER_MAGIC; \
            (lock)->pc = RT_NULL;          \
        } while (0)

#else /* !RT_DEBUGING_SPINLOCK */

    #define _SPIN_LOCK_DEBUG_OWNER(lock)    RT_UNUSED(lock)
    #define _SPIN_UNLOCK_DEBUG_OWNER(lock)  RT_UNUSED(lock)
#endif /* RT_DEBUGING_SPINLOCK */

#ifdef RT_DEBUGING_CRITICAL
    #define _SPIN_LOCK_DEBUG_CRITICAL(lock)               \
        do                                                \
        {                                                 \
            (lock)->critical_level = rt_critical_level(); \
        } while (0)

    #define _SPIN_UNLOCK_DEBUG_CRITICAL(lock, critical) \
        do                                              \
        {                                               \
            (critical) = (lock)->critical_level;        \
        } while (0)

#else /* !RT_DEBUGING_CRITICAL */
    #define _SPIN_LOCK_DEBUG_CRITICAL(lock)             RT_UNUSED(lock)
    #define _SPIN_UNLOCK_DEBUG_CRITICAL(lock, critical) do {critical = 0; RT_UNUSED(lock);} while (0)

#endif /* RT_DEBUGING_CRITICAL */

#define RT_SPIN_LOCK_DEBUG(lock)         \
    do                                   \
    {                                    \
        _SPIN_LOCK_DEBUG_OWNER(lock);    \
        _SPIN_LOCK_DEBUG_CRITICAL(lock); \
    } while (0)

#define RT_SPIN_UNLOCK_DEBUG(lock, critical)         \
    do                                               \
    {                                                \
        _SPIN_UNLOCK_DEBUG_OWNER(lock);              \
        _SPIN_UNLOCK_DEBUG_CRITICAL(lock, critical); \
    } while (0)

typedef struct rt_spinlock rt_spinlock_t;

#define RT_DEFINE_SPINLOCK(x)  struct rt_spinlock x = RT_SPINLOCK_INIT

#ifdef __cplusplus
}
#endif

#endif /* __RT_TYPES_H__ */
```

## Source: `data/rtos/rt-thread/include/klibc/kerrno.h`

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-22     Meco Man     the first version
 */

#ifndef __RT_KERRNO_H__
#define __RT_KERRNO_H__

#include <rtconfig.h>
#include <rttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RT_USING_LIBC) && !defined(RT_USING_NANO)
/* POSIX error code compatible */
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        255             /**< A generic/unknown error happens */
#define RT_ETIMEOUT                     ETIMEDOUT       /**< Timed out */
#define RT_EFULL                        ENOSPC          /**< The resource is full */
#define RT_EEMPTY                       ENODATA         /**< The resource is empty */
#define RT_ENOMEM                       ENOMEM          /**< No memory */
#define RT_ENOSYS                       ENOSYS          /**< Function not implemented */
#define RT_EBUSY                        EBUSY           /**< Busy */
#define RT_EIO                          EIO             /**< IO error */
#define RT_EINTR                        EINTR           /**< Interrupted system call */
#define RT_EINVAL                       EINVAL          /**< Invalid argument */
#define RT_ENOENT                       ENOENT          /**< No entry */
#define RT_ENOSPC                       ENOSPC          /**< No space left */
#define RT_EPERM                        EPERM           /**< Operation not permitted */
#define RT_EFAULT                       EFAULT          /**< Bad address */
#define RT_ENOBUFS                      ENOBUFS         /**< No buffer space is available */
#define RT_ESCHEDISR                    253             /**< scheduler failure in isr context */
#define RT_ESCHEDLOCKED                 252             /**< scheduler failure in critical region */
#define RT_ETRAP                        254             /**< Trap event */
#else
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic/unknown error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< Function not implemented */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */
#define RT_EINTR                        9               /**< Interrupted system call */
#define RT_EINVAL                       10              /**< Invalid argument */
#define RT_ENOENT                       11              /**< No entry */
#define RT_ENOSPC                       12              /**< No space left */
#define RT_EPERM                        13              /**< Operation not permitted */
#define RT_ETRAP                        14              /**< Trap event */
#define RT_EFAULT                       15              /**< Bad address */
#define RT_ENOBUFS                      16              /**< No buffer space is available */
#define RT_ESCHEDISR                    17              /**< scheduler failure in isr context */
#define RT_ESCHEDLOCKED                 18              /**< scheduler failure in critical region */
#endif /* defined(RT_USING_LIBC) && !defined(RT_USING_NANO) */

rt_err_t rt_get_errno(void);
void rt_set_errno(rt_err_t no);
int *_rt_errno(void);
const char *rt_strerror(rt_err_t error);
#if !defined(RT_USING_NEWLIBC) && !defined(_WIN32)
#ifndef errno
#define errno    *_rt_errno()
#endif
#endif /* !defined(RT_USING_NEWLIBC) && !defined(_WIN32) */

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/rt-thread/documentation/6.components/device-driver/pin/pin.md`

```
@page page_device_pin PIN Device

# Introduction of Pin

The pins on the chip are generally divided into four categories: power supply, clock, control, and I/O. The I/O pins are further divided into General Purpose Input Output (GPIO) and function-multiplexed I/O (such as SPI/I2C/UART, etc.) pins, referring to their usage mode.

Most MCU pins have more than one function. Their internal structure is different and their supported functionality are different. The actual function of the pin can be switched through different configurations. The main features of the General Purpose Input Output (GPIO) port are as follows:

* Programmable Interrupt: The interrupt trigger mode is configurable. Generally, there are five interrupt trigger modes as shown in the following figure:

  ![5 Interrupt Trigger Modes](figures/pin2.png)

* Input and output modes can be controlled.

   * Output modes generally include Output push-pull, Output open-drain, Output pull-up, and Output pull-down. When the pin is in the output mode, the connected peripherals can be controlled by configuring the level of the pin output to be high or low.

   * Input modes generally include: Input floating,  Input pull-up,  Input pull-down, and  Analog. When the pin is in the input mode, the level state of the pin can be read, that is, high level or low level.

# Access PIN Device

The application accesses the GPIO through the PIN device management interface provided by RT-Thread. The related interfaces are as follows:

| Function | **Description**             |
| ---------------- | ---------------------------------- |
| rt_pin_mode()  | Set pin mode |
| rt_pin_write()     | Set the pin level |
| rt_pin_read()   | Read pin level |
| rt_pin_attach_irq()  | Bind pin interrupt callback function |
| rt_pin_irq_enable()   | Enable pin interrupt |
| rt_pin_detach_irq()  | Detach pin interrupt callback function |

## Obtain Pin Number

The pin numbers provided by RT-Thread need to be distinguished from the chip pin numbers, which not the same. The pin numbers are defined by the PIN device driver and are related to the specific chip used. There are two ways to obtain the pin number: use the macro definition or view the PIN driver file.

### Use Macro Definition

If you use the BSP in the `rt-thread/bsp/stm32` directory, you can use the following macro to obtain the pin number:

```c
GET_PIN(port, pin)
```

The sample code for the pin number corresponding to LED0 with pin number PF9 is as follows:

```c
#define LED0_PIN        GET_PIN(F, 9)
```

### View Driver Files

If you use a different BSP, you will need to check the PIN driver code `drv_gpio.c` file to confirm the pin number. There is an array in this file that holds the number information for each PIN pin, as shown below:

```c
static const rt_uint16_t pins[] =
{
    __STM32_PIN_DEFAULT,
    __STM32_PIN_DEFAULT,
    __STM32_PIN(2, A, 15),
    __STM32_PIN(3, B, 5),
    __STM32_PIN(4, B, 8),
    __STM32_PIN_DEFAULT,
    __STM32_PIN_DEFAULT,
    __STM32_PIN_DEFAULT,
    __STM32_PIN(8, A, 14),
    __STM32_PIN(9, B, 6),
    ... ...
}
```

Take `__STM32_PIN(2, A, 15)` as an example, 2 is the pin number used by RT-Thread, A is the port number, and 15 is the pin number, so the pin number corresponding to PA15 is 2.

## Set Pin Mode

Before the pin is used, you need to set the input or output mode first, and the following functions are used:

```c
void rt_pin_mode(rt_base_t pin, rt_base_t mode);
```

| Parameter | **Discription**    |
| --------- | ------------------ |
| pin       | Pin number         |
| mode      | Pin operation mode |

At present, the pin working mode supported by RT-Thread can take one of the five macro definition values as shown. The mode supported by the chip corresponding to each mode needs to refer to the specific implementation of the PIN device driver:

```c
#define PIN_MODE_OUTPUT 0x00            /* Output */
#define PIN_MODE_INPUT 0x01             /* Input */
#define PIN_MODE_INPUT_PULLUP 0x02      /* input Pull up  */
#define PIN_MODE_INPUT_PULLDOWN 0x03    /* input Pull down  */
#define PIN_MODE_OUTPUT_OD 0x04         /* output Open drain  */
```

An example of use is as follows:

```c
#define BEEP_PIN_NUM            35  /* PB0 */

/* Buzzer pin is in output mode */
rt_pin_mode(BEEP_PIN_NUM, PIN_MODE_OUTPUT);
```

## Set The Pin Level

The function to set the pin output level is as follows:

```c
void rt_pin_write(rt_base_t pin, rt_base_t value);
```

| **Parameter** | Discription      |
|----------|-------------------------|
| pin      | Pin number        |
| value    | Level logic value, which can take one of two macro definition values: PIN_LOW means low level, or PIN_HIGH means high level |

Examples of use are as follows:

```c
#define BEEP_PIN_NUM            35  /* PB0 */

/* Beep's pin is in output mode */
rt_pin_mode(BEEP_PIN_NUM, PIN_MODE_OUTPUT);
/* Set low level */
rt_pin_write(BEEP_PIN_NUM, PIN_LOW);
```

## Read Pin Level

The functions to read the pin level are as follows:

```c
int rt_pin_read(rt_base_t pin);
```

| Parameter  | Description |
| ---------- | ----------- |
| pin        | Pin number  |
| **return** | ——          |
| PIN_LOW    | Low level   |
| PIN_HIGH   | High level  |

Examples of use are as follows:

```c
#define BEEP_PIN_NUM            35  /* PB0 */
int status;

/* Buzzer pin is in output mode */
rt_pin_mode(BEEP_PIN_NUM, PIN_MODE_OUTPUT);
/* Set low level */
rt_pin_write(BEEP_PIN_NUM, PIN_LOW);

status = rt_pin_read(BEEP_PIN_NUM);
```

## Bind Pin Interrupt Callback Function

To use the interrupt functionality of a pin, you can use the following function to configure the pin to some interrupt trigger mode and bind an interrupt callback function to the corresponding pin. When the pin interrupt occurs, the callback function will be executed.:

```c
rt_err_t rt_pin_attach_irq(rt_int32_t pin, rt_uint32_t mode,
                           void (*hdr)(void *args), void *args);
```

| Parameter  | Description                                                  |
| ---------- | ------------------------------------------------------------ |
| pin        | Pin number                                                   |
| mode       | Interrupt trigger mode                                       |
| hdr        | Interrupt callback function. Users need to define this function |
| args       | Interrupt the parameters of the callback function, set to RT_NULL when not needed |
| return     | ——                                                           |
| RT_EOK     | Binding succeeded                                            |
| error code | Binding failed                                               |

Interrupt trigger mode mode can take one of the following five macro definition values:

```c
#define PIN_IRQ_MODE_RISING 0x00         /* Rising edge trigger */
#define PIN_IRQ_MODE_FALLING 0x01        /* Falling edge trigger */
#define PIN_IRQ_MODE_RISING_FALLING 0x02 /* Edge trigger (triggered on both rising and falling edges)*/
#define PIN_IRQ_MODE_HIGH_LEVEL 0x03     /* High level trigger */
#define PIN_IRQ_MODE_LOW_LEVEL 0x04      /* Low level trigger */
```

Examples of use are as follows:

```c
#define KEY0_PIN_NUM            55  /* PD8 */
/* Interrupt callback function */
void beep_on(void *args)
{
    rt_kprintf("turn on beep!\n");

    rt_pin_write(BEEP_PIN_NUM, PIN_HIGH);
}
static void pin_beep_sample(void)
{
    /* Button 0 pin is the input mode */
    rt_pin_mode(KEY0_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    /* Bind interrupt, rising edge mode, callback function named beep_on */
    rt_pin_attach_irq(KEY0_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
}
```

## Enable Pin Interrupt

After binding the pin interrupt callback function, use the following function to enable pin interrupt:

```c
rt_err_t rt_pin_irq_enable(rt_base_t pin, rt_uint32_t enabled);
```

| **Parameter** | **Description** |
|----------|----------------|
| pin      | Pin number |
| enabled  | Status, one of two values: PIN_IRQ_ENABLE, and PIN_IRQ_DISABLE |
| **return** | ——             |
| RT_EOK   | Enablement succeeded |
| error code | Enablement failed |

Examples of use are as follows:

```c
#define KEY0_PIN_NUM            55  /* PD8 */
/* Interrupt callback function */
void beep_on(void *args)
{
    rt_kprintf("turn on beep!\n");

    rt_pin_write(BEEP_PIN_NUM, PIN_HIGH);
}
static void pin_beep_sample(void)
{
    /* Key 0 pin is the input mode */
    rt_pin_mode(KEY0_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    /* Bind interrupt, rising edge mode, callback function named beep_on */
    rt_pin_attach_irq(KEY0_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
    /* Enable interrupt */
    rt_pin_irq_enable(KEY0_PIN_NUM, PIN_IRQ_ENABLE);
}
```

## Detach Pin Interrupt Callback Function

You can use the following function to detach the pin interrupt callback function:

```c
rt_err_t rt_pin_detach_irq(rt_int32_t pin);
```

| **Parameter** | **Description**      |
| ------------- | -------------------- |
| pin           | Pin number           |
| **return**    | ——                   |
| RT_EOK        | Detachment succeeded |
| error code    | Detachment failed    |

After the pin detaches the interrupt callback function, the interrupt is not closed. You can also call the bind interrupt callback function to bind the other callback functions again.

```c
#define KEY0_PIN_NUM            55  /* PD8 */
/* Interrupt callback function */
void beep_on(void *args)
{
    rt_kprintf("turn on beep!\n");

    rt_pin_write(BEEP_PIN_NUM, PIN_HIGH);
}
static void pin_beep_sample(void)
{
    /* Key 0 pin is the input mode */
    rt_pin_mode(KEY0_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    /* Bind interrupt, rising edge mode, callback function named beep_on */
    rt_pin_attach_irq(KEY0_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
    /* Enable interrupt */
    rt_pin_irq_enable(KEY0_PIN_NUM, PIN_IRQ_ENABLE);
    /* Detach interrupt callback function */
    rt_pin_detach_irq(KEY0_PIN_NUM);
}
```

# PIN Device Usage Example

The following sample code is the pin device usage example. The main steps of the sample code are as follows:

1. Set the corresponding pin of the beep to the output mode and give a default low state.

2. Set the key 0 and button 1 corresponding to the input mode, then bind the interrupt callback function and enable the interrupt.

3. When the key 0 is pressed, the beep starts to sound, and when the key 1 is pressed, the beep stops.

```c
/*
 * Program listing: This is a PIN device usage routine
 * The routine exports the pin_beep_sample command to the control terminal
 * Command call format：pin_beep_sample
 * Program function: control the buzzer by controlling the level state of the corresponding pin of the buzzer by pressing the button
*/

#include <rtthread.h>
#include <rtdevice.h>

/* Pin number, determined by looking at the device driver file drv_gpio.c */
#ifndef BEEP_PIN_NUM
    #define BEEP_PIN_NUM            35  /* PB0 */
#endif
#ifndef KEY0_PIN_NUM
    #define KEY0_PIN_NUM            55  /* PD8 */
#endif
#ifndef KEY1_PIN_NUM
    #define KEY1_PIN_NUM            56  /* PD9 */
#endif

void beep_on(void *args)
{
    rt_kprintf("turn on beep!\n");

    rt_pin_write(BEEP_PIN_NUM, PIN_HIGH);
}

void beep_off(void *args)
{
    rt_kprintf("turn off beep!\n");

    rt_pin_write(BEEP_PIN_NUM, PIN_LOW);
}

static void pin_beep_sample(void)
{
    /* Beep pin is in output mode */
    rt_pin_mode(BEEP_PIN_NUM, PIN_MODE_OUTPUT);
    /* Default low level */
    rt_pin_write(BEEP_PIN_NUM, PIN_LOW);

    /* KEY 0 pin is the input mode */
    rt_pin_mode(KEY0_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    /* Bind interrupt, falling edge mode, callback function named beep_on */
    rt_pin_attach_irq(KEY0_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_on, RT_NULL);
    /* Enable interrupt */
    rt_pin_irq_enable(KEY0_PIN_NUM, PIN_IRQ_ENABLE);

    /* KEY 1 pin is input mode */
    rt_pin_mode(KEY1_PIN_NUM, PIN_MODE_INPUT_PULLUP);
    /* Binding interrupt, falling edge mode, callback function named beep_off */
    rt_pin_attach_irq(KEY1_PIN_NUM, PIN_IRQ_MODE_FALLING, beep_off, RT_NULL);
    /* Enable interrupt */
    rt_pin_irq_enable(KEY1_PIN_NUM, PIN_IRQ_ENABLE);
}
/* Export to the msh command list */
MSH_CMD_EXPORT(pin_beep_sample, pin beep sample);
```
```
