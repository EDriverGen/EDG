# Raw RTOS/Bus Pack: rt-thread / i2c

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rt-thread/components/drivers/include/drivers/dev_i2c.h`

```c
/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2021-04-20     RiceChen      added support for bus control api
 * 2024-06-23     wdfk-prog     Add the config struct
 */

#ifndef __DEV_I2C_H__
#define __DEV_I2C_H__

#include <rtthread.h>
/**
 * @defgroup    group_drivers_i2c I2C
 * @brief       I2C driver api
 * @ingroup     group_device_driver
 *
 * <b>Example</b>
 * @code {.c}
 * #include <rtthread.h>
 * #include <rtdevice.h>
 *
 * #define AHT10_I2C_BUS_NAME          "i2c1"  // I2C bus device name for the sensor
 * #define AHT10_ADDR                  0x38    // Slave address
 * #define AHT10_CALIBRATION_CMD       0xE1    // Calibration command
 * #define AHT10_NORMAL_CMD            0xA8    // Normal command
 * #define AHT10_GET_DATA              0xAC    // Read-data command
 *
 * static struct rt_i2c_bus_device *i2c_bus = RT_NULL;     // I2C bus device handle
 * static rt_bool_t initialized = RT_FALSE;                // Sensor initialization state
 *
 * // Write a sensor register
 * static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *data)
 * {
 *     rt_uint8_t buf[3];
 *     struct rt_i2c_msg msgs;
 *     rt_uint32_t buf_size = 1;
 *
 *     buf[0] = reg; //cmd
 *     if (data != RT_NULL)
 *     {
 *         buf[1] = data[0];
 *         buf[2] = data[1];
 *         buf_size = 3;
 *     }
 *
 *     msgs.addr = AHT10_ADDR;
 *     msgs.flags = RT_I2C_WR;
 *     msgs.buf = buf;
 *     msgs.len = buf_size;
 *
 *     // Transfer data through the I2C device interface
 *     if (rt_i2c_transfer(bus, &msgs, 1) == 1)
 *     {
 *         return RT_EOK;
 *     }
 *     else
 *     {
 *         return -RT_ERROR;
 *     }
 * }
 *
 * // Read sensor register data
 * static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
 * {
 *     struct rt_i2c_msg msgs;
 *
 *     msgs.addr = AHT10_ADDR;
 *     msgs.flags = RT_I2C_RD;
 *     msgs.buf = buf;
 *     msgs.len = len;
 *
 *     // Transfer data through the I2C device interface
 *     if (rt_i2c_transfer(bus, &msgs, 1) == 1)
 *     {
 *         return RT_EOK;
 *     }
 *     else
 *     {
 *         return -RT_ERROR;
 *     }
 * }
 *
 * static void read_temp_humi(float *cur_temp, float *cur_humi)
 * {
 *     rt_uint8_t temp[6];
 *
 *     write_reg(i2c_bus, AHT10_GET_DATA, RT_NULL);      // Send command
 *     rt_thread_mdelay(400);
 *     read_regs(i2c_bus, 6, temp);                // Read sensor data
 *
 *     // Convert humidity data
 *     *cur_humi = (temp[1] << 12 | temp[2] << 4 | (temp[3] & 0xf0) >> 4) * 100.0 / (1 << 20);
 *     // Convert temperature data
 *     *cur_temp = ((temp[3] & 0xf) << 16 | temp[4] << 8 | temp[5]) * 200.0 / (1 << 20) - 50;
 * }
 *
 * static void aht10_init(const char *name)
 * {
 *     rt_uint8_t temp[2] = {0, 0};
 *
 *     // Find the I2C bus device and obtain its handle
 *     i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);
 *
 *     if (i2c_bus == RT_NULL)
 *     {
 *         rt_kprintf("can't find %s device!\n", name);
 *     }
 *     else
 *     {
 *         write_reg(i2c_bus, AHT10_NORMAL_CMD, temp);
 *         rt_thread_mdelay(400);
 *
 *         temp[0] = 0x08;
 *         temp[1] = 0x00;
 *         write_reg(i2c_bus, AHT10_CALIBRATION_CMD, temp);
 *         rt_thread_mdelay(400);
 *         initialized = RT_TRUE;
 *     }
 * }
 *
 * static void i2c_aht10_sample(int argc, char *argv[])
 * {
 *     float humidity, temperature;
 *     char name[RT_NAME_MAX];
 *
 *     humidity = 0.0;
 *     temperature = 0.0;
 *
 *     if (argc == 2)
 *     {
 *         rt_strncpy(name, argv[1], RT_NAME_MAX);
 *     }
 *     else
 *     {
 *         rt_strncpy(name, AHT10_I2C_BUS_NAME, RT_NAME_MAX);
 *     }
 *
 *     if (!initialized)
 *     {
 *         // Initialize the sensor
 *         aht10_init(name);
 *     }
 *     if (initialized)
 *     {
 *         // Read temperature and humidity data
 *         read_temp_humi(&temperature, &humidity);
 *
 *         rt_kprintf("read aht10 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
 *         if( temperature >= 0 )
 *         {
 *             rt_kprintf("read aht10 sensor temperature: %d.%d°C\n", (int)temperature, (int)(temperature * 10) % 10);
 *         }
 *         else
 *         {
 *             rt_kprintf("read aht10 sensor temperature: %d.%d°C\n", (int)temperature, (int)(-temperature * 10) % 10);
 *         }
 *     }
 *     else
 *     {
 *         rt_kprintf("initialize sensor failed!\n");
 *     }
 * }
 * // Export to the msh command list
 * MSH_CMD_EXPORT(i2c_aht10_sample, i2c aht10 sample);
 * @endcode
 */

