# Raw RTOS/Bus Pack: rt-thread / uart_polling

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rt-thread/components/drivers/include/drivers/dev_serial.h`

```c
/*
 * Copyright (c) 2006-2024 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-05-15     lgnq         first version.
 * 2012-05-28     bernard      change interfaces
 * 2013-02-20     bernard      use RT_SERIAL_RB_BUFSZ to define
 *                             the size of ring buffer.
 */

#ifndef __DEV_SERIAL_H__
#define __DEV_SERIAL_H__

#include <rtthread.h>
/**
 * @defgroup    group_drivers_serial Serial
 * @brief       Serial driver api
 * @ingroup     group_device_driver
 *
 * <b>Example</b>
 * @code {.c}
 *
 * #include <rtthread.h>
 *
 * #define SAMPLE_UART_NAME       "uart2"
 * static struct rt_semaphore rx_sem;
 * static rt_device_t serial;
 *
 * static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
 * {
 *
 *     rt_sem_release(&rx_sem);
 *
 *     return RT_EOK;
 * }
 *
 * static void serial_thread_entry(void *parameter)
 * {
 *     char ch;
 *
 *     while (1)
 *     {
 *
 *         while (rt_device_read(serial, -1, &ch, 1) != 1)
 *         {
 *
 *             rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
 *         }
 *
 *         ch = ch + 1;
 *         rt_device_write(serial, 0, &ch, 1);
 *     }
 * }
 *
 * static int uart_sample(int argc, char *argv[])
 * {
 *     rt_err_t ret = RT_EOK;
 *     char uart_name[RT_NAME_MAX];
 *     char str[] = "hello RT-Thread!\r\n";
 *
 *     if (argc == 2)
 *     {
 *         rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
 *     }
 *     else
 *     {
 *         rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
 *     }
 *
 *
 *     serial = rt_device_find(uart_name);
 *     if (!serial)
 *     {
 *         rt_kprintf("find %s failed!\n", uart_name);
 *         return -RT_ERROR;
 *     }
 *
 *
 *     rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
 *
 *     rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
 *
 *     rt_device_set_rx_indicate(serial, uart_input);
 *
 *     rt_device_write(serial, 0, str, (sizeof(str) - 1));
 *
 *
 *     rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
 *
 *     if (thread != RT_NULL)
 *     {
 *         rt_thread_startup(thread);
 *     }
 *     else
 *     {
 *         ret = -RT_ERROR;
 *     }
 *
 *     return ret;
 * }
 *
 * MSH_CMD_EXPORT(uart_sample, uart device sample);
 * @endcode
 */


/*!
 * @addtogroup group_drivers_serial
 * @{
 */
#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_500000                500000
#define BAUD_RATE_576000                576000
#define BAUD_RATE_921600                921600
#define BAUD_RATE_1000000               1000000
#define BAUD_RATE_1152000               1152000
#define BAUD_RATE_1500000               1500000
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_2500000               2500000
#define BAUD_RATE_3000000               3000000
#define BAUD_RATE_3500000               3500000
#define BAUD_RATE_4000000               4000000

#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9

#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3

#ifdef _WIN32
#include <windows.h>
#else
#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2
#endif

#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1

#define NRZ_NORMAL                      0       /* Non Return to Zero : normal mode */
#define NRZ_INVERTED                    1       /* Non Return to Zero : inverted mode */

#ifndef RT_SERIAL_RB_BUFSZ
#define RT_SERIAL_RB_BUFSZ              64
#endif

#define RT_SERIAL_EVENT_RX_IND          0x01    /* Rx indication */
#define RT_SERIAL_EVENT_TX_DONE         0x02    /* Tx complete   */
#define RT_SERIAL_EVENT_RX_DMADONE      0x03    /* Rx DMA transfer done */
#define RT_SERIAL_EVENT_TX_DMADONE      0x04    /* Tx DMA transfer done */
#define RT_SERIAL_EVENT_RX_TIMEOUT      0x05    /* Rx timeout    */

#define RT_SERIAL_DMA_RX                0x01
#define RT_SERIAL_DMA_TX                0x02

#define RT_SERIAL_RX_INT                0x01
#define RT_SERIAL_TX_INT                0x02

#define RT_SERIAL_ERR_OVERRUN           0x01
#define RT_SERIAL_ERR_FRAMING           0x02
#define RT_SERIAL_ERR_PARITY            0x03

#define RT_SERIAL_TX_DATAQUEUE_SIZE     2048
#define RT_SERIAL_TX_DATAQUEUE_LWM      30

#define RT_SERIAL_FLOWCONTROL_CTSRTS    1
#define RT_SERIAL_FLOWCONTROL_NONE      0

/* Default config for serial_configure structure */
#define RT_SERIAL_CONFIG_DEFAULT           \
{                                          \
    BAUD_RATE_115200, /* 115200 bits/s */  \
    DATA_BITS_8,      /* 8 databits */     \
    STOP_BITS_1,      /* 1 stopbit */      \
    PARITY_NONE,      /* No parity  */     \
    BIT_ORDER_LSB,    /* LSB first sent */ \
    NRZ_NORMAL,       /* Normal mode */    \
    RT_SERIAL_RB_BUFSZ, /* Buffer size */  \
    RT_SERIAL_FLOWCONTROL_NONE, /* Off flowcontrol */ \
    0                                      \
}

/**
 * @brief Sets a hook function when RX indicate is called
 *
 */
typedef void (*rt_hw_serial_rxind_hookproto_t)(rt_device_t dev, rt_size_t size);
RT_OBJECT_HOOKLIST_DECLARE(rt_hw_serial_rxind_hookproto_t, rt_hw_serial_rxind);

struct serial_configure
{
    rt_uint32_t baud_rate;

    rt_uint32_t data_bits               :4;
    rt_uint32_t stop_bits               :2;
    rt_uint32_t parity                  :2;
    rt_uint32_t bit_order               :1;
    rt_uint32_t invert                  :1;
    rt_uint32_t bufsz                   :16;
    rt_uint32_t flowcontrol             :1;
    rt_uint32_t reserved                :5;
};

/*
 * Serial FIFO mode
 */
struct rt_serial_rx_fifo
{
    /* software fifo */
    rt_uint8_t *buffer;

    rt_uint16_t put_index, get_index;

    rt_bool_t is_full;
};

struct rt_serial_tx_fifo
{
    struct rt_completion completion;
};

/*
 * Serial DMA mode
 */
struct rt_serial_rx_dma
{
    rt_bool_t activated;
};

struct rt_serial_tx_dma
{
    rt_bool_t activated;
    struct rt_data_queue data_queue;
};

