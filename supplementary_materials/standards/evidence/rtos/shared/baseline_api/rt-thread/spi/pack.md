# Raw RTOS/Bus Pack: rt-thread / spi

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rt-thread/components/drivers/include/drivers/dev_spi.h`

```c
/*
 * Copyright (c) 2006-2025 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-11-23     Bernard      Add extern "C"
 * 2020-06-13     armink       fix the 3 wires issue
 * 2022-09-01     liYony       fix api rt_spi_sendrecv16 about MSB and LSB bug
 * 2025-10-30     wdfk-prog    enable interrupt-safe operations using spinlocks
 */

#ifndef __DEV_SPI_H__
#define __DEV_SPI_H__

#include <stdlib.h>
#include <rtthread.h>
#include <drivers/dev_pin.h>
#include <drivers/core/driver.h>

/**
 * @defgroup    group_drivers_spi SPI
 * @brief       SPI driver api
 * @ingroup     group_device_driver
 *
 * <b>Example</b>
 * @code {.c}
 * #include <rtthread.h>
 * #include <rtdevice.h>
 *
 * #define W25Q_SPI_DEVICE_NAME     "qspi10"
 *
 * static void spi_w25q_sample(int argc, char *argv[])
 * {
 *     struct rt_spi_device *spi_dev_w25q;
 *     char name[RT_NAME_MAX];
 *     rt_uint8_t w25x_read_id = 0x90;
 *     rt_uint8_t id[5] = {0};
 *
 *     if (argc == 2)
 *     {
 *         rt_strncpy(name, argv[1], RT_NAME_MAX);
 *     }
 *     else
 *     {
 *         rt_strncpy(name, W25Q_SPI_DEVICE_NAME, RT_NAME_MAX);
 *     }
 *
 *     // Find the SPI device and obtain its handle
 *     spi_dev_w25q = (struct rt_spi_device *)rt_device_find(name);
 *     if (!spi_dev_w25q)
 *     {
 *         rt_kprintf("spi sample run failed! can't find %s device!\n", name);
 *     }
 *     else
 *     {
 *         // Method 1: Send the command and read the ID using rt_spi_send_then_recv()
 *         rt_spi_send_then_recv(spi_dev_w25q, &w25x_read_id, 1, id, 5);
 *         rt_kprintf("use rt_spi_send_then_recv() read w25q ID is:%x%x\n", id[3], id[4]);
 *
 *         // Method 2: Send the command and read the ID using rt_spi_transfer_message()
 *         struct rt_spi_message msg1, msg2;
 *
 *         msg1.send_buf   = &w25x_read_id;
 *         msg1.recv_buf   = RT_NULL;
 *         msg1.length     = 1;
 *         msg1.cs_take    = 1;
 *         msg1.cs_release = 0;
 *         msg1.next       = &msg2;
 *
 *         msg2.send_buf   = RT_NULL;
 *         msg2.recv_buf   = id;
 *         msg2.length     = 5;
 *         msg2.cs_take    = 0;
 *         msg2.cs_release = 1;
 *         msg2.next       = RT_NULL;
 *
 *         rt_spi_transfer_message(spi_dev_w25q, &msg1);
 *         rt_kprintf("use rt_spi_transfer_message() read w25q ID is:%x%x\n", id[3], id[4]);
 *
 *     }
 * }
 * // Export to the msh command list
 * MSH_CMD_EXPORT(spi_w25q_sample, spi w25q sample);
 * @endcode
 */

/*!
 * @addtogroup group_drivers_spi
 * @{
 */