/*!
 * @addtogroup group_drivers_i2c
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

#define RT_I2C_WR                0x0000    /*!< i2c wirte flag */
#define RT_I2C_RD               (1u << 0)  /*!< i2c read flag  */
#define RT_I2C_ADDR_10BIT       (1u << 2)  /*!< this is a ten bit chip address */
#define RT_I2C_NO_START         (1u << 4)  /*!< do not generate START condition */
#define RT_I2C_IGNORE_NACK      (1u << 5)  /*!< ignore NACK from slave */
#define RT_I2C_NO_READ_ACK      (1u << 6)  /* when I2C reading, we do not ACK */
#define RT_I2C_NO_STOP          (1u << 7)  /*!< do not generate STOP condition */

#define RT_I2C_CTRL_SET_MAX_HZ  0x20

#define RT_I2C_DEV_CTRL_10BIT        (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x01)
#define RT_I2C_DEV_CTRL_ADDR         (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x02)
#define RT_I2C_DEV_CTRL_TIMEOUT      (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x03)
#define RT_I2C_DEV_CTRL_RW           (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x04)
#define RT_I2C_DEV_CTRL_CLK          (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x05)
#define RT_I2C_DEV_CTRL_UNLOCK       (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x06)
#define RT_I2C_DEV_CTRL_GET_STATE    (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x07)
#define RT_I2C_DEV_CTRL_GET_MODE     (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x08)
#define RT_I2C_DEV_CTRL_GET_ERROR    (RT_DEVICE_CTRL_BASE(I2CBUS) + 0x09)

/**
 * @brief I2C Private Data
 */
struct rt_i2c_priv_data
{
    struct rt_i2c_msg  *msgs;
    rt_size_t  number;
};

/**
 * @brief I2C Message
 */
struct rt_i2c_msg
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    rt_uint16_t len;
    rt_uint8_t  *buf;
};

struct rt_i2c_bus_device;

/**
 * @brief I2C Bus Device Operations
 */
struct rt_i2c_bus_device_ops
{
    rt_ssize_t (*master_xfer)(struct rt_i2c_bus_device *bus,
                             struct rt_i2c_msg msgs[],
                             rt_uint32_t num);
    rt_ssize_t (*slave_xfer)(struct rt_i2c_bus_device *bus,
                            struct rt_i2c_msg msgs[],
                            rt_uint32_t num);
    rt_err_t (*i2c_bus_control)(struct rt_i2c_bus_device *bus,
                                int cmd,
                                void *args);
};

/**
 * I2C configuration structure.
 * mode : master: 0x00; slave: 0x01;
 * max_hz: Maximum limit baud rate.
 * usage_freq: Actual usage baud rate.
 */
struct rt_i2c_configuration
{
    rt_uint8_t  mode;
    rt_uint8_t  reserved[3];

    rt_uint32_t max_hz;
    rt_uint32_t usage_freq;
};

/**
 * @brief I2C Bus Device
 */
struct rt_i2c_bus_device
{
    struct rt_device parent;
    const struct rt_i2c_bus_device_ops *ops;
    rt_uint16_t  flags;
    struct rt_mutex lock;
    rt_uint32_t  timeout;
    rt_uint32_t  retries;
    struct rt_i2c_configuration config;
    void *priv;
};

/**
 * @brief I2C Client
 */
struct rt_i2c_client
{
#ifdef RT_USING_DM
    struct rt_device parent;

    const char *name;
    const struct rt_i2c_device_id *id;
    const struct rt_ofw_node_id *ofw_id;
#endif
    struct rt_i2c_bus_device       *bus;
    rt_uint16_t                    client_addr;
};

#ifdef RT_USING_DM
struct rt_i2c_device_id
{
    char name[20];
    void *data;
};

struct rt_i2c_driver
{
    struct rt_driver parent;

    const struct rt_i2c_device_id *ids;
    const struct rt_ofw_node_id *ofw_ids;

    rt_err_t (*probe)(struct rt_i2c_client *client);
    rt_err_t (*remove)(struct rt_i2c_client *client);
    rt_err_t (*shutdown)(struct rt_i2c_client *client);
};

rt_err_t rt_i2c_driver_register(struct rt_i2c_driver *driver);
rt_err_t rt_i2c_device_register(struct rt_i2c_client *client);