struct rt_serial_device
{
    struct rt_device          parent;

    const struct rt_uart_ops *ops;
    struct serial_configure   config;

    void *serial_rx;
    void *serial_tx;

    struct rt_spinlock spinlock;
#ifdef RT_USING_SERIAL_BYPASS
    struct rt_serial_bypass* bypass;
#endif
    struct rt_device_notify rx_notify;
};
typedef struct rt_serial_device rt_serial_t;

/**
 * @brief Configure the serial device
 */
struct rt_uart_ops
{
    rt_err_t (*configure)(struct rt_serial_device *serial, struct serial_configure *cfg);
    rt_err_t (*control)(struct rt_serial_device *serial, int cmd, void *arg);

    int (*putc)(struct rt_serial_device *serial, char c);
    int (*getc)(struct rt_serial_device *serial);

    rt_ssize_t (*dma_transmit)(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, int direction);
};

/**
 * @brief Serial interrupt service routine
 * @param serial    serial device
 * @param event     event mask
 * @ingroup group_drivers_serial
 */
void rt_hw_serial_isr(struct rt_serial_device *serial, int event);

/**
 * @brief Register a serial device to device list
 *
 * @param serial    serial device
 * @param name      device name
 * @param flag      device flag
 * @param data      device private data
 * @return rt_err_t        error code
 * @note This function will register a serial device to system device list,
 *       and add a device object to system object list.
 * @ingroup group_drivers_serial
 */
rt_err_t rt_hw_serial_register(struct rt_serial_device *serial,
                               const char              *name,
                               rt_uint32_t              flag,
                               void                    *data);

/**
 * @brief     register a serial device to system device list and add a device object to system object list
 *
 * @param serial    serial device
 * @return rt_err_t error code
 *
 * @ingroup group_drivers_serial
 */
rt_err_t rt_hw_serial_register_tty(struct rt_serial_device *serial);

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

## Source: `data/rtos/rt-thread/components/drivers/serial/dev_serial.c`