#ifdef __cplusplus
extern "C"{
#endif

/**
 * At CPOL=0 the base value of the clock is zero
 *  - For CPHA=0, data are captured on the clock's rising edge (low->high transition)
 *    and data are propagated on a falling edge (high->low clock transition).
 *  - For CPHA=1, data are captured on the clock's falling edge and data are
 *    propagated on a rising edge.
 * At CPOL=1 the base value of the clock is one (inversion of CPOL=0)
 *  - For CPHA=0, data are captured on clock's falling edge and data are propagated
 *    on a rising edge.
 *  - For CPHA=1, data are captured on clock's rising edge and data are propagated
 *    on a falling edge.
 */
#define RT_SPI_CPHA     (1<<0)                             /*!< bit[0]:CPHA, clock phase */
#define RT_SPI_CPOL     (1<<1)                             /*!< bit[1]:CPOL, clock polarity */

#define RT_SPI_LSB      (0<<2)                             /*!< bit[2]: 0-LSB */
#define RT_SPI_MSB      (1<<2)                             /*!< bit[2]: 1-MSB */

#define RT_SPI_MASTER   (0<<3)                             /*!< SPI master device */
#define RT_SPI_SLAVE    (1<<3)                             /*!< SPI slave device */

#define RT_SPI_CS_HIGH  (1<<4)                             /*!< Chipselect active high */
#define RT_SPI_NO_CS    (1<<5)                             /*!< No chipselect */
#define RT_SPI_3WIRE    (1<<6)                             /*!< SI/SO pin shared */
#define RT_SPI_READY    (1<<7)                             /*!< Slave pulls low to pause */

#define RT_SPI_MODE_MASK    (RT_SPI_CPHA | RT_SPI_CPOL | RT_SPI_MSB | RT_SPI_SLAVE | RT_SPI_CS_HIGH | RT_SPI_NO_CS | RT_SPI_3WIRE | RT_SPI_READY)

#define RT_SPI_MODE_0       (0 | 0)                        /*!< CPOL = 0, CPHA = 0 */
#define RT_SPI_MODE_1       (0 | RT_SPI_CPHA)              /*!< CPOL = 0, CPHA = 1 */
#define RT_SPI_MODE_2       (RT_SPI_CPOL | 0)              /*!< CPOL = 1, CPHA = 0 */
#define RT_SPI_MODE_3       (RT_SPI_CPOL | RT_SPI_CPHA)    /*!< CPOL = 1, CPHA = 1 */

#define RT_SPI_BUS_MODE_SPI         (1<<0)
#define RT_SPI_BUS_MODE_QSPI        (1<<1)

#define RT_SPI_CS_CNT_MAX           16

/**
 * @brief SPI message structure
 */
struct rt_spi_message
{
    const void *send_buf;
    void *recv_buf;
    rt_size_t length;
    struct rt_spi_message *next;

    unsigned cs_take    : 1;
    unsigned cs_release : 1;
};

/**
 * @brief SPI configuration structure
 */
struct rt_spi_configuration
{
    rt_uint8_t mode;
    rt_uint8_t data_width;
#ifdef RT_USING_DM
    rt_uint8_t data_width_tx;
    rt_uint8_t data_width_rx;
#else
    rt_uint16_t reserved;
#endif

    rt_uint32_t max_hz;
    rt_uint32_t usage_freq;
};

struct rt_spi_ops;

/**
 * @brief SPI bus structure
 */
struct rt_spi_bus
{
    struct rt_device parent;
    rt_uint8_t mode;
    const struct rt_spi_ops *ops;

#ifdef RT_USING_DM
    rt_base_t cs_pins[RT_SPI_CS_CNT_MAX];
    rt_uint8_t cs_active_vals[RT_SPI_CS_CNT_MAX];
    rt_bool_t slave;
    int num_chipselect;
#endif /* RT_USING_DM */

    struct rt_mutex lock;
#ifdef RT_USING_SPI_ISR
    rt_base_t _isr_lvl;
    struct rt_spinlock _spinlock;
#endif /* RT_USING_SPI_ISR */
    struct rt_spi_device *owner;
};

/**
 * @brief SPI operators
 */
struct rt_spi_ops
{
    rt_err_t (*configure)(struct rt_spi_device *device, struct rt_spi_configuration *configuration);
    rt_ssize_t (*xfer)(struct rt_spi_device *device, struct rt_spi_message *message);
};

#ifdef RT_USING_DM
/**
 * @brief SPI delay info
 */
struct rt_spi_delay
{
#define RT_SPI_DELAY_UNIT_USECS 0
#define RT_SPI_DELAY_UNIT_NSECS 1
#define RT_SPI_DELAY_UNIT_SCK   2
    rt_uint16_t value;
    rt_uint8_t  unit;
};
#endif /* RT_USING_DM */

/**
 * @brief SPI Virtual BUS, one device must connected to a virtual BUS
 */
struct rt_spi_device
{
    struct rt_device parent;
    struct rt_spi_bus *bus;

#ifdef RT_USING_DM
    const char *name;
    const struct rt_spi_device_id *id;
    const struct rt_ofw_node_id *ofw_id;

    rt_uint8_t chip_select[RT_SPI_CS_CNT_MAX];
    struct rt_spi_delay cs_setup;
    struct rt_spi_delay cs_hold;
    struct rt_spi_delay cs_inactive;
#endif

    struct rt_spi_configuration config;
    rt_base_t cs_pin;
    void   *user_data;
};

/**
 * @brief QSPI message structure
 */
struct rt_qspi_message
{
    struct rt_spi_message parent;

    /* instruction stage */
    struct
    {
        rt_uint8_t content;
        rt_uint8_t qspi_lines;
    } instruction;

    /* address and alternate_bytes stage */
    struct
    {
        rt_uint32_t content;
        rt_uint8_t size;
        rt_uint8_t qspi_lines;
    } address, alternate_bytes;

    /* dummy_cycles stage */
    rt_uint32_t dummy_cycles;

    /* number of lines in qspi data stage, the other configuration items are in parent */
    rt_uint8_t qspi_data_lines;
};

/**
 * @brief QSPI configuration structure
 */
struct rt_qspi_configuration
{
    struct rt_spi_configuration parent;
    /* The size of medium */
    rt_uint32_t medium_size;
    /* double data rate mode */
    rt_uint8_t ddr_mode;
    /* the data lines max width which QSPI bus supported, such as 1, 2, 4 */
    rt_uint8_t qspi_dl_width ;
};

/**
 * @brief QSPI operators
 */
struct rt_qspi_device
{
    struct rt_spi_device parent;

    struct rt_qspi_configuration config;

    void (*enter_qspi_mode)(struct rt_qspi_device *device);

    void (*exit_qspi_mode)(struct rt_qspi_device *device);
};

#define SPI_DEVICE(dev) ((struct rt_spi_device *)(dev))

#ifdef RT_USING_DM
struct rt_spi_device_id
{
    char name[20];
    void *data;
};

struct rt_spi_driver
{
    struct rt_driver parent;

    const struct rt_spi_device_id *ids;
    const struct rt_ofw_node_id *ofw_ids;

    rt_err_t (*probe)(struct rt_spi_device *device);
    rt_err_t (*remove)(struct rt_spi_device *device);
    rt_err_t (*shutdown)(struct rt_spi_device *device);
};

rt_err_t rt_spi_driver_register(struct rt_spi_driver *driver);
rt_err_t rt_spi_device_register(struct rt_spi_device *device);

#define RT_SPI_DRIVER_EXPORT(driver)  RT_DRIVER_EXPORT(driver, spi, BUILIN)

rt_inline const void *rt_spi_device_id_data(struct rt_spi_device *device)
{
    return device->id ? device->id->data : (device->ofw_id ? device->ofw_id->data : RT_NULL);
}
#endif /* RT_USING_DM */

/**
 * @brief register a SPI bus
 *
 * @param bus the SPI bus
 * @param name the name of SPI bus
 * @param ops the operations of SPI bus
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_bus_register(struct rt_spi_bus       *bus,
                             const char              *name,
                             const struct rt_spi_ops *ops);


/**
 * @brief attach a device on SPI bus
 *
 * @param device the SPI device
 * @param name the name of SPI device
 * @param bus_name the name of SPI bus
 * @param user_data the user data of SPI device
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_bus_attach_device(struct rt_spi_device *device,
                                  const char           *name,
                                  const char           *bus_name,
                                  void                 *user_data);

/**
 * @brief Detach a device from the SPI bus.
 *
 * This function serves as the high-level API to detach a SPI device from its bus.
 * It unregisters the device from the device framework and ensures all associated
 * resources, such as the chip select pin, are properly released by calling
 * the underlying implementation.
 *
 * @param device The SPI device to be detached.
 *
 * @return rt_err_t The result of the operation. RT_EOK on success, otherwise an error code.
 */
rt_err_t rt_spi_bus_detach_device(struct rt_spi_device *device);

/**
 * @brief attach a device on SPI bus with CS pin
 *
 * @param device the SPI device
 * @param name the name of SPI device
 * @param bus_name the name of SPI bus
 * @param cs_pin the CS pin of SPI device
 * @param user_data the user data of SPI device
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_bus_attach_device_cspin(struct rt_spi_device *device,
                                        const char           *name,
                                        const char           *bus_name,
                                        rt_base_t             cs_pin,
                                        void                 *user_data);

/**
 * @brief Detach a device from the SPI bus and release its CS pin.
 *
 * This function provides the low-level implementation for detaching a device
 * from the SPI bus. It specifically handles the operations for the chip select (CS)
 * pin, resetting it to input mode to release it. This function is typically
 * called by the higher-level rt_spi_bus_detach_device() and should not be
 * called directly by the user application.
 *
 * @param device The SPI device to be detached.
 *
 * @return rt_err_t The result of the operation. RT_EOK on success, otherwise an error code.
 */
rt_err_t rt_spi_bus_detach_device_cspin(struct rt_spi_device *device);

/**
 * @brief  Reconfigure the SPI bus for the specified device.
 *
 * @param  device: Pointer to the SPI device attached to the SPI bus.
 * @retval RT_EOK if the SPI device was successfully released and the bus was configured.
 *         RT_EBUSY if the SPI bus is currently in use; the new configuration will take effect once the device releases the bus.
 *         Other return values indicate failure to configure the SPI bus due to various reasons.
 * @note   If the configuration of the SPI device has been updated and requires bus re-initialization,
 *         call this function directly. This function will reconfigure the SPI bus for the specified device.
 *         If this is the first time to initialize the SPI device, please call rt_spi_configure or rt_qspi_configure.
 *         This function is used to reconfigure the SPI bus when the SPI device is already in use.
 *         For further details, refer to:
 *         https://github.com/RT-Thread/rt-thread/pull/8528
 */
rt_err_t rt_spi_bus_configure(struct rt_spi_device *device);

/**
 * @brief This function takes SPI bus.
 *
 * @param device the SPI device attached to SPI bus
 *
 * @return RT_EOK on taken SPI bus successfully. others on taken SPI bus failed.
 */
rt_err_t rt_spi_take_bus(struct rt_spi_device *device);

/**
 * @brief This function releases SPI bus.
 *
 * @param device the SPI device attached to SPI bus
 *
 * @return RT_EOK on release SPI bus successfully.
 */
rt_err_t rt_spi_release_bus(struct rt_spi_device *device);

/**
 * @brief This function take SPI device (takes CS of SPI device).
 *
 * @param device the SPI device attached to SPI bus
 *
 * @return RT_EOK on release SPI bus successfully. others on taken SPI bus failed.
 */
rt_err_t rt_spi_take(struct rt_spi_device *device);

/**
 * @brief This function releases SPI device (releases CS of SPI device).
 *
 * @param device the SPI device attached to SPI bus
 *
 * @return RT_EOK on release SPI device successfully.
 */
rt_err_t rt_spi_release(struct rt_spi_device *device);

/**
 * @brief  This function can set configuration on SPI device.
 *
 * @param  device: the SPI device attached to SPI bus
 * @param  cfg: the configuration pointer.
 *
 * @retval RT_EOK on release SPI device successfully.
 *         RT_EBUSY is not an error condition and the configuration will take effect once the device has the bus
 *         others on taken SPI bus failed.
 */
rt_err_t rt_spi_configure(struct rt_spi_device        *device,
                          struct rt_spi_configuration *cfg);


/**
 * @brief This function can send data then receive data from SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param send_buf the buffer to be transmitted to SPI device.
 * @param send_length the number of data to be transmitted.
 * @param recv_buf the buffer to be recivied from SPI device.
 * @param recv_length the data to be recivied.
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_send_then_recv(struct rt_spi_device *device,
                               const void           *send_buf,
                               rt_size_t             send_length,
                               void                 *recv_buf,
                               rt_size_t             recv_length);

/**
 * @brief This function can send data then send data from SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param send_buf1 the buffer to be transmitted to SPI device.
 * @param send_length1 the number of data to be transmitted.
 * @param send_buf2 the buffer to be transmitted to SPI device.
 * @param send_length2 the number of data to be transmitted.
 *
 * @return the status of transmit.
 */
rt_err_t rt_spi_send_then_send(struct rt_spi_device *device,
                               const void           *send_buf1,
                               rt_size_t             send_length1,
                               const void           *send_buf2,
                               rt_size_t             send_length2);

/**
 * @brief This function transmits data to SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param send_buf the buffer to be transmitted to SPI device.
 * @param recv_buf the buffer to save received data from SPI device.
 * @param length the length of transmitted data.
 *
 * @return the actual length of transmitted.
 */
rt_ssize_t rt_spi_transfer(struct rt_spi_device *device,
                           const void           *send_buf,
                           void                 *recv_buf,
                           rt_size_t             length);

/**
 * @brief The SPI device transmits 8 bytes of data
 *
 * @param device the SPI device attached to SPI bus
 * @param senddata send data buffer
 * @param recvdata receive data buffer
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_sendrecv8(struct rt_spi_device *device,
                          rt_uint8_t            senddata,
                          rt_uint8_t           *recvdata);

/**
 * @brief The SPI device transmits 16 bytes of data
 *
 * @param device the SPI device attached to SPI bus
 * @param senddata send data buffer
 * @param recvdata receive data buffer
 *
 * @return rt_err_t error code
 */
rt_err_t rt_spi_sendrecv16(struct rt_spi_device *device,
                           rt_uint16_t           senddata,
                           rt_uint16_t          *recvdata);

/**
 * @brief This function transfers a message list to the SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param message the message list to be transmitted to SPI device
 *
 * @return RT_NULL if transmits message list successfully,
 *         SPI message which be transmitted failed.
 */
struct rt_spi_message *rt_spi_transfer_message(struct rt_spi_device  *device,
                                               struct rt_spi_message *message);

/**
 * @brief This function receives data from SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param recv_buf the buffer to be recivied from SPI device.
 * @param length the data to be recivied.
 *
 * @return the actual length of received.
*/
rt_inline rt_size_t rt_spi_recv(struct rt_spi_device *device,
                                void                 *recv_buf,
                                rt_size_t             length)
{
    return rt_spi_transfer(device, RT_NULL, recv_buf, length);
}

/**
 * @brief This function sends data to SPI device.
 *
 * @param device the SPI device attached to SPI bus
 * @param send_buf the buffer to be transmitted to SPI device.
 * @param length the number of data to be transmitted.
 *
 * @return the actual length of send.
 */
rt_inline rt_size_t rt_spi_send(struct rt_spi_device *device,
                                const void           *send_buf,
                                rt_size_t             length)
{
    return rt_spi_transfer(device, send_buf, RT_NULL, length);
}

/**
 * @brief This function appends a message to the SPI message list.
 *
 * @param list the SPI message list header.
 * @param message the message pointer to be appended to the message list.
 */
rt_inline void rt_spi_message_append(struct rt_spi_message *list,
                                     struct rt_spi_message *message)
{
    RT_ASSERT(list != RT_NULL);
    if (message == RT_NULL)
        return; /* not append */

    while (list->next != RT_NULL)
    {
        list = list->next;
    }

    list->next = message;
    message->next = RT_NULL;
}

/**
 * @brief This function can set configuration on QSPI device.
 *
 * @param device the QSPI device attached to QSPI bus.
 * @param cfg the configuration pointer.
 *
 * @return the actual length of transmitted.
 */
rt_err_t rt_qspi_configure(struct rt_qspi_device *device, struct rt_qspi_configuration *cfg);

/**
 * @brief This function can register a SPI bus for QSPI mode.
 *
 * @param bus the SPI bus for QSPI mode.
 * @param name The name of the spi bus.
 * @param ops the SPI bus instance to be registered.
 *
 * @return the actual length of transmitted.
 */
rt_err_t rt_qspi_bus_register(struct rt_spi_bus *bus, const char *name, const struct rt_spi_ops *ops);

/**
 * @brief This function transmits data to QSPI device.
 *
 * @param device the QSPI device attached to QSPI bus.
 * @param message the message pointer.
 *
 * @return the actual length of transmitted.
 */
rt_ssize_t rt_qspi_transfer_message(struct rt_qspi_device  *device, struct rt_qspi_message *message);

/**
 * @brief This function can send data then receive data from QSPI device
 *
 * @param device the QSPI device attached to QSPI bus.
 * @param send_buf the buffer to be transmitted to QSPI device.
 * @param send_length the number of data to be transmitted.
 * @param recv_buf the buffer to be recivied from QSPI device.
 * @param recv_length the data to be recivied.
 *
 * @return the status of transmit.
 */
rt_ssize_t rt_qspi_send_then_recv(struct rt_qspi_device *device, const void *send_buf, rt_size_t send_length,void *recv_buf, rt_size_t recv_length);

/**
 * @brief This function can send data to QSPI device
 *
 * @param device the QSPI device attached to QSPI bus.
 * @param send_buf the buffer to be transmitted to QSPI device.
 * @param length the number of data to be transmitted.
 *
 * @return the status of transmit.
 */
rt_ssize_t rt_qspi_send(struct rt_qspi_device *device, const void *send_buf, rt_size_t length);

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

## Source: `data/rtos/rt-thread/components/drivers/spi/dev_spi_core.c`

```c
/*
 * Copyright (c) 2006-2025 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-08     bernard      first version.
 * 2012-02-03     bernard      add const attribute to the ops.
 * 2012-05-15     dzzxzz       fixed the return value in attach_device.
 * 2012-05-18     bernard      Changed SPI message to message list.
 *                             Added take/release SPI device/bus interface.
 * 2012-09-28     aozima       fixed rt_spi_release_bus assert error.
 * 2025-10-30     wdfk-prog    enable interrupt-safe operations using spinlocks
 */

#include "drivers/dev_spi.h"

#define DBG_TAG    "spi.core"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#ifdef RT_USING_DM
#include "dev_spi_dm.h"
#endif

extern rt_err_t rt_spi_bus_device_init(struct rt_spi_bus *bus, const char *name);
extern rt_err_t rt_spidev_device_init(struct rt_spi_device *dev, const char *name);

rt_err_t spi_bus_register(struct rt_spi_bus       *bus,
                          const char              *name,
                          const struct rt_spi_ops *ops)
{
    rt_err_t result;

    result = rt_spi_bus_device_init(bus, name);
    if (result != RT_EOK)
        return result;

    /* initialize mutex lock */
    rt_mutex_init(&(bus->lock), name, RT_IPC_FLAG_PRIO);
#ifdef RT_USING_SPI_ISR
    rt_spin_lock_init(&bus->_spinlock);
#endif /* RT_USING_SPI_ISR */
    /* set ops */
    bus->ops = ops;
    /* initialize owner */
    bus->owner = RT_NULL;

#ifdef RT_USING_DM
    if (!bus->slave)
    {
        int pin_count = rt_pin_get_named_pin_count(&bus->parent, "cs");

        if (pin_count > 0)
        {
            pin_count = rt_max_t(int, pin_count, bus->num_chipselect);

            for (int i = 0; i < pin_count; ++i)
            {
                bus->cs_pins[i] = rt_pin_get_named_pin(&bus->parent, "cs", i,
                                                       RT_NULL, &bus->cs_active_vals[i]);
            }
        }
        else if (pin_count < 0)
        {
            result = pin_count;

            LOG_E("CS PIN find error = %s", rt_strerror(result));

            rt_device_unregister(&bus->parent);
            return result;
        }
    }

    spi_bus_scan_devices(bus);
#endif

    return RT_EOK;
}

rt_err_t rt_spi_bus_register(struct rt_spi_bus       *bus,
                             const char              *name,
                             const struct rt_spi_ops *ops)
{
    /* set bus mode */
    bus->mode = RT_SPI_BUS_MODE_SPI;

    return spi_bus_register(bus, name, ops);
}

rt_err_t rt_spi_bus_attach_device_cspin(struct rt_spi_device *device,
                                        const char           *name,
                                        const char           *bus_name,
                                        rt_base_t            cs_pin,
                                        void                 *user_data)
{
    rt_err_t result;
    rt_device_t bus;

    /* get physical spi bus */
    bus = rt_device_find(bus_name);
    if (bus != RT_NULL && bus->type == RT_Device_Class_SPIBUS)
    {
        device->bus = (struct rt_spi_bus *)bus;

        if (device->bus->owner == RT_NULL)
            device->bus->owner = device;

        /* initialize spidev device */
        result = rt_spidev_device_init(device, name);
        if (result != RT_EOK)
            return result;

        if (cs_pin != PIN_NONE)
        {
            rt_pin_mode(cs_pin, PIN_MODE_OUTPUT);
        }

        rt_memset(&device->config, 0, sizeof(device->config));
        device->parent.user_data = user_data;
        device->cs_pin = cs_pin;
        return RT_EOK;
    }

    /* not found the host bus */
    return -RT_ERROR;
}

rt_err_t rt_spi_bus_detach_device_cspin(struct rt_spi_device *device)
{
    rt_err_t result;

    RT_ASSERT(device != RT_NULL);

    result = rt_device_unregister(&device->parent);
    if (result != RT_EOK)
    {
        LOG_E("Failed to unregister spi device, result: %d", result);
        return result;
    }

    if (device->bus != RT_NULL && device->bus->owner == device)
    {
        device->bus->owner = RT_NULL;
    }

    if (device->cs_pin != PIN_NONE)
    {
        rt_pin_mode(device->cs_pin, PIN_MODE_INPUT);
    }

    device->bus = RT_NULL;

    return RT_EOK;
}

rt_err_t rt_spi_bus_attach_device(struct rt_spi_device *device,
                                  const char           *name,
                                  const char           *bus_name,
                                  void                 *user_data)
{
    return rt_spi_bus_attach_device_cspin(device, name, bus_name, PIN_NONE, user_data);
}

rt_err_t rt_spi_bus_detach_device(struct rt_spi_device *device)
{
    return rt_spi_bus_detach_device_cspin(device);
}

static rt_err_t spi_lock(struct rt_spi_bus *bus)
{
    RT_ASSERT(bus);

    rt_err_t ret = -RT_ERROR;
    /* If the scheduler is started and in thread context */
    if (rt_scheduler_is_available())
    {
        ret = rt_mutex_take(&(bus->lock), RT_WAITING_FOREVER);
    }
    else
    {
#ifdef RT_USING_SPI_ISR
        bus->_isr_lvl = rt_spin_lock_irqsave(&bus->_spinlock);
        ret = RT_EOK;
#endif /* RT_USING_SPI_ISR */
    }
    return ret;
}

static rt_err_t spi_unlock(struct rt_spi_bus *bus)
{
    RT_ASSERT(bus);

    rt_err_t ret = -RT_ERROR;
    /* If the scheduler is started and in thread context */
    if (rt_scheduler_is_available())
    {
        ret = rt_mutex_release(&(bus->lock));
    }
    else
    {
#ifdef RT_USING_SPI_ISR
        rt_spin_unlock_irqrestore(&bus->_spinlock, bus->_isr_lvl);
        ret = RT_EOK;
#endif /* RT_USING_SPI_ISR */
    }
    return ret;
}

rt_err_t rt_spi_bus_configure(struct rt_spi_device *device)
{
    rt_err_t result = -RT_ERROR;

    if (device->bus != RT_NULL)
    {
        result = spi_lock(device->bus);
        if (result == RT_EOK)
        {
            if (device->bus->owner == device)
            {
                /* current device is using, re-configure SPI bus */
                result = device->bus->ops->configure(device, &device->config);
                if (result != RT_EOK)
                {
                    /* configure SPI bus failed */
                    LOG_E("SPI device %s configuration failed", device->parent.parent.name);
                }
            }
            else
            {
                /* RT_EBUSY is not an error condition and
                 * the configuration will take effect once the device has the bus
                 */
                result = -RT_EBUSY;
            }
            /* release lock */
            spi_unlock(device->bus);
        }
    }
    else
    {
        result = RT_EOK;
    }

    return result;
}

rt_err_t rt_spi_configure(struct rt_spi_device        *device,
                          struct rt_spi_configuration *cfg)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    /* reset the CS pin */
    if (device->cs_pin != PIN_NONE)
    {
        rt_err_t result = spi_lock(device->bus);
        if (result == RT_EOK)
        {
            if (cfg->mode & RT_SPI_CS_HIGH)
            {
                rt_pin_write(device->cs_pin, PIN_LOW);
            }
            else
            {
                rt_pin_write(device->cs_pin, PIN_HIGH);
            }
            spi_unlock(device->bus);
        }
        else
        {
            return result;
        }
    }

    /* If the configurations are the same, we don't need to set again. */
    if (device->config.data_width == cfg->data_width &&
        device->config.mode       == (cfg->mode & RT_SPI_MODE_MASK) &&
        device->config.max_hz     == cfg->max_hz)
    {
        return RT_EOK;
    }

    /* set configuration */
    device->config.data_width = cfg->data_width;
    device->config.mode       = cfg->mode & RT_SPI_MODE_MASK;
    device->config.max_hz     = cfg->max_hz;

    return rt_spi_bus_configure(device);
}

rt_err_t rt_spi_send_then_send(struct rt_spi_device *device,
                               const void           *send_buf1,
                               rt_size_t             send_length1,
                               const void           *send_buf2,
                               rt_size_t             send_length2)
{
    rt_err_t result;
    struct rt_spi_message message;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    result = spi_lock(device->bus);
    if (result == RT_EOK)
    {
        if (device->bus->owner != device)
        {
            /* not the same owner as current, re-configure SPI bus */
            result = device->bus->ops->configure(device, &device->config);
            if (result == RT_EOK)
            {
                /* set SPI bus owner */
                device->bus->owner = device;
            }
            else
            {
                /* configure SPI bus failed */
                LOG_E("SPI device %s configuration failed", device->parent.parent.name);
                goto __exit;
            }
        }

        /* send data1 */
        message.send_buf   = send_buf1;
        message.recv_buf   = RT_NULL;
        message.length     = send_length1;
        message.cs_take    = 1;
        message.cs_release = 0;
        message.next       = RT_NULL;

        result = device->bus->ops->xfer(device, &message);
        if (result < 0)
        {
            LOG_E("SPI device %s transfer failed", device->parent.parent.name);
            goto __exit;
        }

        /* send data2 */
        message.send_buf   = send_buf2;
        message.recv_buf   = RT_NULL;
        message.length     = send_length2;
        message.cs_take    = 0;
        message.cs_release = 1;
        message.next       = RT_NULL;

        result = device->bus->ops->xfer(device, &message);
        if (result < 0)
        {
            LOG_E("SPI device %s transfer failed", device->parent.parent.name);
            goto __exit;
        }

        result = RT_EOK;
    }
    else
    {
        return -RT_EIO;
    }

__exit:
    spi_unlock(device->bus);

    return result;
}

rt_err_t rt_spi_send_then_recv(struct rt_spi_device *device,
                               const void           *send_buf,
                               rt_size_t             send_length,
                               void                 *recv_buf,
                               rt_size_t             recv_length)
{
    rt_err_t result;
    struct rt_spi_message message;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    result = spi_lock(device->bus);
    if (result == RT_EOK)
    {
        if (device->bus->owner != device)
        {
            /* not the same owner as current, re-configure SPI bus */
            result = device->bus->ops->configure(device, &device->config);
            if (result == RT_EOK)
            {
                /* set SPI bus owner */
                device->bus->owner = device;
            }
            else
            {
                /* configure SPI bus failed */
                LOG_E("SPI device %s configuration failed", device->parent.parent.name);
                goto __exit;
            }
        }

        /* send data */
        message.send_buf   = send_buf;
        message.recv_buf   = RT_NULL;
        message.length     = send_length;
        message.cs_take    = 1;
        message.cs_release = 0;
        message.next       = RT_NULL;

        result = device->bus->ops->xfer(device, &message);
        if (result < 0)
        {
            LOG_E("SPI device %s transfer failed", device->parent.parent.name);
            goto __exit;
        }

        /* recv data */
        message.send_buf   = RT_NULL;
        message.recv_buf   = recv_buf;
        message.length     = recv_length;
        message.cs_take    = 0;
        message.cs_release = 1;
        message.next       = RT_NULL;

        result = device->bus->ops->xfer(device, &message);
        if (result < 0)
        {
            LOG_E("SPI device %s transfer failed", device->parent.parent.name);
            goto __exit;
        }

        result = RT_EOK;
    }
    else
    {
        return -RT_EIO;
    }

__exit:
    spi_unlock(device->bus);

    return result;
}

rt_ssize_t rt_spi_transfer(struct rt_spi_device *device,
                           const void           *send_buf,
                           void                 *recv_buf,
                           rt_size_t             length)
{
    rt_ssize_t result;
    struct rt_spi_message message;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    result = spi_lock(device->bus);
    if (result == RT_EOK)
    {
        if (device->bus->owner != device)
        {
            /* not the same owner as current, re-configure SPI bus */
            result = device->bus->ops->configure(device, &device->config);
            if (result == RT_EOK)
            {
                /* set SPI bus owner */
                device->bus->owner = device;
            }
            else
            {
                /* configure SPI bus failed */
                LOG_E("SPI device %s configuration failed", device->parent.parent.name);
                goto __exit;
            }
        }

        /* initial message */
        message.send_buf   = send_buf;
        message.recv_buf   = recv_buf;
        message.length     = length;
        message.cs_take    = 1;
        message.cs_release = 1;
        message.next       = RT_NULL;

        /* transfer message */
        result = device->bus->ops->xfer(device, &message);
        if (result < 0)
        {
            LOG_E("SPI device %s transfer failed", device->parent.parent.name);
            goto __exit;
        }
    }
    else
    {
        return -RT_EIO;
    }

__exit:
    spi_unlock(device->bus);

    return result;
}

rt_err_t rt_spi_sendrecv8(struct rt_spi_device *device,
                          rt_uint8_t            senddata,
                          rt_uint8_t           *recvdata)
{
    rt_ssize_t len = rt_spi_transfer(device, &senddata, recvdata, 1);
    if (len < 0)
    {
        return (rt_err_t)len;
    }
    else
    {
        return RT_EOK;
    }
}

rt_err_t rt_spi_sendrecv16(struct rt_spi_device *device,
                           rt_uint16_t           senddata,
                           rt_uint16_t          *recvdata)
{
    rt_ssize_t len;
    rt_uint16_t tmp;

    if (device->config.mode & RT_SPI_MSB)
    {
        tmp = ((senddata & 0xff00) >> 8) | ((senddata & 0x00ff) << 8);
        senddata = tmp;
    }

    len = rt_spi_transfer(device, &senddata, recvdata, 2);
    if (len < 0)
    {
        return (rt_err_t)len;
    }

    if (device->config.mode & RT_SPI_MSB)
    {
        tmp = ((*recvdata & 0xff00) >> 8) | ((*recvdata & 0x00ff) << 8);
        *recvdata = tmp;
    }

    return RT_EOK;
}

struct rt_spi_message *rt_spi_transfer_message(struct rt_spi_device  *device,
                                               struct rt_spi_message *message)
{
    rt_err_t result;
    struct rt_spi_message *index;

    RT_ASSERT(device != RT_NULL);

    /* get first message */
    index = message;
    if (index == RT_NULL)
        return index;

    result = spi_lock(device->bus);
    if (result != RT_EOK)
    {
        return index;
    }

    /* configure SPI bus */
    if (device->bus->owner != device)
    {
        /* not the same owner as current, re-configure SPI bus */
        result = device->bus->ops->configure(device, &device->config);
        if (result == RT_EOK)
        {
            /* set SPI bus owner */
            device->bus->owner = device;
        }
        else
        {
            /* configure SPI bus failed */
            goto __exit;
        }
    }

    /* transmit each SPI message */
    while (index != RT_NULL)
    {
        /* transmit SPI message */
        result = device->bus->ops->xfer(device, index);
        if (result < 0)
        {
            break;
        }

        index = index->next;
    }

__exit:
    /* release bus lock */
    spi_unlock(device->bus);

    return index;
}

rt_err_t rt_spi_take_bus(struct rt_spi_device *device)
{
    rt_err_t result = RT_EOK;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    result = spi_lock(device->bus);
    if (result != RT_EOK)
    {
        return -RT_EBUSY;
    }

    /* configure SPI bus */
    if (device->bus->owner != device)
    {
        /* not the same owner as current, re-configure SPI bus */
        result = device->bus->ops->configure(device, &device->config);
        if (result == RT_EOK)
        {
            /* set SPI bus owner */
            device->bus->owner = device;
        }
        else
        {
            /* configure SPI bus failed */
            spi_unlock(device->bus);

            return result;
        }
    }

    return result;
}

rt_err_t rt_spi_release_bus(struct rt_spi_device *device)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->owner == device);

    /* release lock */
    return spi_unlock(device->bus);
}

rt_err_t rt_spi_take(struct rt_spi_device *device)
{
    rt_ssize_t result;
    struct rt_spi_message message;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    rt_memset(&message, 0, sizeof(message));
    message.cs_take = 1;

    result = device->bus->ops->xfer(device, &message);
    if (result < 0)
    {
        return (rt_err_t)result;
    }

    return RT_EOK;
}

rt_err_t rt_spi_release(struct rt_spi_device *device)
{
    rt_ssize_t result;
    struct rt_spi_message message;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    rt_memset(&message, 0, sizeof(message));
    message.cs_release = 1;

    result = device->bus->ops->xfer(device, &message);
    if (result < 0)
    {
        return (rt_err_t)result;
    }

    return RT_EOK;
}
```

## Source: `data/rtos/rt-thread/components/drivers/spi/dev_spi.c`

```c
/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rtthread.h>
#include "drivers/dev_spi.h"

#define DBG_TAG     "spi.dev"
#define DBG_LVL     DBG_INFO
#include <rtdbg.h>

#ifdef RT_USING_DM
#include "dev_spi_dm.h"
#endif

/* SPI bus device interface, compatible with RT-Thread 0.3.x/1.0.x */
static rt_ssize_t _spi_bus_device_read(rt_device_t dev,
                                      rt_off_t    pos,
                                      void       *buffer,
                                      rt_size_t   size)
{
    struct rt_spi_bus *bus;

    bus = (struct rt_spi_bus *)dev;
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(bus->owner != RT_NULL);

    return rt_spi_transfer(bus->owner, RT_NULL, buffer, size);
}

static rt_ssize_t _spi_bus_device_write(rt_device_t dev,
                                       rt_off_t    pos,
                                       const void *buffer,
                                       rt_size_t   size)
{
    struct rt_spi_bus *bus;

    bus = (struct rt_spi_bus *)dev;
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(bus->owner != RT_NULL);

    return rt_spi_transfer(bus->owner, buffer, RT_NULL, size);
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops spi_bus_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _spi_bus_device_read,
    _spi_bus_device_write,
    RT_NULL
};
#endif

rt_err_t rt_spi_bus_device_init(struct rt_spi_bus *bus, const char *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    /* set device type */
    device->type    = RT_Device_Class_SPIBUS;
    /* initialize device interface */
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &spi_bus_ops;
#else
    device->init    = RT_NULL;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = _spi_bus_device_read;
    device->write   = _spi_bus_device_write;
    device->control = RT_NULL;
#endif

    /* register to device manager */
    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);
}

/* SPI Dev device interface, compatible with RT-Thread 0.3.x/1.0.x */
static rt_ssize_t _spidev_device_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   size)
{
    struct rt_spi_device *device;

    device = (struct rt_spi_device *)dev;
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    return rt_spi_transfer(device, RT_NULL, buffer, size);
}

static rt_ssize_t _spidev_device_write(rt_device_t dev,
                                      rt_off_t    pos,
                                      const void *buffer,
                                      rt_size_t   size)
{
    struct rt_spi_device *device;

    device = (struct rt_spi_device *)dev;
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);

    return rt_spi_transfer(device, buffer, RT_NULL, size);
}

static rt_err_t _spidev_device_control(rt_device_t dev,
                                       int         cmd,
                                       void       *args)
{
    switch (cmd)
    {
    case 0: /* set device */
        break;
    case 1:
        break;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops spi_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    _spidev_device_read,
    _spidev_device_write,
    _spidev_device_control
};
#endif

rt_err_t rt_spidev_device_init(struct rt_spi_device *dev, const char *name)
{
    struct rt_device *device;
    RT_ASSERT(dev != RT_NULL);

    device = &(dev->parent);

    /* set device type */
    device->type    = RT_Device_Class_SPIDevice;
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &spi_device_ops;
#else
    device->init    = RT_NULL;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = _spidev_device_read;
    device->write   = _spidev_device_write;
    device->control = _spidev_device_control;
#endif

    /* register to device manager */
    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);
}

#ifdef RT_USING_DM
static rt_err_t spidev_probe(struct rt_spi_device *spi_dev)
{
    const char *bus_name;
    struct rt_device *dev = &spi_dev->parent;

    if (spi_dev->parent.ofw_node)
    {
        if (rt_dm_dev_prop_index_of_string(dev, "compatible", "spidev") >= 0)
        {
            LOG_E("spidev is not supported in OFW");

            return -RT_EINVAL;
        }
    }

    bus_name = rt_dm_dev_get_name(&spi_dev->bus->parent);
    rt_dm_dev_set_name(dev, "%s_%d", bus_name, spi_dev->chip_select[0]);

    return RT_EOK;
}

static const struct rt_spi_device_id spidev_ids[] =
{
    { .name = "dh2228fv" },
    { .name = "ltc2488" },
    { .name = "sx1301" },
    { .name = "bk4" },
    { .name = "dhcom-board" },
    { .name = "m53cpld" },
    { .name = "spi-petra" },
    { .name = "spi-authenta" },
    { .name = "em3581" },
    { .name = "si3210" },
    { /* sentinel */ },
};

static const struct rt_ofw_node_id spidev_ofw_ids[] =
{
    { .compatible = "cisco,spi-petra" },
    { .compatible = "dh,dhcom-board" },
    { .compatible = "lineartechnology,ltc2488" },
    { .compatible = "lwn,bk4" },
    { .compatible = "menlo,m53cpld" },
    { .compatible = "micron,spi-authenta" },
    { .compatible = "rohm,dh2228fv" },
    { .compatible = "semtech,sx1301" },
    { .compatible = "silabs,em3581" },
    { .compatible = "silabs,si3210" },
    { .compatible = "rockchip,spidev" },
    { /* sentinel */ },
};

static struct rt_spi_driver spidev_driver =
{
    .ids = spidev_ids,
    .ofw_ids = spidev_ofw_ids,

    .probe = spidev_probe,
};
RT_SPI_DRIVER_EXPORT(spidev_driver);
#endif /* RT_USING_DM */
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

## Source: `data/rtos/rt-thread/documentation/6.components/device-driver/spi/spi.md`

```
@page page_device_spi SPI Device

# Introduction to SPI

SPI (Serial Peripheral Interface) is a high-speed, full-duplex, synchronous communication bus commonly used for short-range communication. It is mainly used in EEPROM, FLASH, real-time clock, AD converter, and digital signal processing and between the device and the digital signal decoder. SPI generally uses 4 lines of communication, as shown in the following figure:

![Ways of communication from SPI Master to SPI Slave](figures/spi1.png)

* MOSI :SPI Bus Master Output/Slave Input.

* MISO :SPI Bus Master Input/Slave Output.

* SCLK :Serial Clock, Master device outputs clock signal to slave device.

* CS : select the slave device, also called SS, CSB, CSN, EN, etc., the master device outputs a chip select signal to the slave device.

The SPI works in master-slave mode and usually has one master and one or more slaves. The communication is initiated by the master device. The master device selects the slave device to communicate through CS, and then provides a clock signal to the slave device through SCLK. The data is output to the slave device through the MOSI, and the data sent by the slave device is received through the MISO.

As shown in the figure below, the chip has two SPI controllers. The SPI controller corresponds to the SPI master. Each SPI controller can connect multiple SPI slaves. The slave devices mounted on the same SPI controller share three signal pins: SCK, MISO, MOSI, but the CS pins of each slave device are independent.

![Connect from one SPI controller to multiple SPI slaves](figures/spi2.png)

The master device selects the slave device by controlling the CS pin, typically active low. Only one CS pin is active on an SPI master, and the slave connected to the active CS pin can now communicate with the master.

The slave's clock is provided by the master through SCLK, and MOSI and MISO complete the data transfer based on SCLK. The working timing mode of the SPI is determined by the phase relationship between CPOL (Clock Polarity) and CPHA (Clock Phase). CPOL represents the state of the initial level of the clock signal. A value of 0 indicates that the initial state of the clock signal is low, and a value of 1 indicates that the initial level of the clock signal is high. CPHA indicates on which clock edge the data is sampled. A value of 0 indicates that the data is sampled on the first clock change edge, and a value of 1 indicates that the data is sampled on the second clock change edge. There are 4 working timing modes according to different combinations of CPOL and CPHA: ①CPOL=0, CPHA=0; ②CPOL=0, CPHA=1; ③CPOL=1, CPHA=0; ④CPOL=1, CPHA=1. As shown below:

![4 working timing modes of SPI](figures/spi5.png)

**QSPI:** QSPI is short for Queued SPI and is an extension of the SPI interface from Motorola, which is more extensive than SPI applications. Based on the SPI protocol, Motorola has enhanced its functionality, added a queue transfer mechanism, and introduced a queue serial peripheral interface protocol (QSPI protocol). Using this interface, users can transfer transmission queues containing up to 16 8-bit or 16-bit data at one time. Once the transfer is initiated, CPU is not required until the end of the transfer, greatly improving the transfer efficiency. Compared to SPI, the biggest structural feature of QSPI is the replacement of the transmit and receive data registers of the SPI with 80 bytes of RAM.

**Dual SPI Flash:** For SPI Flash, full-duplex is not commonly used. You can send a command byte into Dual mode and let it work in half-duplex mode to double data transfer. Thus, MOSI becomes SIO0 (serial io 0), and MISO becomes SIO1 (serial io 1), so that 2 bit data can be transmitted in one clock cycle, which doubles the data transmission.

**Quad SPI Flash:** Similar to the Dual SPI, Quad SPI Flash adds two I/O lines (SIO2, SIO3) to transfer 4 bits of data in one clock.

So for SPI Flash, there are three types of standard SPI Flash, Dual SPI Flash, Quad SPI Flash. At the same clock, the higher the number of lines, the higher the transmission rate.

# Mount SPI Device

The SPI driver registers the SPI bus and the SPI device needs to be mounted to the SPI bus that has already been registered.

```C
rt_err_t rt_spi_bus_attach_device(struct rt_spi_device *device,
                                  const char           *name,
                                  const char           *bus_name,
                                  void                 *user_data)
```

| **Parameter** | Description                |
| -------- | ---------------------------------- |
| device     | SPI device handle                  |
| name     |  SPI device name                  |
| bus_name     | SPI bus name                  |
| user_data     | user data pointer                |
| **Return** | ——                                 |
| RT_EOK     | Success     |
| Other Errors | Failure |

This function is used to mount an SPI device to the specified SPI bus, register the SPI device with the kernel, and save user_data to the control block of the SPI device.

The general SPI bus naming principle is spix, and the SPI device naming principle is spixy. For example, spi10 means device 0 mounted on the spi1 bus. User_data is generally the CS pin pointer of the SPI device. When data is transferred, the SPI controller will operate this pin for chip select.

If you use the BSP in the `rt-thread/bsp/stm32` directory, you can use the following function to mount the SPI device to the bus:

```c
rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, GPIO_TypeDef* cs_gpiox, uint16_t cs_gpio_pin);
```

The following sample code mounts the SPI FLASH W25Q128 to the SPI bus:

```c
static int rt_hw_spi_flash_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    rt_hw_spi_device_attach("spi1", "spi10", GPIOB, GPIO_PIN_14);

    if (RT_NULL == rt_sfud_flash_probe("W25Q128", "spi10"))
    {
        return -RT_ERROR;
    };

    return RT_EOK;
}
/* Export to automatic initialization */
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);
```

# Configuring SPI Device

The SPI device's transmission parameters need to be configured after the SPI device is mounted to the SPI bus.

```c
rt_err_t rt_spi_configure(struct rt_spi_device *device,
                          struct rt_spi_configuration *cfg)
```

| **Parameter** | **Description**              |
| -------- | ---------------------------------- |
| device   | SPI device handle               |
| cfg      | SPI configuration parameter pointer |
| **Return** | ——                                 |
| RT_EOK     | Success       |

This function saves the configuration parameters pointed to by `cfg` to the control block of the SPI device device, which is used when transferring data.

The `struct rt_spi_configuration` prototype is as follows:

```c
struct rt_spi_configuration
{
    rt_uint8_t mode;        /* mode */
    rt_uint8_t data_width;  /* data width, 8 bits, 16 bits, 32 bits */
    rt_uint16_t reserved;   /* reserved */
    rt_uint32_t max_hz;     /* maximum frequency */
};
```

**Mode: **Contains MSB/LSB, master-slave mode, timing mode, etc. The available macro combinations are as follows:

```c
/* Set the data transmission order whether the MSB bit is first or the LSB bit is before */
#define RT_SPI_LSB      (0<<2)                        /* bit[2]: 0-LSB */
#define RT_SPI_MSB      (1<<2)                        /* bit[2]: 1-MSB */

/* Set the master-slave mode of the SPI */
#define RT_SPI_MASTER   (0<<3)                        /* SPI master device */
#define RT_SPI_SLAVE    (1<<3)                        /* SPI slave device */

/* Set clock polarity and clock phase */
#define RT_SPI_MODE_0   (0 | 0)                       /* CPOL = 0, CPHA = 0 */
#define RT_SPI_MODE_1   (0 | RT_SPI_CPHA)             /* CPOL = 0, CPHA = 1 */
#define RT_SPI_MODE_2   (RT_SPI_CPOL | 0)             /* CPOL = 1, CPHA = 0 */
#define RT_SPI_MODE_3   (RT_SPI_CPOL | RT_SPI_CPHA)   /* CPOL = 1, CPHA = 1 */

#define RT_SPI_CS_HIGH  (1<<4)                        /* Chipselect active high */
#define RT_SPI_NO_CS    (1<<5)                        /* No chipselect */
#define RT_SPI_3WIRE    (1<<6)                        /* SI/SO pin shared */
#define RT_SPI_READY    (1<<7)                        /* Slave pulls low to pause */
```

**Data width:** The data width format that can be sent and received by the SPI master and SPI slaves is set to 8-bit, 16-bit or 32-bit.

**Maximum Frequency：** Set the baud rate for data transfer, also based on the baud rate range at which the SPI master and SPI slaves operate.

The example for configuration is as follows:

```c
    struct rt_spi_configuration cfg;
    cfg.data_width = 8;
    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = 20 * 1000 *1000;                           /* 20M */

    rt_spi_configure(spi_dev, &cfg);
```

# QSPI Configuration

To configure the transmission parameters of a QSPI device, use the following function:

```c
rt_err_t rt_qspi_configure(struct rt_qspi_device *device, struct rt_qspi_configuration *cfg);
```

| **Parameter** | **Description**                 |
| -------- | ---------------------------------- |
| device   | QSPI device handle           |
| cfg      | QSPI configuration parameter pointer |
| **Return** | ——                                 |
| RT_EOK     | Success       |

This function saves the configuration parameters pointed to by `cfg` to the control block of the QSPI device, which is used when transferring data.

The `struct rt_qspi_configuration` prototype is as follows:

```c
struct rt_qspi_configuration
{
    struct rt_spi_configuration parent;    /* SPI device configuration parent */
    rt_uint32_t medium_size;               /* medium size */
    rt_uint8_t ddr_mode;                   /* double rate mode */
    rt_uint8_t qspi_dl_width ;             /* QSPI bus width, single line mode 1 bit, 2 line mode 2 bits, 4 line mode 4 bits */
};
```

# Access SPI Device

In general, the MCU's SPI device communicates as a master and slave. In the RT-Thread, the SPI master is virtualized as an SPI bus device. The application uses the SPI device management interface to access the SPI slave device. The main interfaces are as follows:

| **Function** | **Description**                |
| -------------------- | ---------------------------------- |
| rt_device_find()  | Find device handles based on SPI device name |
| rt_spi_transfer_message()     | Custom transfer data |
| rt_spi_transfer()     | Transfer data once |
| rt_spi_send()     | Send data once |
| rt_spi_recv()     | Receive data one |
| rt_spi_send_then_send()  | Send data twice |
| rt_spi_send_then_recv()  | Send then Receive |

>The SPI data transfer related interface will call rt_mutex_take(). This function cannot be called in the interrupt service routine, which will cause the assertion to report an error.

## Find SPI Device

Before using the SPI device, you need to find and obtain the device handle according to the SPI device name, so that you can operate the SPI device. The device function is as follows.

```c
rt_device_t rt_device_find(const char* name);
```

| **Parameter** | **Description**                                              |
| ------------- | ------------------------------------------------------------ |
| name          | Device name                                                  |
| **Return**    | ——                                                           |
| device handle | Finding the corresponding device will return the corresponding device handle |
| RT_NULL       | Corresponding device object unfound                          |

In general, the name of the SPI device registered to the system is spi10, qspi10, etc. The usage examples are as follows:

```c
#define W25Q_SPI_DEVICE_NAME     "qspi10"   /* SPI device name */
struct rt_spi_device *spi_dev_w25q;     /* SPI device handle */

/* Find the spi device to get the device handle */
spi_dev_w25q = (struct rt_spi_device *)rt_device_find(W25Q_SPI_DEVICE_NAME);
```

## Transfer Custom Data

By obtaining the SPI device handle, the SPI device management interface can be used to access the SPI device device for data transmission and reception. You can transfer messages by the following function:

```c
struct rt_spi_message *rt_spi_transfer_message(struct rt_spi_device  *device,struct rt_spi_message *message)；
```

| **Parameter**    | **Description**                                              |
| ---------------- | ------------------------------------------------------------ |
| device           | SPI device handle                                            |
| message          | message pointer                                              |
| **Return**       | ——                                                           |
| RT_NULL          | Send successful                                              |
| Non-null pointer | Send failed, return a pointer to the remaining unsent message |

This function can transmit a series of messages, the user can customize the value of each parameter of the message structure to be transmitted, so that the data transmission mode can be conveniently controlled. The `struct rt_spi_message` prototype is as follows:

```c
struct rt_spi_message
{
    const void *send_buf;           /* Send buffer pointer */
    void *recv_buf;                 /* Receive buffer pointer */
    rt_size_t length;               /* Send/receive data bytes */
    struct rt_spi_message *next;    /* Pointer to the next message to continue sending */
    unsigned cs_take    : 1;        /* Take chip selection*/
    unsigned cs_release : 1;        /* Release chip selection */
};
```
send_buf :sendbuf is the send buffer pointer. When the value is RT_NULL, it means that the current transmission is only receiving state, and no data needs to be sent.

recv_buf :recvbuf is the receive buffer pointer. When the value is RT_NULL, it means that the current transmission is in the transmit-only state. It does not need to save the received data, so the received data is directly discarded.

length :The unit of length is word, that is, when the data length is 8 bits, each length occupies 1 byte; when the data length is 16 bits, each length occupies 2 bytes.

next :The parameter next is a pointer to the next message to continue to send. If only one message is sent, the value of this pointer is RT_NULL. Multiple messages to be transmitted are connected together in a singly linked list by the next pointer.

cs_take :A cs_take value of 1 means that the corresponding CS is set to a valid state before data is transferred.

cs_release :A cs_release value of 1 indicates that the corresponding CS is released after the data transfer ends.

>When send_buf or recv_buf is not empty, the available size for both cannot be less than length.
If you use this function to transfer messages, the first message sent by cs_take needs to be set to 1. Set the chip to be valid, and the cs_release of the last message needs to be set to 1. Release the chip select.

An example of use is as follows:

```c
#define W25Q_SPI_DEVICE_NAME     "qspi10"   /* SPI device name */
struct rt_spi_device *spi_dev_w25q;         /* SPI device handle */
struct rt_spi_message msg1, msg2;
rt_uint8_t w25x_read_id = 0x90;             /* command */
rt_uint8_t id[5] = {0};

/* Find the spi device to get the device handle */
spi_dev_w25q = (struct rt_spi_device *)rt_device_find(W25Q_SPI_DEVICE_NAME);
/* Send command to read ID */
struct rt_spi_message msg1, msg2;

msg1.send_buf   = &w25x_read_id;
msg1.recv_buf   = RT_NULL;
msg1.length     = 1;
msg1.cs_take    = 1;
msg1.cs_release = 0;
msg1.next       = &msg2;

msg2.send_buf   = RT_NULL;
msg2.recv_buf   = id;
msg2.length     = 5;
msg2.cs_take    = 0;
msg2.cs_release = 1;
msg2.next       = RT_NULL;

rt_spi_transfer_message(spi_dev_w25q, &msg1);
rt_kprintf("use rt_spi_transfer_message() read w25q ID is:%x%x\n", id[3], id[4]);
```

## Transfer Data Once

If only transfer data for once, use the following function:

```c
rt_size_t rt_spi_transfer(struct rt_spi_device *device,
                          const void           *send_buf,
                          void                  *recv_buf,
                          rt_size_t             length);
```

| **Parameter** | **Description**  |
|----------|----------------------|
| device   | SPI device handle |
| send_buf | Send data buffer pointer |
| recv_buf | Receive data buffer pointer |
| length   | Length of data send/received |
| **Return** | ——                   |
| 0   | Transmission failed |
| Non-0 Value | Length of data successfully transferred |

This function is equivalent to calling `rt_spi_transfer_message()` to transfer a message. When starting to send data, the chip is selected. When the function returns, the chip is released. The message parameter is configured as follows:

```c
struct rt_spi_message msg；

msg.send_buf   = send_buf;
msg.recv_buf   = recv_buf;
msg.length     = length;
msg.cs_take    = 1;
msg.cs_release = 1;
msg.next        = RT_NULL;
```

## Send Data Once

If only send data once and ignore the received data, use the following function:

```c
rt_size_t rt_spi_send(struct rt_spi_device *device,
                      const void           *send_buf,
                      rt_size_t             length)
```

| **Parameter** | **Description** |
|----------|--------------------|
| device   | SPI device handle |
| send_buf | Send data buffer pointer |
| length   | Length of data sent |
| **Return** | ——                 |
| 0    | Transmission failed |
| Non-0 Value | Length of data successfully transferred |

Call this function to send the data of the buffer pointed to by send_buf, ignoring the received data. This function is a wrapper of the `rt_spi_transfer()` function.

This function is equivalent to calling  `rt_spi_transfer_message()` to transfer a message. When the data starts to be sent, the chip is selected. When the function returns, the chip is released. The message parameter is configured as follows:

```c
struct rt_spi_message msg；

msg.send_buf   = send_buf;
msg.recv_buf   = RT_NULL;
msg.length     = length;
msg.cs_take    = 1;
msg.cs_release = 1;
msg.next       = RT_NULL;
```

## Receive Data Once

If only receive data once,  use the following function:

```c
rt_size_t rt_spi_recv(struct rt_spi_device *device,
                      void                 *recv_buf,
                      rt_size_t             length);
```

| **Parameter** | **Description** |
|----------|--------------------|
| device   | SPI device handle |
| recv_buf | Send data buffer pointer |
| length   | Length of data sent |
| **Return** | ——                 |
| 0    | Transmission failed |
| Non-0 Value | Length of data successfully transferred |

Call this function to receive the data and save it to the buffer pointed to by recv_buf. This function is a wrapper of the `rt_spi_transfer()` function. The SPI bus protocol stipulates that the master can only generate a clock, so when receiving data, the master will send the data 0XFF.

This function is equivalent to calling  `rt_spi_transfer_message()` to transfer a message. When receiving data, the chip is selected. When the function returns, the chip is released. The message parameter is configured as follows:

```c
struct rt_spi_message msg；

msg.send_buf   = RT_NULL;
msg.recv_buf   = recv_buf;
msg.length     = length;
msg.cs_take    = 1;
msg.cs_release = 1;
msg.next       = RT_NULL;
```

## Send Data Twice in Succession

If need to send data of 2 buffers in succession and the CS is not released within the process, you can call the following function:

```c
rt_err_t rt_spi_send_then_send(struct rt_spi_device *device,
                               const void           *send_buf1,
                               rt_size_t             send_length1,
                               const void           *send_buf2,
                               rt_size_t             send_length2);
```

| **Parameter** | **Description**        |
|--------------|---------------------------|
| device       | SPI device handle |
| send_buf1    | Send data buffer pointer 1 |
| send_length1 | Send data buffer length 1 |
| send_buf2    | Send data buffer pointer 2 |
| send_length2 | Send data buffer length 2 |
| **Return** | ——                        |
| RT_EOK       | Send Successful |
| -RT_EIO     | Send Failed        |

This function can continuously send data of 2 buffers, ignore the received data, select the CS when send_buf1 is sent, and release the CS after sending send_buf2.

This function is suitable for writing a piece of data to the SPI device, sending data such as commands and addresses for the first time, and sending data of the specified length for the second time. The reason is that it is sent twice instead of being merged into one data block, or `rt_spi_send()`is called twice, because in most data write operations, commands and addresses need to be sent first, and the length is usually only a few bytes. If send it in conjunction with the data that follows, it will need a memory space request and a lot of data handling. If `rt_spi_send()`is called twice, the chip select will be released after the command and address are sent. Most SPI devices rely on setting the chip select once to be the start of the command, so the chip selects the command or address after sending. After the data is released, the operation is discarded.

This function is equivalent to calling  `rt_spi_transfer_message()` to transfer 2 messages. The message parameter is configured as follows:

```c
struct rt_spi_message msg1,msg2；

msg1.send_buf   = send_buf1;
msg1.recv_buf   = RT_NULL;
msg1.length     = send_length1;
msg1.cs_take    = 1;
msg1.cs_release = 0;
msg1.next       = &msg2;

msg2.send_buf   = send_buf2;
msg2.recv_buf   = RT_NULL;
msg2.length     = send_length2;
msg2.cs_take    = 0;
msg2.cs_release = 1;
msg2.next       = RT_NULL;
```

## Receive Data After Sending Data

If need to send data to the slave device first, then receive the data sent from the slave device, and the CS is not released within the process, call the following function to implement:

```c
rt_err_t rt_spi_send_then_recv(struct rt_spi_device *device,
                               const void           *send_buf,
                               rt_size_t             send_length,
                               void                 *recv_buf,
                               rt_size_t             recv_length);
```

| **Parameter** | **Description**       |
|-------------|--------------------------|
| device      | SPI slave device handle |
| send_buf    | Send data buffer pointer |
| send_length | Send data buffer length |
| recv_buf | Receive data buffer pointer |
| recv_length | Receive data buffer length |
| **Return** | ——                       |
| RT_EOK      | Successful            |
| -RT_EIO    | Failed                |

This function select CS when sending the first data send_buf when the received data is ignored, and the second data is sent. At this time, the master device will send the data 0XFF, and the received data will be saved in recv_buf, and CS will be released when the function returns.

This function is suitable for reading a piece of data from the SPI slave device. The first time it will send some command and address data, and then receive the data of the specified length.

This function is equivalent to calling  `rt_spi_transfer_message()` to transfer 2 messages. The message parameter is configured as follows:

```c
struct rt_spi_message msg1,msg2；

msg1.send_buf   = send_buf;
msg1.recv_buf   = RT_NULL;
msg1.length     = send_length;
msg1.cs_take    = 1;
msg1.cs_release = 0;
msg1.next       = &msg2;

msg2.send_buf   = RT_NULL;
msg2.recv_buf   = recv_buf;
msg2.length     = recv_length;
msg2.cs_take    = 0;
msg2.cs_release = 1;
msg2.next       = RT_NULL;
```

The SPI device management module also provides  `rt_spi_sendrecv
/* ... truncated ... */
```