#define RT_I2C_DRIVER_EXPORT(driver)  RT_DRIVER_EXPORT(driver, i2c, BUILIN)

/**
 * @brief Get ID match data from I2C client
 *
 * This function retrieves the driver-specific data associated with the matched
 * device ID or OFW node ID for the I2C client.
 *
 * @param client the I2C client device
 *
 * @return const void* pointer to the ID match data, or RT_NULL if no match data exists
 */
rt_inline const void *rt_i2c_client_id_data(struct rt_i2c_client *client)
{
    return client->id ? client->id->data : (client->ofw_id ? client->ofw_id->data : RT_NULL);
}
#endif /* RT_USING_DM */

/**
 * @brief I2C Bus Device Initialization
 *
 * @param bus the I2C bus device
 * @param name the name of I2C bus device
 *
 * @return rt_err_t error code
 */
rt_err_t rt_i2c_bus_device_device_init(struct rt_i2c_bus_device *bus,
                                       const char               *name);

/**
 * @brief I2C Bus Device Register
 *
 * @param bus the I2C bus device
 * @param bus_name the name of I2C bus device
 *
 * @return rt_err_t error code
 */
rt_err_t rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name);

/**
 * @brief I2C Bus Device Find
 *
 * @param bus_name the name of I2C bus device
 *
 * @return rt_i2c_bus_device the I2C bus device
 */
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);

/**
 * @brief I2C data transmission.
 *
 * @param bus the I2C bus device
 * @param msgs the I2C message list
 * @param num the number of I2C message
 *
 * @return rt_ssize_t the actual length of transmitted
 */
rt_ssize_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num);

/**
 * @brief I2C Control
 *
 * @param bus the I2C bus device
 * @param cmd the I2C control command
 * @param args the I2C control arguments
 *
 * @return rt_err_t error code
 */
rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        int cmd,
                        void *args);

/**
 * @brief I2C Master Send
 *
 * @param bus the I2C bus device
 * @param addr the I2C slave address
 * @param flags the I2C flags
 * @param buf the I2C send buffer
 * @param count the I2C send buffer length
 *
 * @return rt_ssize_t the actual length of transmitted
 */
rt_ssize_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count);

/**
 * @brief I2C Master Receive
 *
 * @param bus the I2C bus device
 * @param addr the I2C slave address
 * @param flags the I2C flags
 * @param buf the I2C receive buffer
 * @param count the I2C receive buffer length
 *
 * @return rt_ssize_t the actual length of received
 */
rt_ssize_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count);

rt_inline rt_err_t rt_i2c_bus_lock(struct rt_i2c_bus_device *bus, rt_tick_t timeout)
{
    return rt_mutex_take(&bus->lock, timeout);
}

rt_inline rt_err_t rt_i2c_bus_unlock(struct rt_i2c_bus_device *bus)
{
    return rt_mutex_release(&bus->lock);
}

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

## Source: `data/rtos/rt-thread/components/drivers/i2c/dev_i2c_core.c`

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2021-04-20     RiceChen      added support for bus control api
 * 2024-06-23     wdfk-prog     Add max_hz setting
 */

#include <rtdevice.h>

#define DBG_TAG               "I2C"
#ifdef RT_I2C_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

rt_err_t rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name)
{
    rt_err_t res = RT_EOK;

    rt_mutex_init(&bus->lock, "i2c_lock", RT_IPC_FLAG_PRIO);

    if (bus->timeout == 0) bus->timeout = RT_TICK_PER_SECOND;

    res = rt_i2c_bus_device_device_init(bus, bus_name);

    LOG_D("I2C bus [%s] registered", bus_name);

#ifdef RT_USING_DM
    if (!res)
    {
        i2c_bus_scan_clients(bus);
    }
#endif

    return res;
}

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name)
{
    struct rt_i2c_bus_device *bus;
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL || dev->type != RT_Device_Class_I2CBUS)
    {
        LOG_E("I2C bus %s not exist", bus_name);

        return RT_NULL;
    }

    bus = (struct rt_i2c_bus_device *)dev->user_data;

    return bus;
}

rt_ssize_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num)
{
    rt_ssize_t ret;
    rt_err_t err;

    if (bus->ops->master_xfer)
    {
#ifdef RT_I2C_DEBUG
        for (ret = 0; ret < num; ret++)
        {
            LOG_D("msgs[%d] %c, addr=0x%02x, len=%d", ret,
                  (msgs[ret].flags & RT_I2C_RD) ? 'R' : 'W',
                  msgs[ret].addr, msgs[ret].len);
        }
#endif
        err = rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
        if (err != RT_EOK)
        {
            return (rt_ssize_t)err;
        }
        ret = bus->ops->master_xfer(bus, msgs, num);
        err = rt_mutex_release(&bus->lock);
        if (err != RT_EOK)
        {
            return (rt_ssize_t)err;
        }
        return ret;
    }
    else
    {
        LOG_E("I2C bus operation not supported");
        return -RT_EINVAL;
    }
}

rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        int                       cmd,
                        void                      *args)
{
    rt_err_t ret;

    switch (cmd)
    {
        case RT_I2C_CTRL_SET_MAX_HZ:
        {
            if (args == RT_NULL)
            {
                return -RT_ERROR;
            }

            rt_uint32_t max_hz = *(rt_uint32_t *)args;
            if(max_hz > 0)
            {
                bus->config.max_hz = max_hz;
            }
            else
            {
                return -RT_ERROR;
            }
            break;
        }
        default:
        {
            if(bus->ops->i2c_bus_control)
            {
                ret = bus->ops->i2c_bus_control(bus, cmd, args);
                return ret;
            }
            else
            {
                LOG_E("I2C bus operation not supported");
                return -RT_EINVAL;
            }
            break;
        }
    }
    return RT_EOK;
}

rt_ssize_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count)
{
    rt_ssize_t ret;
    struct rt_i2c_msg msg;

    msg.addr  = addr;
    msg.flags = flags;
    msg.len   = count;
    msg.buf   = (rt_uint8_t *)buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret == 1) ? count : ret;
}

rt_ssize_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count)
{
    rt_ssize_t ret;
    struct rt_i2c_msg msg;
    RT_ASSERT(bus != RT_NULL);

    msg.addr   = addr;
    msg.flags  = flags | RT_I2C_RD;
    msg.len    = count;
    msg.buf    = buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret == 1) ? count : ret;
}
```

## Source: `data/rtos/rt-thread/components/drivers/i2c/dev_i2c_dev.c`

```c
/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2014-08-03     bernard       fix some compiling warning
 * 2021-04-20     RiceChen      added support for bus clock control
 */

#include <rtdevice.h>

#define DBG_TAG               "I2C"
#ifdef RT_I2C_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

static rt_ssize_t i2c_bus_device_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    LOG_D("I2C bus dev [%s] reading %u bytes.", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_recv(bus, addr, flags, (rt_uint8_t *)buffer, count);
}