```c
/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-03-13     bernard      first version
 * 2012-05-15     lgnq         modified according bernard's implementation.
 * 2012-05-28     bernard      code cleanup
 * 2012-11-23     bernard      fix compiler warning.
 * 2013-02-20     bernard      use RT_SERIAL_RB_BUFSZ to define
 *                             the size of ring buffer.
 * 2014-07-10     bernard      rewrite serial framework
 * 2014-12-31     bernard      use open_flag for poll_tx stream mode.
 * 2015-05-19     Quintin      fix DMA tx mod tx_dma->activated flag !=RT_FALSE BUG
 *                             in open function.
 * 2015-11-10     bernard      fix the poll rx issue when there is no data.
 * 2016-05-10     armink       add fifo mode to DMA rx when serial->config.bufsz != 0.
 * 2017-01-19     aubr.cool    prevent change serial rx bufsz when serial is opened.
 * 2017-11-07     JasonJia     fix data bits error issue when using tcsetattr.
 * 2017-11-15     JasonJia     fix poll rx issue when data is full.
 *                             add TCFLSH and FIONREAD support.
 * 2018-12-08     Ernest Chen  add DMA choice
 * 2020-09-14     WillianChan  add a line feed to the carriage return character
 *                             when using interrupt tx
 * 2020-12-14     Meco Man     implement function of setting window's size(TIOCSWINSZ)
 * 2021-08-22     Meco Man     implement function of getting window's size(TIOCGWINSZ)
 * 2023-09-15     xqyjlj       perf rt_hw_interrupt_disable/enable
 * 2024-11-25     zhujiale     add bypass mode
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG    "UART"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifdef RT_USING_POSIX_STDIO
#include <dfs_file.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>

#ifdef RT_USING_POSIX_TERMIOS
#include <termios.h>
#endif

/* it's possible the 'getc/putc' is defined by stdio.h in gcc/newlib. */
#ifdef getc
#undef getc
#endif

#ifdef putc
#undef putc
#endif

RT_OBJECT_HOOKLIST_DEFINE(rt_hw_serial_rxind);

static rt_err_t serial_fops_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_wqueue_wakeup(&(dev->wait_queue), (void*)POLLIN);

    RT_OBJECT_HOOKLIST_CALL(rt_hw_serial_rxind, (dev, size));

    return RT_EOK;
}

/* fops for serial */
static int serial_fops_open(struct dfs_file *fd)
{
    rt_err_t ret = 0;
    rt_uint16_t flags = 0;
    rt_device_t device;

    device = (rt_device_t)fd->vnode->data;
    RT_ASSERT(device != RT_NULL);

    switch (fd->flags & O_ACCMODE)
    {
        case O_RDONLY:
            LOG_D("fops open: O_RDONLY!");
            flags = RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDONLY;
            break;
        case O_WRONLY:
            LOG_D("fops open: O_WRONLY!");
            flags = RT_DEVICE_FLAG_WRONLY;
            break;
        case O_RDWR:
            LOG_D("fops open: O_RDWR!");
            flags = RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR;
            break;
        default:
            LOG_E("fops open: unknown mode - %d!", fd->flags & O_ACCMODE);
            break;
    }

    if ((fd->flags & O_ACCMODE) != O_WRONLY)
        rt_device_set_rx_indicate(device, serial_fops_rx_ind);
    ret = rt_device_open(device, flags);
    if (ret == RT_EOK) return 0;

    return ret;
}

static int serial_fops_close(struct dfs_file *fd)
{
    rt_device_t device;

    device = (rt_device_t)fd->vnode->data;

    rt_device_set_rx_indicate(device, RT_NULL);
    rt_device_close(device);

    return 0;
}

static int serial_fops_ioctl(struct dfs_file *fd, int cmd, void *args)
{
    rt_device_t device;
    int flags = (int)(rt_base_t)args;
    int mask  = O_NONBLOCK | O_APPEND;

    device = (rt_device_t)fd->vnode->data;
    switch ((rt_ubase_t)cmd)
    {
    case FIONREAD:
        break;
    case FIONWRITE:
        break;
    case F_SETFL:
        flags &= mask;
        fd->flags &= ~mask;
        fd->flags |= flags;
        break;
    }

    return rt_device_control(device, cmd, args);
}

#ifdef RT_USING_DFS_V2
static ssize_t serial_fops_read(struct dfs_file *fd, void *buf, size_t count, off_t *pos)
#else
static ssize_t serial_fops_read(struct dfs_file *fd, void *buf, size_t count)
#endif
{
    int size = 0;
    rt_device_t device;
    int wait_ret;

    device = (rt_device_t)fd->vnode->data;

    do
    {
        size = rt_device_read(device, -1,  buf, count);
        if (size <= 0)
        {
            if (fd->flags & O_NONBLOCK)
            {
                size = -EAGAIN;
                break;
            }

            wait_ret = rt_wqueue_wait_interruptible(&(device->wait_queue), 0, RT_WAITING_FOREVER);
            if (wait_ret != RT_EOK)
            {
                break;
            }
        }
    }while (size <= 0);

    if (size < 0)
    {
        size = 0;
    }
    return size;
}

#ifdef RT_USING_DFS_V2
static ssize_t serial_fops_write(struct dfs_file *fd, const void *buf, size_t count, off_t *pos)
#else
static ssize_t serial_fops_write(struct dfs_file *fd, const void *buf, size_t count)
#endif
{
    rt_device_t device;

    device = (rt_device_t)fd->vnode->data;
    return rt_device_write(device, -1, buf, count);
}

static int serial_fops_poll(struct dfs_file *fd, struct rt_pollreq *req)
{
    int mask = 0;
    int flags = 0;
    rt_device_t device;
    struct rt_serial_device *serial;

    device = (rt_device_t)fd->vnode->data;
    RT_ASSERT(device != RT_NULL);

    serial = (struct rt_serial_device *)device;

    /* only support POLLIN */
    flags = fd->flags & O_ACCMODE;
    if (flags == O_RDONLY || flags == O_RDWR)
    {
        rt_base_t level;
        struct rt_serial_rx_fifo* rx_fifo;

        rt_poll_add(&(device->wait_queue), req);

        rx_fifo = (struct rt_serial_rx_fifo*) serial->serial_rx;

        level = rt_spin_lock_irqsave(&(serial->spinlock));
        if ((rx_fifo->get_index != rx_fifo->put_index) || (rx_fifo->get_index == rx_fifo->put_index && rx_fifo->is_full == RT_TRUE))
            mask |= POLLIN;
        rt_spin_unlock_irqrestore(&(serial->spinlock), level);
    }

    return mask;
}

static const struct dfs_file_ops _serial_fops =
{
    .open   = serial_fops_open,
    .close  = serial_fops_close,
    .ioctl  = serial_fops_ioctl,
    .read   = serial_fops_read,
    .write  = serial_fops_write,
    .poll   = serial_fops_poll,
};
#endif /* RT_USING_POSIX_STDIO */

/*
 * Serial poll routines
 */
rt_inline int _serial_poll_rx(struct rt_serial_device *serial, rt_uint8_t *data, int length)
{
    int ch;
    int size;

    RT_ASSERT(serial != RT_NULL);
    size = length;

    while (length)
    {
        ch = serial->ops->getc(serial);
        if (ch == -1) break;

        *data = ch;
        data ++; length --;

        if(serial->parent.open_flag & RT_DEVICE_FLAG_STREAM)
        {
            if (ch == '\n') break;
        }
    }

    return size - length;
}

rt_inline int _serial_poll_tx(struct rt_serial_device *serial, const rt_uint8_t *data, int length)
{
    int size;
    RT_ASSERT(serial != RT_NULL);

    size = length;
    while (length)
    {
        /*
         * to be polite with serial console add a line feed
         * to the carriage return character
         */
        if (*data == '\n' && (serial->parent.open_flag & RT_DEVICE_FLAG_STREAM))
        {
            serial->ops->putc(serial, '\r');
        }

        if(serial->ops->putc(serial, *data) < 0) {
            break;
        }

        ++ data;
        -- length;
    }

    return size - length;
}

/*
 * Serial interrupt routines
 */
rt_inline int _serial_int_rx(struct rt_serial_device *serial, rt_uint8_t *data, int length)
{
    int size;
    struct rt_serial_rx_fifo* rx_fifo;

    RT_ASSERT(serial != RT_NULL);
    size = length;

    rx_fifo = (struct rt_serial_rx_fifo*) serial->serial_rx;
    RT_ASSERT(rx_fifo != RT_NULL);

#ifdef RT_USING_SERIAL_BYPASS
    if (serial->bypass)
    {
        rt_bypass_work_straight(serial);
        while (length)
        {
            rt_uint8_t ch;
            if (!rt_bypass_getchar(serial, &ch))
                break;

            *data = ch & 0xff;
            data++; length--;
        }

        return size - length;
    }
#endif
    /* read from software FIFO */
    while (length)
    {
        int ch;
        rt_base_t level;

        /* disable interrupt */
        level = rt_spin_lock_irqsave(&(serial->spinlock));

        /* there's no data: */
        if ((rx_fifo->get_index == rx_fifo->put_index) && (rx_fifo->is_full == RT_FALSE))
        {
            /* no data, enable interrupt and break out */
            rt_spin_unlock_irqrestore(&(serial->spinlock), level);
            break;
        }

        /* otherwise there's the data: */
        ch = rx_fifo->buffer[rx_fifo->get_index];
        rx_fifo->get_index += 1;
        if (rx_fifo->get_index >= serial->config.bufsz) rx_fifo->get_index = 0;

        if (rx_fifo->is_full == RT_TRUE)
        {
            rx_fifo->is_full = RT_FALSE;
        }

        /* enable interrupt */
        rt_spin_unlock_irqrestore(&(serial->spinlock), level);

        *data = ch & 0xff;
        data ++; length --;
    }

    return size - length;
}

rt_inline int _serial_int_tx(struct rt_serial_device *serial, const rt_uint8_t *data, int length)
{
    int size;
    struct rt_serial_tx_fifo *tx;

    RT_ASSERT(serial != RT_NULL);

    size = length;
    tx = (struct rt_serial_tx_fifo*) serial->serial_tx;
    RT_ASSERT(tx != RT_NULL);

    while (length)
    {
        /*
         * to be polite with serial console add a line feed
         * to the carriage return character
         */
        if (*data == '\n' && (serial->parent.open_flag & RT_DEVICE_FLAG_STREAM))
        {
            if (serial->ops->putc(serial, '\r') == -1)
            {
                rt_completion_wait(&(tx->completion), RT_WAITING_FOREVER);
                continue;
            }
        }

        while (serial->ops->putc(serial, *(char*)data) == -1)
        {
            rt_completion_wait(&(tx->completion), RT_WAITING_FOREVER);
        }

        data ++; length --;
    }

    return size - length;
}

static void _serial_check_buffer_size(void)
{
    static rt_bool_t already_output = RT_FALSE;

    if (already_output == RT_FALSE)
    {
#if !defined(RT_USING_ULOG) || defined(ULOG_USING_ISR_LOG)
        LOG_W("Warning: There is no enough buffer for saving data,"
              " please increase the RT_SERIAL_RB_BUFSZ option.");
#endif
        already_output = RT_TRUE;
    }
}

#if defined(RT_USING_POSIX_STDIO) || defined(RT_SERIAL_USING_DMA)
static rt_ssize_t _serial_fifo_calc_recved_len(struct rt_serial_device *serial)
{
    struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *) serial->serial_rx;

    RT_ASSERT(rx_fifo != RT_NULL);

    if (rx_fifo->put_index == rx_fifo->get_index)
    {
        return (rx_fifo->is_full == RT_FALSE ? 0 : serial->config.bufsz);
    }
    else
    {
        if (rx_fifo->put_index > rx_fifo->get_index)
        {
            return rx_fifo->put_index - rx_fifo->get_index;
        }
        else
        {
            return serial->config.bufsz - (rx_fifo->get_index - rx_fifo->put_index);
        }
    }
}
#endif /* RT_USING_POSIX_STDIO || RT_SERIAL_USING_DMA */

#ifdef RT_SERIAL_USING_DMA
/**
 * Calculate DMA received data length.
 *
 * @param serial serial device
 *
 * @return length
 */
static rt_ssize_t rt_dma_calc_recved_len(struct rt_serial_device *serial)
{
    return _serial_fifo_calc_recved_len(serial);
}

/**
 * Read data finish by DMA mode then update the get index for receive fifo.
 *
 * @param serial serial device
 * @param len get data length for this operate
 */
static void rt_dma_recv_update_get_index(struct rt_serial_device *serial, rt_size_t len)
{
    struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *) serial->serial_rx;

    RT_ASSERT(rx_fifo != RT_NULL);
    RT_ASSERT(len <= rt_dma_calc_recved_len(serial));

    if (rx_fifo->is_full && len != 0) rx_fifo->is_full = RT_FALSE;

    rx_fifo->get_index += (rt_uint16_t)len;
    if (rx_fifo->get_index >= serial->config.bufsz)
    {
        rx_fifo->get_index %= serial->config.bufsz;
    }
}

/**
 * DMA received finish then update put index for receive fifo.
 *
 * @param serial serial device
 * @param len received length for this transmit
 */
static void rt_dma_recv_update_put_index(struct rt_serial_device *serial, rt_size_t len)
{
    struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *)serial->serial_rx;

    RT_ASSERT(rx_fifo != RT_NULL);

    if (rx_fifo->get_index <= rx_fifo->put_index)
    {
        rx_fifo->put_index += (rt_uint16_t)len;
        /* beyond the fifo end */
        if (rx_fifo->put_index >= serial->config.bufsz)
        {
            rx_fifo->put_index %= serial->config.bufsz;
            /* force overwrite get index */
            if (rx_fifo->put_index >= rx_fifo->get_index)
            {
                rx_fifo->is_full = RT_TRUE;
            }
        }
    }
    else
    {
        rx_fifo->put_index += (rt_uint16_t)len;
        if (rx_fifo->put_index >= rx_fifo->get_index)
        {
            /* beyond the fifo end */
            if (rx_fifo->put_index >= serial->config.bufsz)
            {
                rx_fifo->put_index %= serial->config.bufsz;
            }
            /* force overwrite get index */
            rx_fifo->is_full = RT_TRUE;
        }
    }

    if(rx_fifo->is_full == RT_TRUE)
    {
        _serial_check_buffer_size();
        rx_fifo->get_index = rx_fifo->put_index;
    }
}

/*
 * Serial DMA routines
 */
rt_inline int _serial_dma_rx(struct rt_serial_device *serial, rt_uint8_t *data, int length)
{
    rt_base_t level;

    RT_ASSERT((serial != RT_NULL) && (data != RT_NULL));

    level = rt_spin_lock_irqsave(&(serial->spinlock));

    if (serial->config.bufsz == 0)
    {
        int result = RT_EOK;
        struct rt_serial_rx_dma *rx_dma;

        rx_dma = (struct rt_serial_rx_dma*)serial->serial_rx;
        RT_ASSERT(rx_dma != RT_NULL);

        if (rx_dma->activated != RT_TRUE)
        {
            rx_dma->activated = RT_TRUE;
            RT_ASSERT(serial->ops->dma_transmit != RT_NULL);
            serial->ops->dma_transmit(serial, data, length, RT_SERIAL_DMA_RX);
        }
        else result = -RT_EBUSY;
        rt_spin_unlock_irqrestore(&(serial->spinlock), level);

        if (result == RT_EOK) return length;

        rt_set_errno(result);
        return 0;
    }
    else
    {
        struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *) serial->serial_rx;
        rt_size_t recv_len = 0, fifo_recved_len = rt_dma_calc_recved_len(serial);

        RT_ASSERT(rx_fifo != RT_NULL);

        if (length < (int)fifo_recved_len)
            recv_len = length;
        else
            recv_len = fifo_recved_len;

        if (rx_fifo->get_index + recv_len < serial->config.bufsz)
            rt_memcpy(data, rx_fifo->buffer + rx_fifo->get_index, recv_len);
        else
        {
            rt_memcpy(data, rx_fifo->buffer + rx_fifo->get_index,
                    serial->config.bufsz - rx_fifo->get_index);
            rt_memcpy(data + serial->config.bufsz - rx_fifo->get_index, rx_fifo->buffer,
                    recv_len + rx_fifo->get_index - serial->config.bufsz);
        }
        rt_dma_recv_update_get_index(serial, recv_len);
        rt_spin_unlock_irqrestore(&(serial->spinlock), level);
        return recv_len;
    }
}

rt_inline int _serial_dma_tx(struct rt_serial_device *serial, const rt_uint8_t *data, int length)
{
    rt_base_t level;
    rt_err_t result;
    struct rt_serial_tx_dma *tx_dma;

    tx_dma = (struct rt_serial_tx_dma*)(serial->serial_tx);

    result = rt_data_queue_push(&(tx_dma->data_queue), data, length, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        level = rt_spin_lock_irqsave(&(serial->spinlock));
        if (tx_dma->activated != RT_TRUE)
        {
            tx_dma->activated = RT_TRUE;
            rt_spin_unlock_irqrestore(&(serial->spinlock), level);

            /* make a DMA transfer */
            serial->ops->dma_transmit(serial, (rt_uint8_t *)data, length, RT_SERIAL_DMA_TX);
        }
        else
        {
            rt_spin_unlock_irqrestore(&(serial->spinlock), level);
        }

        return length;
    }
    else
    {
        rt_set_errno(result);
        return 0;
    }
}
#endif /* RT_SERIAL_USING_DMA */

/* RT-Thread Device Interface */
/*
 * This function initializes serial device.
 */
static rt_err_t rt_serial_init(struct rt_device *dev)
{
    rt_err_t result = RT_EOK;
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    /* initialize rx/tx */
    serial->serial_rx = RT_NULL;
    serial->serial_tx = RT_NULL;

    rt_memset(&serial->rx_notify, 0, sizeof(struct rt_device_notify));

    /* apply configuration */
    if (serial->ops->configure)
        result = serial->ops->configure(serial, &serial->config);

    return result;
}

static rt_err_t rt_serial_open(struct rt_device *dev, rt_uint16_t oflag)
{
    rt_uint16_t stream_flag = 0;
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    LOG_D("open serial device: 0x%08x with open flag: 0x%04x",
        dev, oflag);
    /* check device flag with the open flag */
    if ((oflag & RT_DEVICE_FLAG_DMA_RX) && !(dev->flag & RT_DEVICE_FLAG_DMA_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_DMA_TX) && !(dev->flag & RT_DEVICE_FLAG_DMA_TX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_RX) && !(dev->flag & RT_DEVICE_FLAG_INT_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_TX) && !(dev->flag & RT_DEVICE_FLAG_INT_TX))
        return -RT_EIO;

    /* keep steam flag */
    if ((oflag & RT_DEVICE_FLAG_STREAM) || (dev->open_flag & RT_DEVICE_FLAG_STREAM))
        stream_flag = RT_DEVICE_FLAG_STREAM;

    /* get open flags */
    dev->open_flag = oflag & 0xff;

    /* initialize the Rx/Tx structure according to open flag */
    if (serial->serial_rx == RT_NULL)
    {
        if (oflag & RT_DEVICE_FLAG_INT_RX)
        {
            struct rt_serial_rx_fifo* rx_fifo;

            rx_fifo = (struct rt_serial_rx_fifo*) rt_malloc (sizeof(struct rt_serial_rx_fifo) +
                serial->config.bufsz);
            RT_ASSERT(rx_fifo != RT_NULL);
            rx_fifo->buffer = (rt_uint8_t*) (rx_fifo + 1);
            rt_memset(rx_fifo->buffer, 0, serial->config.bufsz);
            rx_fifo->put_index = 0;
            rx_fifo->get_index = 0;
            rx_fifo->is_full = RT_FALSE;

            serial->serial_rx = rx_fifo;
            dev->open_flag |= RT_DEVICE_FLAG_INT_RX;
            /* configure low level device */
            serial->ops->control(serial, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        }
#ifdef RT_SERIAL_USING_DMA
        else if (oflag & RT_DEVICE_FLAG_DMA_RX)
        {
            if (serial->config.bufsz == 0) {
                struct rt_serial_rx_dma* rx_dma;

                rx_dma = (struct rt_serial_rx_dma*) rt_malloc (sizeof(struct rt_serial_rx_dma));
                RT_ASSERT(rx_dma != RT_NULL);
                rx_dma->activated = RT_FALSE;

                serial->serial_rx = rx_dma;
            } else {
                struct rt_serial_rx_fifo* rx_fifo;

                rx_fifo = (struct rt_serial_rx_fifo*) rt_malloc (sizeof(struct rt_serial_rx_fifo) +
                    serial->config.bufsz);
                RT_ASSERT(rx_fifo != RT_NULL);
                rx_fifo->buffer = (rt_uint8_t*) (rx_fifo + 1);
                rt_memset(rx_fifo->buffer, 0, serial->config.bufsz);
                rx_fifo->put_index = 0;
                rx_fifo->get_index = 0;
                rx_fifo->is_full = RT_FALSE;
                serial->serial_rx = rx_fifo;
                /* configure fifo address and length to low level device */
                serial->ops->control(serial, RT_DEVICE_CTRL_CONFIG, (void *) RT_DEVICE_FLAG_DMA_RX);
            }
            dev->open_flag |= RT_DEVICE_FLAG_DMA_RX;
        }
#endif /* RT_SERIAL_USING_DMA */
        else
        {
            serial->serial_rx = RT_NULL;
        }
    }
    else
    {
        if (oflag & RT_DEVICE_FLAG_INT_RX)
            dev->open_flag |= RT_DEVICE_FLAG_INT_RX;
#ifdef RT_SERIAL_USING_DMA
        else if (oflag & RT_DEVICE_FLAG_DMA_RX)
            dev->open_flag |= RT_DEVICE_FLAG_DMA_RX;
#endif /* RT_SERIAL_USING_DMA */
    }

    if (serial->serial_tx == RT_NULL)
    {
        if (oflag & RT_DEVICE_FLAG_INT_TX)
        {
            struct rt_serial_tx_fifo *tx_fifo;

            tx_fifo = (struct rt_serial_tx_fifo*) rt_malloc(sizeof(struct rt_serial_tx_fifo));
            RT_ASSERT(tx_fifo != RT_NULL);

            rt_completion_init(&(tx_fifo->completion));
            serial->serial_tx = tx_fifo;

            dev->open_flag |= RT_DEVICE_FLAG_INT_TX;
            /* configure low level device */
            serial->ops->control(serial, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_TX);
        }
#ifdef RT_SERIAL_USING_DMA
        else if (oflag & RT_DEVICE_FLAG_DMA_TX)
        {
            struct rt_serial_tx_dma* tx_dma;

            tx_dma = (struct rt_serial_tx_dma*) rt_malloc (sizeof(struct rt_serial_tx_dma));
            RT_ASSERT(tx_dma != RT_NULL);
            tx_dma->activated = RT_FALSE;

            rt_data_queue_init(&(tx_dma->data_queue), 8, 4, RT_NULL);
            serial->serial_tx = tx_dma;

            dev->open_flag |= RT_DEVICE_FLAG_DMA_TX;
            /* configure low level device */
            serial->ops->control(serial, RT_DEVICE_CTRL_CONFIG, (void *)RT_DEVICE_FLAG_DMA_TX);
        }
#endif /* RT_SERIAL_USING_DMA */
        else
        {
            serial->serial_tx = RT_NULL;
        }
    }
    else
    {
        if (oflag & RT_DEVICE_FLAG_INT_TX)
            dev->open_flag |= RT_DEVICE_FLAG_INT_TX;
#ifdef RT_SERIAL_USING_DMA
        else if (oflag & RT_DEVICE_FLAG_DMA_TX)
            dev->open_flag |= RT_DEVICE_FLAG_DMA_TX;
#endif /* RT_SERIAL_USING_DMA */
    }

    /* set stream flag */
    dev->open_flag |= stream_flag;

    return RT_EOK;
}

static rt_err_t rt_serial_close(struct rt_device *dev)
{
    struct rt_serial_device *serial;

    RT_ASSERT(dev != RT_NULL);
    serial = (struct rt_serial_device *)dev;

    /* this device has more reference count */
    if (dev->ref_count > 1) return RT_EOK;

    if (dev->open_flag & RT_DEVICE_FLAG_INT_RX)
    {
        struct rt_serial_rx_fifo* rx_fifo;

        /* configure low level device */
        serial->ops->control(serial, RT_DEVICE_CTRL_CLR_INT, (void*)RT_DEVICE_FLAG_INT_RX);
        dev->open_flag &= ~RT_DEVICE_FLAG_INT_RX;

        rx_fifo = (struct rt_serial_rx_fifo*)serial->serial_rx;
        RT_ASSERT(rx_fifo != RT_NULL);

        rt_free(rx_fifo);
        serial->serial_rx = RT_NULL;

    }
#ifdef RT_SERIAL_USING_DMA
    else if (dev->open_flag & RT_DEVICE_FLAG_DMA_RX)
    {
        /* configure low level device */
        serial->ops->control(serial, RT_DEVICE_CTRL_CLR_INT, (void *) RT_DEVICE_FLAG_DMA_RX);
        dev->open_flag &= ~RT_DEVICE_FLAG_DMA_RX;

        if (serial->config.bufsz == 0)
        {
            struct rt_serial_rx_dma* rx_dma;

            rx_dma = (struct rt_serial_rx_dma*)serial->serial_rx;
            RT_ASSERT(rx_dma != RT_NULL);

            rt_free(rx_dma);
        }
        else
        {
            struct rt_serial_rx_fifo* rx_fifo;

            rx_fifo = (struct rt_serial_rx_fifo*)serial->serial_rx;
            RT_ASSERT(rx_fifo != RT_NULL);

            rt_free(rx_fifo);
        }
        serial->serial_rx = RT_NULL;

    }
#endif /* RT_SERIAL_USING_DMA */

    if (dev->open_flag & RT_DEVICE_FLAG_INT_TX)
    {
        struct rt_serial_tx_fifo* tx_fifo;

        serial->ops->control(serial, RT_DEVICE_CTRL_CLR_INT, (void*)RT_DEVICE_FLAG_INT_TX);
      
/* ... truncated ... */
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

## Source: `data/rtos/rt-thread/documentation/6.components/device-driver/uart/uart.md`

```
@page page_device_uart UART Device

