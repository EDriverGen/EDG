# Raw RTOS/Bus Pack: openharmony-liteosm / uart_polling

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/framework/include/platform/uart_if.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup UART
 * @{
 *
 * @brief Defines standard APIs of universal asynchronous receiver/transmitter (UART) capabilities.
 *
 * You can use this module to access the UART and enable the driver to operate a UART-compliant device.
 * The functions in this module help you to obtain and release the UART device handle, read and write data,
 * obtain and set the baud rate and device attributes.
 *
 * @since 1.0
 */

/**
 * @file uart_if.h
 *
 * @brief Declares standard UART APIs.
 *
 * @since 1.0
 */

#ifndef UART_IF_H
#define UART_IF_H

#include "platform_if.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines basic attributes of the UART port.
 *
 * You can configure the attributes via {@link UartSetAttribute}. If the parameters are not set,
 * default attributes are used.
 *
 * @attention The UART controller determines which UART attribute parameters are supported.
 *
 * @since 1.0
 */
#pragma pack(push, 4)
struct UartAttribute {
    /**
     * Data Bit | Description
     * ------------| -----------------------
     * UART_ATTR_DATABIT_8 | 8 data bits
     * UART_ATTR_DATABIT_7 | 7 data bits
     * UART_ATTR_DATABIT_6 | 6 data bits
     * UART_ATTR_DATABIT_5 | 5 data bits
    */
    unsigned int dataBits : 4;
/**
 * @brief Indicates the UART word length, which is 8 data bits per frame.
 *
 * @since 1.0
 */
#define UART_ATTR_DATABIT_8 0
/**
 * @brief Indicates the UART word length, which is 7 data bits per frame.
 *
 * @since 1.0
 */
#define UART_ATTR_DATABIT_7 1
/**
 * @brief Indicates the UART word length, which is 6 data bits per frame.
 *
 * @since 1.0
 */
#define UART_ATTR_DATABIT_6 2
/**
 * @brief Indicates the UART word length, which is 5 data bits per frame.
 *
 * @since 1.0
 */
#define UART_ATTR_DATABIT_5 3
    /**
     * Parity Bit | Description
     * ------------| -----------------------
     * UART_ATTR_PARITY_NONE | No parity bit
     * UART_ATTR_PARITY_ODD | Odd parity bit
     * UART_ATTR_PARITY_EVEN | Even parity bit
     * UART_ATTR_PARITY_MARK | <b>1</b>
     * UART_ATTR_PARITY_SPACE | <b>0</b>
     */
    unsigned int parity : 4;
/**
 * @brief Indicates that the UART device has no parity bit.
 *
 * @since 1.0
 */
#define UART_ATTR_PARITY_NONE 0
/**
 * @brief Indicates that the UART device has an odd parity bit.
 *
 * @since 1.0
 */
#define UART_ATTR_PARITY_ODD 1
/**
 * @brief Indicates that the UART device has an even parity bit.
 *
 * @since 1.0
 */
#define UART_ATTR_PARITY_EVEN 2
/**
 * @brief Indicates that the parity bit is 1.
 *
 * @since 1.0
 */
#define UART_ATTR_PARITY_MARK 3
/**
* @brief Indicates that the parity bit is 0.
 *
 * @since 1.0
 */
#define UART_ATTR_PARITY_SPACE 4
    /**
     * Stop Bit | Description
     * ------------| -----------------------
     * UART_ATTR_STOPBIT_1 | 1 stop bit
     * UART_ATTR_STOPBIT_1P5 | 1.5 stop bits
     * UART_ATTR_STOPBIT_2 | 2 stop bits
     */
    unsigned int stopBits : 4;
/**
 * @brief Indicates that the UART device has 1 stop bit.
 *
 * @since 1.0
 */
#define UART_ATTR_STOPBIT_1 0
/**
 * @brief Indicates that the UART device has 1.5 stop bits.
 *
 * @since 1.0 */
#define UART_ATTR_STOPBIT_1P5 1
/**
 * @brief Indicates that the UART device has 2 stop bits.
 *
 * @since 1.0
 */
#define UART_ATTR_STOPBIT_2 2
    /**
     * RTS | Description
     * ------------| -----------------------
     * UART_ATTR_RTS_DIS | RTS disabled
     * UART_ATTR_RTS_EN | RTS enabled
     */
    unsigned int rts : 1;
/**
* @brief Indicates that Request To Send (RTS) is disabled for the UART device.
 *
 * @since 1.0
 */
#define UART_ATTR_RTS_DIS 0
/**
 * @brief Indicates that RTS is enabled for the UART device.
 *
 * @since 1.0
 */
#define UART_ATTR_RTS_EN 1
    /**
     * CTS | Description
     * ------------| -----------------------
     * UART_ATTR_CTS_DIS | CTS disabled
     * UART_ATTR_CTS_EN | CTS enabled
     */
    unsigned int cts : 1;
/**
 * @brief Indicates that Clear To Send (CTS) is disabled for the UART device.
 *
 * @since 1.0
 */
#define UART_ATTR_CTS_DIS 0
/**
 * @brief Indicates that CTS is enabled for the UART device.
 *
 * @since 1.0
 */
#define UART_ATTR_CTS_EN 1
    /**
     * Receiver FIFO | Description
     * ------------| -----------------------
     * UART_ATTR_RX_FIFO_DIS | FIFO disabled
     * UART_ATTR_RX_FIFO_EN | FIFO enabled
     */
    unsigned int fifoRxEn : 1;
/**
 * @brief Indicates that First In First Out (FIFO) is disabled for the receiving UART.
 *
 * @since 1.0
 */
#define UART_ATTR_RX_FIFO_DIS 0
/**
 * @brief Indicates that FIFO is enabled for the receiving UART.
 *
 * @since 1.0
 */
#define UART_ATTR_RX_FIFO_EN 1
    /**
     * Transmitter FIFO | Description
     * ------------| -----------------------
     * UART_ATTR_TX_FIFO_DIS | FIFO disabled
     * UART_ATTR_TX_FIFO_EN | FIFO enabled
     */
    unsigned int fifoTxEn : 1;
/**
 * @brief Indicates that FIFO is disabled for the transmitting UART.
 *
 * @since 1.0
 */
#define UART_ATTR_TX_FIFO_DIS 0
/**
 * @brief Indicates that FIFO is enabled for the transmitting UART.
 *
 * @since 1.0
 */
#define UART_ATTR_TX_FIFO_EN 1
    /** Reserved bits */
    unsigned int reserved : 16;
};
#pragma pack(pop)

/**
 * @brief Enumerates UART transmission modes.
 *
 * @attention The UART controller determines whether an enumerated transmission mode is supported.
 *
 * @since 1.0
 */
enum UartTransMode {
    UART_MODE_RD_BLOCK = 0,  /**< Blocking mode */
    UART_MODE_RD_NONBLOCK,   /**< Non-blocking mode */
    UART_MODE_DMA_RX_EN,     /**< DMA enabled for data receiving */
    UART_MODE_DMA_RX_DIS,    /**< DMA disabled for data receiving */
    UART_MODE_DMA_TX_EN,     /**< DMA enabled for data transmitting */
    UART_MODE_DMA_TX_DIS,    /**< DMA disabled for data transmitting */
};

/**
 * @brief Enumerates UART I/O commands.
 *
 * @since 1.0
 */
enum UartIoCmd {
    UART_IO_REQUEST = 0,     /**< Reference count management and initialize the UART device. */
    UART_IO_RELEASE,         /**< Reference count management and deinitialize the UART device. */
    UART_IO_READ,            /**< Read data. */
    UART_IO_WRITE,           /**< Write data. */
    UART_IO_GET_BAUD,        /**< Obtain the baud rate. */
    UART_IO_SET_BAUD,        /**< Set the baud rate. */
    UART_IO_GET_ATTRIBUTE,   /**< Obtain the device attributes. */
    UART_IO_SET_ATTRIBUTE,   /**< Set the device attributes. */
    UART_IO_SET_TRANSMODE,   /**< Set the transmission mode. */
};

/**
 * @brief Obtains the UART device handle.
 *
 * Before accessing the UART device, you must call this function to obtain the UART device handle.
 *
 * @param port Indicates the UART port.
 *
 * @return Returns the pointer to the UART device handle if the handle is obtained; returns <b>NULL</b> otherwise.
 * @since 1.0
 */
DevHandle UartOpen(uint32_t port);

/**
 * @brief Releases the UART device handle.
 *
 * If you no longer need to access the UART device, you should call this function to close its handle so as to
 * release unused memory resources.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 *
 * @since 1.0
 */
void UartClose(DevHandle handle);

/**
 * @brief Reads data of a specified size from a UART device.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param data Indicates the pointer to the buffer for receiving the data.
 * @param size Indicates the size of the data to read.
 *
 * @return Returns the size of the data that is successfully read; returns a negative number if the reading fails.
 * @since 1.0
 */
int32_t UartRead(DevHandle handle, uint8_t *data, uint32_t size);

/**
 * @brief Writes data of a specified size into a UART device.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param data Indicates the pointer to the data to write.
 * @param size Indicates the size of the data to write.
 *
 * @return Returns <b>0</b> if the data is successfully written; returns a negative number otherwise.
 * @since 1.0
 */
int32_t UartWrite(DevHandle handle, uint8_t *data, uint32_t size);

/**
 * @brief Obtains the baud rate of the UART device.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param baudRate Indicates the pointer to the obtained baud rate.
 *
 * @return Returns <b>0</b> if the baud rate is obtained; returns a negative number otherwise.
 * @since 1.0
 */
int32_t UartGetBaud(DevHandle handle, uint32_t *baudRate);

/**
 * @brief Sets the baud rate for the UART device.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param baudRate Indicates the baud rate to set.
 *
 * @return Returns <b>0</b> if the setting is successful; returns a negative number otherwise.
 * @since 1.0
 */
int32_t UartSetBaud(DevHandle handle, uint32_t baudRate);

/**
 * @brief Obtains the UART attribute.
 *
 * UART attributes include data bits, stop bits, parity bit, CTS, RTS, and receiving and transmitting FIFO.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param attribute Indicates the pointer to the obtained UART attribute.
 *
 * @return Returns <b>0</b> if the UART attribute is obtained; returns a negative number otherwise.
 * @since 1.0 */
int32_t UartGetAttribute(DevHandle handle, struct UartAttribute *attribute);

/**
 * @brief Sets the UART attribute.
 *
 * UART attributes include data bits, stop bits, parity bit, CTS, RTS, and receiving and transmitting FIFO.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param attribute Indicates the pointer to the UART attribute to set.
 *
 * @return Returns <b>0</b> if the setting is successful; returns a negative number otherwise.
 * @since 1.0
 */
int32_t UartSetAttribute(DevHandle handle, struct UartAttribute *attribute);

/**
 * @brief Sets the UART transmission mode.
 *
 * @param handle Indicates the pointer to the UART device handle, which is obtained via {@link UartOpen}.
 * @param mode Indicates a transmission mode enumerated in {@linkUartTransMode}.
 *
 * @return Returns <b>0</b> if the setting is successful; returns a negative number otherwise.
 * @since 1.0
 */
int32_t UartSetTransMode(DevHandle handle, enum UartTransMode mode);

/**
 * @brief The following uart interface is only available for the mini platform
 *
 * @since 1.0
 */
int32_t UartBlockWrite(DevHandle handle, uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PAL_UART_IF_H */
/** @} */
```

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/framework/include/platform/platform_if.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup COMMON
 * @{
 *
 * @brief Provides common APIs of the platform driver.
 *
 * This module also provides <b>DevHandle</b>, which represents the common data structure of the platform driver.
 *
 * @since 1.0
 */

/**
 * @file platform_if.h
 *
 * @brief Declares common APIs of the platform driver.
 *
 * @since 1.0
 */

#ifndef PLATFORM_IF_H
#define PLATFORM_IF_H

#include "hdf_base.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief Defines the common device handle of the platform driver.
 *
 * The handle is associated with a specific platform device and is used as the
 * first input parameter for all APIs of the platform driver.
 *
 * @since 1.0
 */
typedef void* DevHandle;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* PLATFORM_IF_H */
/** @} */
```

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/interfaces/inner_api/osal/shared/osal_time.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup OSAL
 * @{
 *
 * @brief Defines the structures and interfaces for the Operating System Abstraction Layer (OSAL) module.
 *
 * The OSAL module OpenHarmony OS interface differences and provides unified OS interfaces externally,
 * including the memory management, thread, mutex, spinlock, semaphore, timer, file, interrupt, time,
 * atomic, firmware, and I/O operation modules.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file osal_time.h
 *
 * @brief Declares the time, sleep, and delay interfaces.
 *
 * @since 1.0
 * @version 1.0
 */
#ifndef OSAL_TIME_H
#define OSAL_TIME_H

#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines time.
 */
typedef struct {
    uint64_t sec; /**< Second */
    uint64_t usec; /**< Microsecond */
} OsalTimespec;

/**
 * @brief Describes thread sleep, in seconds.
 *
 * When a thread invokes this function, the CPU is released and the thread enters the sleep state.
 *
 * @param sec Indicates the sleep time, in seconds.
 * @since 1.0
 * @version 1.0
 */
void OsalSleep(uint32_t sec);

/**
 * @brief Describes thread sleep, in milliseconds.
 *
 * When a thread invokes this function, the CPU is released and the thread enters the sleep state.
 *
 * @param ms Indicates the sleep time, in milliseconds.
 * @since 1.0
 * @version 1.0
 */
void OsalMSleep(uint32_t ms);

/**
 * @brief Describes thread sleep, in microsecond.
 *
 * When a thread invokes this function, the CPU is released and the thread enters the sleep state.
 *
 * @param us Indicates the sleep time, in microsecond.
 * @since 1.0
 * @version 1.0
 */
void OsalUSleep(uint32_t us);

/**
 * @brief Obtains the second and microsecond time.
 *
 * @param time Indicates the pointer to the time structure {@link OsalTimespec}.
 *
 * @return Returns a value listed below: \n
 * HDF_STATUS | Description
 * ----------------------| -----------------------
 * HDF_SUCCESS | The operation is successful.
 * HDF_FAILURE | Failed to invoke the system function to obtain time.
 * HDF_ERR_INVALID_PARAM | Invalid parameter.
 *
 * @since 1.0
 * @version 1.0
 */
int32_t OsalGetTime(OsalTimespec *time);

/**
 * @brief Obtains time difference.
 *
 * @param start Indicates the pointer to the start time {@link OsalTimespec}.
 * @param end Indicates the pointer to the end time {@link OsalTimespec}.
 * @param diff Indicates the pointer to the time difference {@link OsalTimespec}.
 *
 * @return Returns a value listed below: \n
 * HDF_STATUS | Description
 * ----------------------| -----------------------
 * HDF_SUCCESS | The operation is successful.
 * HDF_ERR_INVALID_PARAM | Invalid parameter.
 *
 * @since 1.0
 * @version 1.0
 */
int32_t OsalDiffTime(const OsalTimespec *start, const OsalTimespec *end, OsalTimespec *diff);

/**
 * @brief Obtains the system time.
 *
 * @return Returns the system time, in milliseconds.
 * @since 1.0
 * @version 1.0
 */
uint64_t OsalGetSysTimeMs(void);

/**
 * @brief Describes thread delay, in milliseconds.
 *
 * When a thread invokes this function, the CPU is not released. This function returns after waiting for milliseconds.
 *
 * @param ms Indicates the delay time, in milliseconds.
 * @since 1.0
 * @version 1.0
 */
void OsalMDelay(uint32_t ms);

/**
 * @brief Describes thread delay, in microseconds.
 *
 * When a thread invokes this function, the CPU is not released. This function returns after waiting for microseconds.
 *
 * @param us Indicates the delay time, in microseconds.
 * @since 1.0
 * @version 1.0
 */
void OsalUDelay(uint32_t us);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OSAL_TIME_H */
/** @} */
```

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/interfaces/inner_api/utils/hdf_base.h`

```c
/*
 * Copyright (c) 2020-2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup DriverUtils
 * @{
 *
 * @brief Defines common macros and interfaces of the driver module.
 *
 * This module provides interfaces such as log printing, doubly linked list operations, and work queues.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file hdf_base.h
 *
 * @brief Declares driver common types, including the enumerated values returned by the function
 * and the macro for obtaining the array size.
 *
 * @since 1.0
 * @version 1.0
 */
#ifndef HDF_BASE_TYPE_H
#define HDF_BASE_TYPE_H

#include "hdf_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Enumerates HDF return value types.
 */
typedef enum {
    HDF_SUCCESS  = 0, /**< The operation is successful. */
    HDF_FAILURE = -1, /**< Failed to invoke the OS underlying function. */
    HDF_ERR_NOT_SUPPORT = -2, /**< Not supported. */
    HDF_ERR_INVALID_PARAM = -3, /**< Invalid parameter. */
    HDF_ERR_INVALID_OBJECT = -4, /**< Invalid object. */
    HDF_ERR_MALLOC_FAIL    = -6, /**< Memory allocation fails. */
    HDF_ERR_TIMEOUT        = -7, /**< Timeout occurs. */
    HDF_ERR_THREAD_CREATE_FAIL = -10, /**< Failed to create a thread. */
    HDF_ERR_QUEUE_FULL  = -15, /**< The queue is full. */
    HDF_ERR_DEVICE_BUSY = -16, /**< The device is busy. */
    HDF_ERR_IO          = -17, /**< I/O error. */
    HDF_ERR_BAD_FD      = -18, /**< Incorrect file descriptor. */
    HDF_ERR_NOPERM      = -19, /**< No permission. */
    HDF_ERR_OUT_OF_RANGE = -20, /**< Failed to get all result */

#define HDF_BSP_ERR_START (-100) /**< Defines the start of the Board Support Package (BSP) module error codes. */
#define HDF_BSP_ERR_NUM(v) (HDF_BSP_ERR_START + (v)) /**< Defines the BSP module error codes. */
    HDF_BSP_ERR_OP = HDF_BSP_ERR_NUM(-1), /**< Failed to operate a BSP module. */
    HDF_ERR_BSP_PLT_API_ERR = HDF_BSP_ERR_NUM(-2), /**< The platform API of the BSP module is incorrect. */
    HDF_PAL_ERR_DEV_CREATE = HDF_BSP_ERR_NUM(-3), /**< Failed to create a BSP module device. */
    HDF_PAL_ERR_INNER = HDF_BSP_ERR_NUM(-4), /**< Internal error codes of the BSP module. */

#define HDF_DEV_ERR_START (-200) /**< Defines the start of the device module error codes. */
#define HDF_DEV_ERR_NUM(v) (HDF_DEV_ERR_START + (v)) /**< Defines the device module error codes. */
    HDF_DEV_ERR_NO_MEMORY               = HDF_DEV_ERR_NUM(-1), /**< Failed to allocate memory to the device module. */
    HDF_DEV_ERR_NO_DEVICE               = HDF_DEV_ERR_NUM(-2), /**< The device module has no device. */
    HDF_DEV_ERR_NO_DEVICE_SERVICE       = HDF_DEV_ERR_NUM(-3), /**< The device module has no device service. */
    HDF_DEV_ERR_DEV_INIT_FAIL           = HDF_DEV_ERR_NUM(-4), /**< Failed to initialize a device module. */
    HDF_DEV_ERR_PUBLISH_FAIL            = HDF_DEV_ERR_NUM(-5), /**< The device module failed to release a service. */
    HDF_DEV_ERR_ATTACHDEV_FAIL          = HDF_DEV_ERR_NUM(-6), /**< Failed to attach a device to a device module. */
    HDF_DEV_ERR_NODATA                  = HDF_DEV_ERR_NUM(-7), /**< Failed to read data from a device module. */
    HDF_DEV_ERR_NORANGE                 = HDF_DEV_ERR_NUM(-8), /**< The device module data is out of range. */
    HDF_DEV_ERR_OP                      = HDF_DEV_ERR_NUM(-10), /**< Failed to operate a device module. */
    HDF_DEV_ERR_NETDOWN                 = HDF_DEV_ERR_NUM(-11), /**< The network is down. */
} HDF_STATUS;

/**
 * @brief Indicates that the function keeps waiting to obtain a semaphore or mutex.
 */
#define HDF_WAIT_FOREVER 0xFFFFFFFF

/**
 * @brief Defines the array size.
 */
#define HDF_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * @brief Defines a time conversion unit, for example, the unit for converting from second to millisecond.
 */
#define HDF_KILO_UNIT 1000

#ifdef __LITEOS__
/**
 * @brief Declares the full path of the HDF module library.
 */
#define HDF_LIBRARY_FULL_PATH(x) "/usr/lib/" x ".so"

/**
 * @brief Declares the directory of the HDF module library.
 */
#define HDF_LIBRARY_DIR "/usr/lib"

/**
 * @brief Declares the directory of the HDF module configuration files.
 */
#define HDF_ETC_DIR "/etc"

/**
 * @brief Declares the configuration directory of the HDF module.
 */
#define HDF_CONFIG_DIR "/etc"

/**
 * @brief Declares the directory of the HCS configuration file of the HDF.
 */
#define HDF_CHIP_PROD_CONFIG_DIR "/etc"

/**
 * @brief Declares the name of the HDF module library.
 */
#define HDF_LIBRARY_NAME(x) x ".so"
#else
/**
 * @brief Declares the full path of the HDF module library.
 */
#if (defined(__aarch64__) || defined(__x86_64__))
#define HDF_LIBRARY_FULL_PATH(x) "/vendor/lib64/" x ".z.so"
#else
#define HDF_LIBRARY_FULL_PATH(x) "/vendor/lib/" x ".z.so"
#endif

/**
 * @brief Declares the directory of the HDF module library.
 */
#define HDF_LIBRARY_DIR "/vendor/lib"

/**
 * @brief Declares the directory of the HDF module configuration files.
 */
#define HDF_ETC_DIR "/vendor/etc"

/**
 * @brief Declares the configuration directory of the HDF module.
 */
#define HDF_CONFIG_DIR "/vendor/etc/hdfconfig"

/**
 * @brief Declares the directory of the HCS configuration file of the HDF.
 */
#define HDF_CHIP_PROD_CONFIG_DIR "/chip_prod/etc/hdfconfig"

/**
 * @brief Declares the installation directory of the HDF kernel-mode service module driver.
 */
#define HDF_MODULE_DIR "/vendor/modules/"

/**
 * @brief Declares the name of the HDF module library.
 */
#define HDF_LIBRARY_NAME(x) x ".z.so"
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDF_BASE_TYPE_H */
/** @} */
```

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/interfaces/inner_api/utils/hdf_log.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup DriverUtils
 * @{
 *
 * @brief Defines common macros and interfaces of the driver module.
 *
 * This module provides interfaces such as log printing, doubly linked list operations, and work queues.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file hdf_log.h
 *
 * @brief Declares log printing functions of the driver module.
 * This module provides functions for printing logs at the verbose, debug, information, warning, and error levels.
 *
 * To use these functions, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef HDF_LOG_H
#define HDF_LOG_H

#ifdef HDF_LOG_TAG
#undef HDF_LOG_TAG
#endif /* HDF_LOG_TAG */

/** Add quotation mark */
#define LOG_TAG_MARK_EXTEND(HDF_TAG) #HDF_TAG
#define LOG_TAG_MARK(HDF_TAG) LOG_TAG_MARK_EXTEND(HDF_TAG)

#ifndef LOG_TAG
#define LOG_TAG LOG_TAG_MARK(HDF_LOG_TAG)
#endif /* LOG_TAG */

#include "hdf_log_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Prints logs at the verbose level.
 *
 * To use this function, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#define HDF_LOGV(fmt, args...) HDF_LOGV_WRAPPER(fmt, ##args)
/**
 * @brief Prints logs at the debug level.
 *
 * To use this function, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#define HDF_LOGD(fmt, args...) HDF_LOGD_WRAPPER(fmt, ##args)
/**
 * @brief Prints logs at the information level.
 *
 * To use this function, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#define HDF_LOGI(fmt, args...) HDF_LOGI_WRAPPER(fmt, ##args)
/**
 * @brief Prints logs at the warning level.
 *
 * To use this function, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#define HDF_LOGW(fmt, args...) HDF_LOGW_WRAPPER(fmt, ##args)
/**
 * @brief Prints logs at the error level.
 *
 * To use this function, you must define <b>HDF_LOG_TAG</b>, for example, #define HDF_LOG_TAG evt.
 *
 * @since 1.0
 * @version 1.0
 */
#define HDF_LOGE(fmt, args...) HDF_LOGE_WRAPPER(fmt, ##args)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HDF_LOG_H */
/** @} */
```