static rt_ssize_t i2c_bus_device_write(rt_device_t dev,
                                      rt_off_t    pos,
                                      const void *buffer,
                                      rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    LOG_D("I2C bus dev [%s] writing %u bytes.", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_i2c_master_send(bus, addr, flags, (const rt_uint8_t *)buffer, count);
}

static rt_err_t i2c_bus_device_control(rt_device_t dev,
                                       int         cmd,
                                       void       *args)
{
    rt_err_t ret;
    struct rt_i2c_priv_data *priv_data;
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
    /* set 10-bit addr mode */
    case RT_I2C_DEV_CTRL_10BIT:
        bus->flags |= RT_I2C_ADDR_10BIT;
        break;
    case RT_I2C_DEV_CTRL_TIMEOUT:
        bus->timeout = *(rt_uint32_t *)args;
        break;
    case RT_I2C_DEV_CTRL_RW:
        priv_data = (struct rt_i2c_priv_data *)args;
        ret = rt_i2c_transfer(bus, priv_data->msgs, priv_data->number);
        if (ret < 0)
        {
            return -RT_EIO;
        }
        break;
    default:
        return rt_i2c_control(bus, cmd, args);
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops i2c_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    i2c_bus_device_read,
    i2c_bus_device_write,
    i2c_bus_device_control
};
#endif

rt_err_t rt_i2c_bus_device_device_init(struct rt_i2c_bus_device *bus,
                                       const char               *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_I2CBUS;
    /* initialize device interface */
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &i2c_ops;
#else
    device->init    = RT_NULL;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = i2c_bus_device_read;
    device->write   = i2c_bus_device_write;
    device->control = i2c_bus_device_control;
#endif

    /* register to device manager */
    rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}
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

## Source: `data/rtos/rt-thread/components/finsh/finsh.h`

```c
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-03-22     Bernard      first version
 */
#ifndef __FINSH_H__
#define __FINSH_H__

#include <rtdef.h>

#ifdef _MSC_VER
#pragma section("FSymTab$f",read)
#endif /* _MSC_VER */

#ifdef FINSH_USING_OPTION_COMPLETION
#define FINSH_COND(opt) opt,
#else
#define FINSH_COND(opt)
#endif

#ifdef FINSH_USING_DESCRIPTION
#define FINSH_DESC(cmd, desc) __fsym_##cmd##_desc,
#else
#define FINSH_DESC(cmd, desc)
#endif

typedef long (*syscall_func)(void);
#ifdef FINSH_USING_SYMTAB

#ifdef __TI_COMPILER_VERSION__
#define __TI_FINSH_EXPORT_FUNCTION(f)  PRAGMA(DATA_SECTION(f,"FSymTab"))
#endif /* __TI_COMPILER_VERSION__ */

/**
 * @brief Macro to export a command along with its name, description, and options to the symbol table in MSVC.
 *
 * @param[in] name The function name associated with the command.
 * @param[in] cmd The command name.
 * @param[in] desc The description of the command.
 * @param[in] opt The options associated with the command, used for option completion.
 */
#ifdef _MSC_VER
#define MSH_FUNCTION_EXPORT_CMD(name, cmd, desc, opt)               \
                const char __fsym_##cmd##_name[] = #cmd;            \
                const char __fsym_##cmd##_desc[] = #desc;           \
                __declspec(allocate("FSymTab$f"))                   \
                const struct finsh_syscall __fsym_##cmd =           \
                {                           \
                    __fsym_##cmd##_name,    \
                    FINSH_DESC(cmd, desc)   \
                    FINSH_COND(opt)         \
                    (syscall_func)&name     \
                };
#pragma comment(linker, "/merge:FSymTab=mytext")

#elif defined(__TI_COMPILER_VERSION__)
#ifdef __TMS320C28XX__
#define RT_NOBLOCKED __attribute__((noblocked))
#else
#define RT_NOBLOCKED
#endif
#define MSH_FUNCTION_EXPORT_CMD(name, cmd, desc, opt)                           \
                __TI_FINSH_EXPORT_FUNCTION(__fsym_##cmd);                       \
                const char __fsym_##cmd##_name[] = #cmd;                        \
                const char __fsym_##cmd##_desc[] = #desc;                       \
                rt_used RT_NOBLOCKED const struct finsh_syscall __fsym_##cmd =  \
                {                           \
                    __fsym_##cmd##_name,    \
                    FINSH_DESC(cmd, desc)   \
                    FINSH_COND(opt)         \
                    (syscall_func)&name     \
                };

#else
#define MSH_FUNCTION_EXPORT_CMD(name, cmd, desc, opt)                                  \
                const char __fsym_##cmd##_name[] rt_section(".rodata.name") = #cmd;    \
                const char __fsym_##cmd##_desc[] rt_section(".rodata.name") = #desc;   \
                rt_used const struct finsh_syscall __fsym_##cmd rt_section("FSymTab")= \
                {                           \
                    __fsym_##cmd##_name,    \
                    FINSH_DESC(cmd, desc)   \
                    FINSH_COND(opt)         \
                    (syscall_func)&name     \
                };

#endif  /* _MSC_VER */
#else
#define MSH_FUNCTION_EXPORT_CMD(name, cmd, desc, opt)
#endif /* FINSH_USING_SYMTAB */

/**
 * @brief Macro definitions to simplify the declaration of exported functions or commands.
 */
#define __MSH_GET_MACRO(_1, _2, _3, _FUN, ...)  _FUN
#define __MSH_GET_EXPORT_MACRO(_1, _2, _3, _4, _FUN, ...) _FUN

#define _MSH_FUNCTION_CMD2(a0, a1)       \
        MSH_FUNCTION_EXPORT_CMD(a0, a0, a1, 0)

#define _MSH_FUNCTION_CMD2_OPT(a0, a1, a2)       \
        MSH_FUNCTION_EXPORT_CMD(a0, a0, a1, a0##_msh_options)

#define _MSH_FUNCTION_EXPORT_CMD3(a0, a1, a2)       \
        MSH_FUNCTION_EXPORT_CMD(a0, a1, a2, 0)

#define _MSH_FUNCTION_EXPORT_CMD3_OPT(a0, a1, a2, a3)   \
        MSH_FUNCTION_EXPORT_CMD(a0, a1, a2, a0##_msh_options)


/**
 * @ingroup group_finsh
 *
 * @brief This macro exports a system function to finsh shell.
 *
 * @param[in] name Name of function.
 * @param[in] desc Description of function, which will show in help.
 */
#define FINSH_FUNCTION_EXPORT(name, desc)

/**
 * @ingroup group_finsh
 *
 * @brief Exports a system function with an alias name to finsh shell.
 *
 * @param[in] name Name of function.
 * @param[in] alias Alias name of function.
 * @param[in] desc Description of function, which will show in help.
 */
#define FINSH_FUNCTION_EXPORT_ALIAS(name, alias, desc)

/**
 * @ingroup group_finsh
 *
 * @brief Exports a command to module shell.
 *
 * @b Parameters
 *
 * <tt>[in]</tt> @b command Name of the command.
 *
 * <tt>[in]</tt> @b desc    Description of the command, which will show in help list.
 *
 * <tt>[in]</tt> @b opt     This is an option, enter any content to enable option completion
 *
 * @note This macro can be used in two ways:
 * @code MSH_CMD_EXPORT(command, desc) @endcode
 * or
 * @code MSH_CMD_EXPORT(command, desc, opt) @endcode
 */
#define MSH_CMD_EXPORT(...)                                 \
    __MSH_GET_MACRO(__VA_ARGS__, _MSH_FUNCTION_CMD2_OPT,    \
        _MSH_FUNCTION_CMD2)(__VA_ARGS__)

/**
 * @ingroup group_finsh
 *
 * @brief Exports a command with alias to module shell.
 *
 * @b Parameters
 *
 * <tt>[in]</tt> @b command Name of the command.
 *
 * <tt>[in]</tt> @b alias   Alias of the command.
 *
 * <tt>[in]</tt> @b desc    Description of the command, which will show in help list.
 *
 * <tt>[in]</tt> @b opt     An option, enter any content to enable option completion.
 *
 * @note This macro can be used in two ways:
 * @code #define MSH_CMD_EXPORT_ALIAS(command, alias, desc) @endcode
 * or
 * @code #define MSH_CMD_EXPORT_ALIAS(command, alias, desc, opt) @endcode
 */
#define MSH_CMD_EXPORT_ALIAS(...)                                           \
    __MSH_GET_EXPORT_MACRO(__VA_ARGS__, _MSH_FUNCTION_EXPORT_CMD3_OPT,      \
            _MSH_FUNCTION_EXPORT_CMD3)(__VA_ARGS__)

/* system call table */
struct finsh_syscall
{
    const char     *name;       /* the name of system call */
#if defined(FINSH_USING_DESCRIPTION) && defined(FINSH_USING_SYMTAB)
    const char     *desc;       /* description of system call */
#endif

#ifdef FINSH_USING_OPTION_COMPLETION
    struct msh_cmd_opt *opt;
#endif
    syscall_func func;      /* the function address of system call */
};

/* system call item */
struct finsh_syscall_item
{
    struct finsh_syscall_item *next;    /* next item */
    struct finsh_syscall syscall;       /* syscall */
};

#ifdef FINSH_USING_OPTION_COMPLETION
typedef struct msh_cmd_opt
{
    rt_uint32_t     id;
    const char      *name;
    const char      *des;
} msh_cmd_opt_t;

/* Command options declaration and definition macros */

/**
 * @brief Declares a static array of command options for a specific command.
 *
 * @param[in] command The command associated with these options.
 */
#ifdef _MSC_VER
#define CMD_OPTIONS_STATEMENT(command) static struct msh_cmd_opt command##_msh_options[16];
#else
#define CMD_OPTIONS_STATEMENT(command) static struct msh_cmd_opt command##_msh_options[];
#endif

/**
 * @brief Starts the definition of command options for a specific command.
 *
 * @param[in] command The command these options are associated with.
 */
#ifdef _MSC_VER
#define CMD_OPTIONS_NODE_START(command) static struct msh_cmd_opt command##_msh_options[16] = {
#else
#define CMD_OPTIONS_NODE_START(command) static struct msh_cmd_opt command##_msh_options[] = {
#endif

/**
 * @brief Defines a single command option.
 *
 * @param[in] _id Unique identifier for the option.
 * @param[in] _name The name of the option.
 * @param[in] _des Description of the option.
 */
#define CMD_OPTIONS_NODE(_id, _name, _des) {.id = _id, .name = #_name, .des = #_des},

/**
 * Marks the end of command options definition.
 */
#define CMD_OPTIONS_NODE_END    {0},};

void msh_opt_list_dump(void *options);
int msh_cmd_opt_id_get(int argc, char *argv[], void *options);
#define MSH_OPT_ID_GET(fun) msh_cmd_opt_id_get(argc, argv, (void*) fun##_msh_options)
#define MSH_OPT_DUMP(fun)   msh_opt_list_dump((void*) fun##_msh_options)

#else
#define CMD_OPTIONS_STATEMENT(command)
#define CMD_OPTIONS_NODE_START(command)
#define CMD_OPTIONS_NODE(_id, _name, _des)
#define CMD_OPTIONS_NODE_END
#define MSH_OPT_ID_GET(fun) ((int)(-1UL))
#define MSH_OPT_DUMP(fun)   do{}while(0)
#endif

extern struct finsh_syscall_item *global_syscall_list;
extern struct finsh_syscall *_syscall_table_begin, *_syscall_table_end;

#if defined(_MSC_VER) || (defined(__GNUC__) && defined(__x86_64__))
    struct finsh_syscall *finsh_syscall_next(struct finsh_syscall *call);
    #define FINSH_NEXT_SYSCALL(index)  index=finsh_syscall_next(index)
#else
    #define FINSH_NEXT_SYSCALL(index)  index++
#endif

#if !defined(RT_USING_POSIX_STDIO) && defined(RT_USING_DEVICE)
void finsh_set_device(const char *device_name);
#endif

#endif
```

## Source: `data/rtos/rt-thread/documentation/6.components/device-driver/i2c/i2c.md`

```
@page page_device_i2c I2C Bus Device

# Introduction of I2C

The I2C (Inter Integrated Circuit) bus is a half-duplex, bidirectional two-wire synchronous serial bus developed by Philips. The I2C bus has only two signal lines, one is the bidirectional data line SDA (serial data), and the other is the bidirectional clock line SCL (serial clock). Compared to the SPI bus, which has two lines for receiving data and transmitting data between the master and slave devices, the I2C bus uses only one line for data transmission and reception.

Like SPI, I2C works in a master-slave manner. Unlike SPI-master-multi-slave architecture, it allows multiple master devices to exist at the same time. Each device connected to the bus has a unique address, and the master device initiates data transfer, and generates a clock signal. The slave device is addressed by the master device, and only one master device is allowed to communicate at a time. As shown below:

![I2C Bus master-slave device connection mode](figures/i2c1.png)

The main data transmission format of the I2C bus is shown in the following figure:

![I2C Bus Data Transmission Format](figures/i2c2.png)

When the bus is idle, both SDA and SCL are in a high state. When the host wants to communicate with a slave, it will send a start condition first, then send the slave address and read and write control bits, and then transfer the data (the host can send or receive data). The host will send a stop condition when the data transfer ends. Each byte transmitted is 8 bits, with the high bit first and the low bit last. The different terms in the data transmission process are as follows:

* **Starting Condition：** When SCL is high, the host pulls SDA low, indicating that data transfer is about to begin.

* **Slave Address：** The first byte sent by the master is the slave address, the upper 7 bits are the address, the lowest bit is the R/W read/write control bit, R/W bit equals to 1 means the read operation, and 0 means the write operation. The general slave address has 7-bit address mode and 10-bit address mode. In the 10-bit address mode, the first 7 bits of the first byte are a combination of 11110XX, where the last two bits (XX) are two highest 10-bit addresses. The second byte is the remaining 8 bits of the 10-bit slave address, as shown in the following figure:

![7-bit address and 10-bit address format](figures/i2c3.png)

* **Answer Signal：** Each time a byte of data is transmitted, the receiver needs to reply with an ACK (acknowledge). The slave sends an ACK when writing data and the ACK by the host when reading data. When the host reads the last byte of data, it can send NACK (Not acknowledge) and then stop the condition.

* **Data：** After the slave address is sent, some commands may be sent, depending on the slave, and then the data transmission starts, and is sent by the master or the slave. Each data is 8 bits, and the number of bytes of data is not limited.

* **Repeat Start Condition：** In a communication process, when the host may need to transfer data with different slaves or need to switch read and write operations, the host can send another start condition.

* **Stop Condition：** When SDA is low, the master pulls SCL high and stays high, then pulls SDA high to indicate the end of the transfer.

# Access to I2C Bus Devices

In general, the MCU's I2C device communicates as a master and slave. In the RT-Thread, the I2C master is virtualized as an I2C bus device. The I2C slave communicates with the I2C bus through the I2C device interface. The related interfaces are as follows:

| **Function** | **Description**                |
| --------------- | ---------------------------------- |
| rt_device_find()  | Find device handles based on I2C bus device name |
| rt_i2c_transfer() | transfer data |

## Finding I2C Bus Device

Before using the I2C bus device, you need to obtain the device handle according to the I2C bus device name, so that you can operate the I2C bus device. The device function is as follows.

```c
rt_device_t rt_device_find(const char* name);
```

| Parameter | Description                |
| -------- | ---------------------------------- |
| name     | I2C bus device name      |
| **Return Value** | ——                                 |
| device handle | Finding the corresponding device will return the corresponding device handle |
| RT_NULL  | No corresponding device object found |

In general, the name of the I2C device registered to the system is i2c0, i2c1, etc. The usage examples are as follows:

```c
#define AHT10_I2C_BUS_NAME      "i2c1"  /* Sensor connected I2C bus device name */
struct rt_i2c_bus_device *i2c_bus;      /* I2C bus device handle */

/* Find the I2C bus device and get the I2C bus device handle */
i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);
```

## Data Transmission

You can use `rt_i2c_transfer()` for data transfer by getting the I2C bus device handle. The function prototype is as follows:

```c
rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num);
```

| Parameter | Description  |
|--------------------|----------------------|
| bus                | I2C bus device handle |
| msgs[]             | Message array pointer to be transmitted |
| num                | The number of elements in the message array |
| **Return Value** | ——                   |
| the number of elements in the message array | succeeded         |
| error code    | failed           |

Like the custom transport interface of the SPI bus, the data transmitted by the custom transport interface of the I2C bus is also in units of one message. The parameter msgs[] points to the array of messages to be transmitted. The user can customize the content of each message to implement two different data transmission modes supported by the I2C bus. If the master needs to send a repeat start condition, it will need to send 2 messages.

>This function will call rt_mutex_take(), which cannot be called inside the interrupt service routine, which will cause assertion to report an error.

The prototypes of the I2C message data structure are as follows:

```c
struct rt_i2c_msg
{
    rt_uint16_t addr;    /* Slave address */
    rt_uint16_t flags;   /* Reading, writing signs, etc. */
    rt_uint16_t len;     /* Read and write data bytes */
    rt_uint8_t  *buf;    /* Read and write data buffer pointer　*/
}
```

Slave address (addr): Supports 7-bit and 10-bit binary addresses. You need to view the data sheets of different devices.

>The slave address used by the RT-Thread I2C device interface does not contain read/write bits. The read/write bit control needs to modify the flag `flags`.

The flags `flags` can be defined as macros that can be combined with other macros using the bitwise operation "|" as needed.

```c
#define RT_I2C_WR              0x0000        /* Write flag */
#define RT_I2C_RD              (1u << 0)     /* Read flag */
#define RT_I2C_ADDR_10BIT      (1u << 2)     /* 10-bit address mode */
#define RT_I2C_NO_START        (1u << 4)     /* No start condition */
#define RT_I2C_IGNORE_NACK     (1u << 5)     /* Ignore NACK */
#define RT_I2C_NO_READ_ACK     (1u << 6)     /* Do not send ACK when reading */
```

Examples of use are as follows:

```c
#define AHT10_I2C_BUS_NAME      "i2c1"  /* Sensor connected I2C bus device name */
#define AHT10_ADDR               0x38   /* Slave address */
struct rt_i2c_bus_device *i2c_bus;      /* I2C bus device handle */