# UART Introduction

UART (Universal Asynchronous Receiver/Transmitter), as a kind of asynchronous serial communication protocol, the working principle is to transmit each character of the transmitted data one by one. It is the most frequently used data bus during application development.

The UART serial port is characterized by sequentially transmitting data one bit at a time. As long as  two transmission lines can realize two-way communication, one line transmits data while the other receives data . There are several important functions for UART serial communication, namely baud rate, start bit, data bit, stop bit and parity bit. For two ports that use UART serial port communication, these functions must be matched, otherwise the communication can't be carried out normally. The data format of the UART serial port transmission is as shown below:

![Serial Transmission Data Format](figures/uart1.png)

* Start bit: Indicates the start of data transfer and the level logic is "0".
- Data bits: Possible values are 5, 6, 7, 8, and 9, indicating that these bits are transmitted. The value is generally 8, because an ASCII character value is 8 bits.
- Parity check bit: It it used by the receiver to verify the received data. The number of bits is used in the check of "1" is even (even parity) or odd (odd parity) ,in order to verify the data transmission. It is also fine by not using this bit .
- Stop Bit: Indicates the end of one frame of data. The level logic is "1".
- Baudrate: It is the rate at which a serial port communicates, which expressed in bits per second (bps) of the binary code transmitted in unit time. The common baud rate values are 4800, 9600, 14400, 38400, 115200, etc. The higher the value is, the faster the data transmission will be.