/* Find the I2C bus device and get the I2C bus device handle */
i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);

/* Read sensor register data */
static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AHT10_ADDR;     /* Slave address */
    msgs.flags = RT_I2C_RD;     /* Read flag */
    msgs.buf = buf;             /* Read and write data buffer pointer　*/
    msgs.len = len;             /* Read and write data bytes */

    /* Call the I2C device interface to transfer data */
    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}
```

# I2C Bus Device Usage Example

The specific usage of the I2C device can be referred to the following sample code. The main steps of the sample code are as follows:

1. First find the I2C name based on the I2C device name, get the device handle, and then initialize the aht10 sensor.
2. The two functions that control the sensor are the write sensor register `write_reg()` and the read sensor register `read_regs()`, both called `rt_i2c_transfer()` to transfer the data. The function `read_temp_humi()` calls the above two functions to read the temperature and humidity information.

```c
/*
 * Program listing: This is an I2C device usage routine
 * The routine exports the i2c_aht10_sample command to the control terminal
 * Command call format: i2c_aht10_sample i2c1
 * Command explanation: The second parameter of the command is the name of the I2C bus device to be used. If it is empty, the default I2C bus device is used.
 * Program function: read the temperature and humidity data of the aht10 sensor and print.
*/

#include <rtthread.h>
#include <rtdevice.h>

#define AHT10_I2C_BUS_NAME          "i2c1"  /* Sensor connected I2C bus device name */
#define AHT10_ADDR                  0x38    /* Slave address */
#define AHT10_CALIBRATION_CMD       0xE1    /* Calibration command */
#define AHT10_NORMAL_CMD            0xA8    /* General command */
#define AHT10_GET_DATA              0xAC    /* Get data command */

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;     /* I2C bus device handle */
static rt_bool_t initialized = RT_FALSE;                /* Sensor initialization status */

/* Write sensor register */
static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_uint8_t buf[3];
    struct rt_i2c_msg msgs;

    buf[0] = reg; //cmd
    buf[1] = data[0];
    buf[2] = data[1];

    msgs.addr = AHT10_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 3;

    /* Call the I2C device interface to transfer data */
    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/* Read sensor register data */
static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AHT10_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    /* Call the I2C device interface to transfer data */
    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static void read_temp_humi(float *cur_temp, float *cur_humi)
{
    rt_uint8_t temp[6];

    write_reg(i2c_bus, AHT10_GET_DATA, 0);      /* send command */
    rt_thread_mdelay(400);
    read_regs(i2c_bus, 6, temp);                /* obtian sensor data */

    /* Humidity data conversion */
    *cur_humi = (temp[1] << 12 | temp[2] << 4 | (temp[3] & 0xf0) >> 4) * 100.0 / (1 << 20);
    /* Temperature data conversion */
    *cur_temp = ((temp[3] & 0xf) << 16 | temp[4] << 8 | temp[5]) * 200.0 / (1 << 20) - 50;
}