# Access UART Device

The application accesses the serial port hardware through the I/O device management interface provided by RT-Thread. The related interfaces are as follows:

| **Funtion** | **Description**                |
| --------------------------- | -------------------------- |
| rt_device_find()  | find device |
| rt_device_open()     | open device |
| rt_device_read()   | read device |
| rt_device_write()  |write device|
| rt_device_control()  | control device |
| rt_device_set_rx_indicate()   | set receive callback function |
| rt_device_set_tx_complete()  | set send complete callback function |
| rt_device_close()     | close device |

## Find UART Device

The application obtains the device handle according to the uart device name, and then can operate the uart device.The device find function is shown below

```c
rt_device_t rt_device_find(const char* name);
```

| **Parameter** | **Description**                                              |
| ------------- | ------------------------------------------------------------ |
| name          | device's name                                                |
| **back**      | ——                                                           |
| device handle | finding the corresponding device will return to the corresponding device handle |
| RT_NULL       | corresponding device object was not found                    |

Generally, the name of the uart device registered to the system is uart0, uart1, etc. samples are as follows:

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device name */
static rt_device_t serial;              /* uart device handle */
/* Find uart device*/
serial = rt_device_find(SAMPLE_UART_NAME);
```

## Open UART Device

Through the device handle, the application can open and close the device. When the device is opened, it will detect whether the device has been initialized. If it is not initialized, it will call the initialization interface to initialize the device by default. Open the device through the following functions:

```c
rt_err_t rt_device_open(rt_device_t dev, rt_uint16_t oflags);
```

| **Parameter** | **Description**               |
| ---------- | ------------------------------- |
| dev        | device handle            |
| oflags     | device mode flags   |
| **back** | ——                             |
| RT_EOK     | device opened successfully |
| -RT_EBUSY  | If the standalone parameter RT_DEVICE_FLAG_STANDALONE is included in the functions specified when the device is registered, the device will not be allowed to be opened repeatedly |
| Other error codes | device failed to open |

oflags parameters support the following values (Use OR logic to support multiple values):

```c
#define RT_DEVICE_FLAG_STREAM       0x040     /* Stream mode      */
/* Receive mode function */
#define RT_DEVICE_FLAG_INT_RX       0x100     /* Interrupt receive mode */
#define RT_DEVICE_FLAG_DMA_RX       0x200     /* DMA receiving mode */
/* Receive mode function */
#define RT_DEVICE_FLAG_INT_TX       0x400     /* Interrupt receive mode*/
#define RT_DEVICE_FLAG_DMA_TX       0x800     /* DMA receive mode    */
```

There are three modes of uart data receiving and sending: interrupt mode, polling mode and DMA mode. When used, only one of the three modes can be selected. If the open parameter oflag of the serial port does not specify the use of interrupt mode or DMA mode, the polling mode is used by default.

The DMA (Direct Memory Access) transfer mode does not require the CPU to directly control the transfer, and does not have the process of reserving the scene and restoring the scene as they have in the interrupt processing mode. The DMA controller opens a path for directly transferring data to the RAM and the I/O device, which saves CPU resources to do other things. Using DMA transfer can continuously acquire or send a piece of information without taking up interrupts or delays, which is useful when communication is frequent or when large pieces of information are to be transmitted.

> RT_DEVICE_FLAG_STREAM: Stream mode is used to output a string to the serial terminal: when the output character is `"\n"` (corresponding to the hexadecimal value 0x0A), a `"\r"` is automatically output in front (corresponding to hexadecimal value is 0x0D).

The stream mode `RT_DEVICE_FLAG_STREAM` can be used with the receive and send mode parameter with the "|" logic.

An example of using  a uart device in **interrupt receive mode and polling mode** as follows:

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device name */
static rt_device_t serial;              /* uart device handle */
/* find uart device */
serial = rt_device_find(SAMPLE_UART_NAME);

/* Open the uart device in interrupt receive mode and polling mode*/
rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
```

If the uart is to use the DMA receive mode, the oflags takes the value RT_DEVICE_FLAG_DMA_RX.  An example of using a uart device in the **DMA receive and polling send mode** is as follows:

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device's name */
static rt_device_t serial;              /* uart device handle */
/* find uart device */
serial = rt_device_find(SAMPLE_UART_NAME);

/* Open the uart device in DMA receive and polling send mode*/
rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
```

## Control UART Device

Through command  control word, the application can configure the uart device by the following function:

```c
rt_err_t rt_device_control(rt_device_t dev, rt_uint8_t cmd, void* arg);
```

| **Parameter**     | **Description**                                              |
| ----------------- | ------------------------------------------------------------ |
| dev               | device handle                                                |
| cmd               | command  control word can be valued as：RT_DEVICE_CTRL_CONFIG |
| arg               | controlled parameter： struct serial_configure               |
| **Back**          | ——                                                           |
| RT_EOK            | function executed successfully                               |
| -RT_ENOSYS        | execution failed, dev is empty                               |
| Other error codes | execution failed                                             |

* The prototype of control parameter structure: struct serial_configure is as follows：

```c
struct serial_configure
{
    rt_uint32_t baud_rate;          /* Baudrate */
    rt_uint32_t data_bits    :4;    /* Data bit */
    rt_uint32_t stop_bits    :2;    /* Stop bit */
    rt_uint32_t parity       :2;    /* Parity bit */
    rt_uint32_t bit_order    :1;    /* Prioritized by order */
    rt_uint32_t invert       :1;    /* Mode */
    rt_uint32_t bufsz        :16;   /* Receive data buffer size */
    rt_uint32_t reserved     :4;    /* Reserved bit */
};
```

* The default macro configuration provided by RT-Thread is as follows：

```c
#define RT_SERIAL_CONFIG_DEFAULT           \
{                                          \
    BAUD_RATE_115200, /* 115200 bps */     \
    DATA_BITS_8,      /* 8 databits */     \
    STOP_BITS_1,      /* 1 stopbit */      \
    PARITY_NONE,      /* No parity  */     \
    BIT_ORDER_LSB,    /* LSB first sent */ \
    NRZ_NORMAL,       /* Normal mode */    \
    RT_SERIAL_RB_BUFSZ, /* Buffer size */  \
    0                                      \
}
```

The configuration parameters provided by RT-Thread can be defined as the following macro definitions:：

```c
/* The baudrate can be defined as*/
#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_3000000               3000000
/* Data bits can be defined as*/
#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9
/* Stop bits can be defined as */
#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3
/* Parity bits can be defined as */
#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2
/* Bit order can be defined as */
#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1
/* Mode canbe defined as */
#define NRZ_NORMAL                      0   /* normal mode */
#define NRZ_INVERTED                    1   /* inverted mode */
/* Default size of the receive data buffer */
#define RT_SERIAL_RB_BUFSZ              64
```

**Receive Buffer**

When the uart device is opened using interrupt receive mode, the uart driver framework will open a buffer according to the size of RT_SERIAL_RB_BUFSZ to save the received data. When the underlying driver receives a data, it will put the data into the buffer in the interrupt service program.

>The default size of the receive data buffer is 64 bytes.  If the number of received data in one-time is too large and the data is not read in time, the data of the buffer will be overwritten by the newly received data, resulting in data loss. It is recommended to increase the buffer.

A sample for configuring uart hardware parameters such as data bits, check bits, stop bits, and so on are shown below：

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device's name */
static rt_device_t serial;              /* uart device handle */
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* Configuration parameters */
/* Find uart devices */
serial = rt_device_find(SAMPLE_UART_NAME);

/* Open the uart device in interrupt receive and polling send mode */
rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);

config.baud_rate = BAUD_RATE_115200;
config.data_bits = DATA_BITS_8;
config.stop_bits = STOP_BITS_2;
config.parity = PARITY_NONE;
/* The serial port configuration parameters can only be modified after opening the device */
rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
```