static void aht10_init(const char *name)
{
    rt_uint8_t temp[2] = {0, 0};

    /* Find the I2C bus device and get the I2C bus device handle */
    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);

    if (i2c_bus == RT_NULL)
    {
        rt_kprintf("can't find %s device!\n", name);
    }
    else
    {
        write_reg(i2c_bus, AHT10_NORMAL_CMD, temp);
        rt_thread_mdelay(400);

        temp[0] = 0x08;
        temp[1] = 0x00;
        write_reg(i2c_bus, AHT10_CALIBRATION_CMD, temp);
        rt_thread_mdelay(400);
        initialized = RT_TRUE;
    }
}

static void i2c_aht10_sample(int argc, char *argv[])
{
    float humidity, temperature;
    char name[RT_NAME_MAX];

    humidity = 0.0;
    temperature = 0.0;

    if (argc == 2)
    {
        rt_strncpy(name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(name, AHT10_I2C_BUS_NAME, RT_NAME_MAX);
    }

    if (!initialized)
    {
        /* Sensor initialization */
        aht10_init(name);
    }
    if (initialized)
    {
        /* Read temperature and humidity data */
        read_temp_humi(&temperature, &humidity);

        rt_kprintf("read aht10 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
        rt_kprintf("read aht10 sensor temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10);
    }
    else
    {
        rt_kprintf("initialize sensor failed!\n");
    }
}
/* Export to the msh command list */
MSH_CMD_EXPORT(i2c_aht10_sample, i2c aht10 sample);
```
```