## Send Data

To write data to the serial port, the following functions can be used:

```c
rt_size_t rt_device_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size);
```

| **Parameter** | **Description**                         |
| ---------- | ------------------------------------------ |
| dev                | device handle                 |
| pos                | Write data offset, this parameter is not used in uart device |
| buffer             | Memory buffer pointer, place the data to be written |
| size               | The size of the written data |
| **back**       | ——                                     |
| The actual size of the written data | If it is a character device, the return size is in bytes; |
| 0                  | It needs to read the current thread's errno to determine the error status |

Calling this function will write the data in the `buffer` to the `dev` device, the size of the write data is: size.

The sample program for writing data to the serial port is as follows:

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device's name */
static rt_device_t serial;              /* uart device handle */
char str[] = "hello RT-Thread!\r\n";
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* Configuration parameter */
/*find uart device */
serial = rt_device_find(SAMPLE_UART_NAME);

/* Open the uart device in interrupt reception and polling mode */
rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
/* Send string */
rt_device_write(serial, 0, str, (sizeof(str) - 1));
```

## Set The Send completion Callback Function

When the application calls `rt_device_write()` to write data, if the underlying hardware can support automatic transmission, the upper application can set a callback function. This callback function is called after the underlying hardware data has been sent (for example, when the DMA transfer is complete or the FIFO has been written to complete the completion interrupt). You can set the device to send completion instructions by the following function:

```c
rt_err_t rt_device_set_tx_complete(rt_device_t dev, rt_err_t (*tx_done)(rt_device_t dev,void *buffer));
```

| **Parameter** | **Description**           |
| ------------- | ------------------------- |
| dev           | device handle             |
| tx_done       | callback function pointer |
| **back**      | ——                        |
| RT_EOK        | set up successfully       |

When this function is called, the callback function is provided by the user. When the hardware device sends the data, the device driver calls back this function and passes the sent data block address buffer as a parameter to the upper application. When the application (thread) receives the indication, it will release the buffer memory block or use it as the buffer for the next write data according to the condition of sending the buffer.

## Set The Receive Callback Function

The data receiving instruction can be set by the following function. When the serial port receives the data, it will inform the upper application thread that the data has arrived:

```c
rt_err_t rt_device_set_rx_indicate(rt_device_t dev, rt_err_t (*rx_ind)(rt_device_t dev,rt_size_t size));
```

| **Parameter** | **Description** |
| -------- | ------------ |
| dev      | device handle |
| rx_ind   | callback function pointer |
| dev      | device handle (callback function parameter) |
| size     | buffer data size (callback function parameter) |
| **back** | ——           |
| RT_EOK   | set up successfully |

The callback function for this function is provided by the user. If the uart device is opened in interrupt receive mode, the callback function will be called when the serial port receives a data, and the data size of the buffer will be placed in the `size` parameter, and the uart device handle will be placed in the `dev` parameter.

If the uart is opened in DMA receive mode, the callback function is called when the DMA completes receiving a batch of data.

Normally the receiving callback function can send a semaphore or event to notify the serial port data processing thread that data has arrived. The example is as follows:

```c
#define SAMPLE_UART_NAME       "uart2"  /* uart device name */
static rt_device_t serial;              /* uart device handle */
static struct rt_semaphore rx_sem;      /* The semaphore used to receive the message */

/* Receive data callback function */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* When the serial port receives the data, it triggers interrupts, calls this callback function, and sends the received semaphore */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static int uart_sample(int argc, char *argv[])
{
    serial = rt_device_find(SAMPLE_UART_NAME);

    /* Open the uart device in interrupting receive mode */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);

    /* Initialization semaphore */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    /* Set the receive callback function */
    rt_device_set_rx_indicate(serial, uart_input);
}

```

## Receive Data

You can call the following function to read the data received by the uart:

```c
rt_size_t rt_device_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size);
```

| **Parameter**                    | **Description**                                              |
| -------------------------------- | ------------------------------------------------------------ |
| dev                              | device handle                                                |
| pos                              | Read data offset, uart device dose not use this parameter    |
| buffer                           | Buffer pointer, the data read will be saved in the buffer    |
| size                             | Read the size of the data                                    |
| **back**                         | ——                                                           |
| Read the actual size of the data | If it is a character device, the return size is in bytes.    |
| 0                                | It needs to read the current thread's errno to determine the error status |

Read data offset: pos is not valid for character devices. This parameter is mainly used for block devices.

An example of using the interrupt receive mode with the receive callback function is as follows:

```c
static rt_device_t serial;            /* uart device handle */
static struct rt_semaphore rx_sem;    /* Semaphore used to receive messages */

/* Thread receiving data */
static void serial_thread_entry(void *parameter)
{
    char ch;

    while (1)
    {
        /* Reads a byte of data from the serial port and waits for the receiving semaphore if it is not read */
        while (rt_device_read(serial, -1, &ch, 1) != 1)
        {
            /* Blocking waiting to receive semaphore, waiting for the semaphore to read the data again*/
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
        /* Read the data through the serial port dislocation output*/
        ch = ch + 1;
        rt_device_write(serial, 0, &ch, 1);
    }
}
```

## Close The UART Device

After the application completes the serial port operation, the uart device can be closed by the following functions:

```c
rt_err_t rt_device_close(rt_device_t dev);
```

| **Parameter**     | **Description**                                              |
| ----------------- | ------------------------------------------------------------ |
| dev               | device handle                                                |
| **back**          | ——                                                           |
| RT_EOK            | device closed successfully                                   |
| -RT_ERROR         | The device has been completely shut down and cannot be shut down repeatedly |
| other error codes | fail to close the device                                     |

Use the `rt_device_close()` interface and `rt_device_open()` interface in pair. When you open the device, you need to close the device once, so that the device will be completely shut down, otherwise the device will remain open.

# Examples Of Using UART Device

## Interrupt Receiving And Polling Send

The main steps of the sample code are as follows:

1. First find the uart device to get the device handle.
2. Initialize the semaphore that the callback function sends, and then open the uart device in read/write and interrupt receive mode.
3. Set the receive callback function of the uart device, then send the string and create a read data thread.
4. The read data thread will try to read a character data. If there is no data, it will hang and wait for the semaphore. When the uart device receives a data, it will trigger an interrupt and call the receive callback function. This function will send a semaphore to wake up the thread. At this point, the thread will immediately read the received data.
5. This sample code is not limited to a specific BSP. According to the uart device registered by BSP, modify the uart device's name corresponding to the sample code's macro definition  SAMPLE_UART_NAME  to run.

The running sequence diagram is shown as follows:

![Serial Port Interrupt Reception and Polling Transmission Sequence Diagram](figures/uart-int.png)


```c
/*
 * Program list: This is a uart device usage routine
 * The routine exports the uart_sample command to the control terminal
 * Format of command: uart_sample uart2
 * Command explanation: the second parameter of the command is the name of the uart device. If it is null, the default uart device wil be used
 * Program function: output the string "hello RT-Thread!" through the serial port, and then malposition the input character
*/

#include <rtthread.h>

#define SAMPLE_UART_NAME       "uart2"

/* Semaphore used to receive messages */
static struct rt_semaphore rx_sem;
static rt_device_t serial;

/* Receive data callback function */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* After the uart device receives the data, it generates an interrupt, calls this callback function, and then sends the received semaphore. */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void serial_thread_entry(void *parameter)
{
    char ch;

    while (1)
    {
        /* Read a byte of data from the serial port and wait for the receiving semaphore if it is not read */
        while (rt_device_read(serial, -1, &ch, 1) != 1)
        {
            /* Being Suspended and waiting for the semaphore */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
        /* Read the data from the serial port and output through dislocation */
        ch = ch + 1;
        rt_device_write(serial, 0, &ch, 1);
    }
}

static int uart_sample(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    char str[] = "hello RT-Thread!\r\n";

    if (argc == 2)
    {
        rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
    }

    /* Find uart devices in the system */
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return -RT_ERROR;
    }

    /* Initialize the semaphore */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* Open the uart device in interrupt receive and polling send mode */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* Set the receive callback function */
    rt_device_set_rx_indicate(serial, uart_input);
    /* Send string */
    rt_device_write(serial, 0, str, (sizeof(str) - 1));

    /* Create a
/* ... truncated ... */
```
