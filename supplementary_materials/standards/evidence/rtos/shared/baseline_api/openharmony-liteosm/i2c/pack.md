# Raw RTOS/Bus Pack: openharmony-liteosm / i2c

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/framework/include/platform/i2c_if.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup I2C
 * @{
 *
 * @brief Provides standard Inter-Integrated Circuit (I2C) interfaces.
 *
 * This module allows a driver to perform operations on an I2C controller for accessing devices on the I2C bus,
 * including creating and destroying I2C controller handles as well as reading and writing data.
 *
 * @since 1.0
 */

/**
 * @file i2c_if.h
 *
 * @brief Declares the standard I2C interface functions.
 *
 * @since 1.0
 */

#ifndef I2C_IF_H
#define I2C_IF_H

#include "platform_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief Defines the I2C transfer message used during custom transfers.
 *
 * @attention This structure does not limit the data transfer length specified by <b>len</b>.
 * The specific I2C controller determines the maximum length allowed. \n
 * The device address <b>addr</b> indicates the original device address and does not need to
 * contain the read/write flag bit.
 *
 * @since 1.0
 */
struct I2cMsg {
    /** Address of the I2C device */
    uint16_t addr;
    /** Address of the buffer for storing transferred data */
    uint8_t *buf;
    /** Length of the transferred data */
    uint16_t len;
    /**
     * Transfer Mode Flag | Description
     * ------------| -----------------------
     * I2C_FLAG_READ | Read flag
     * I2C_FLAG_ADDR_10BIT | 10-bit addressing flag
     * I2C_FLAG_READ_NO_ACK | No-ACK read flag
     * I2C_FLAG_IGNORE_NO_ACK | Ignoring no-ACK flag
     * I2C_FLAG_NO_START | No START condition flag
     * I2C_FLAG_STOP | STOP condition flag
     */
    uint16_t flags;
};

/**
 * @brief Enumerates flags used for transferring I2C messages.
 *
 * Multiple flags can be used to jointly control a single I2C message transfer.
 * If a bit is set to <b>1</b>, the corresponding feature is enabled. If a bit is set to <b>0</b>,
 * the corresponding feature is disabled.
 *
 * @since 1.0
 */
enum I2cFlag {
    /** Read flag. The value <b>1</b> indicates the read operation, and <b>0</b> indicates the write operation. */
    I2C_FLAG_READ           = (0x1 << 0),
    /** 10-bit addressing flag. The value <b>1</b> indicates that a 10-bit address is used. */
    I2C_FLAG_ADDR_10BIT     = (0x1 << 4),
    /** Dma flag. The value <b>1</b> indicates that the buffer of this message is DMA safe. */
    I2C_FLAG_DMA            = (0x1 << 9),
    /** Non-ACK read flag. The value <b>1</b> indicates that no ACK signal is sent during the read process. */
    I2C_FLAG_READ_NO_ACK    = (0x1 << 11),
    /** Ignoring no-ACK flag. The value <b>1</b> indicates that the non-ACK signal is ignored. */
    I2C_FLAG_IGNORE_NO_ACK  = (0x1 << 12),
    /**
     * No START condition flag. The value <b>1</b> indicates that there is no START condition for the message
     * transfer.
     */
    I2C_FLAG_NO_START       = (0x1 << 14),
    /** STOP condition flag. The value <b>1</b> indicates that the current transfer ends with a STOP condition. */
    I2C_FLAG_STOP           = (0x1 << 15),
};

/**
 * @brief Obtains the handle of an I2C controller.
 *
 * You must call this function before accessing the I2C bus.
 *
 * @param number Indicates the I2C controller ID.
 *
 * @return Returns the pointer to the {@link DevHandle} of the I2C controller if the operation is successful;
 * returns <b>NULL</b> otherwise.
 * @since 1.0
 */
DevHandle I2cOpen(int16_t number);

 /**
 * @brief Releases the handle of an I2C controller.
 *
 * If you no longer need to access the I2C controller, you should call this function to close its handle so as
 * to release unused memory resources.
 *
 * @param handle Indicates the pointer to the device handle of the I2C controller.
 *
 * @since 1.0
 */
void I2cClose(DevHandle handle);

/**
 * @brief Launches a custom transfer to an I2C device.
 *
 * @param handle Indicates the pointer to the device handle of the I2C controller obtained via {@link I2cOpen}.
 * @param msgs Indicates the pointer to the I2C transfer message structure array.
 * @param count Indicates the length of the message structure array.
 *
 * @return Returns the number of transferred message structures if the operation is successful;
 * returns a negative value otherwise.
 * @see I2cMsg
 * @attention This function does not limit the number of message structures specified by <b>count</b> or the data
 * length of each message structure. The specific I2C controller determines the maximum number and data length allowed.
 *
 * @since 1.0
 */
int32_t I2cTransfer(DevHandle handle, struct I2cMsg *msgs, int16_t count);

/**
 * @brief Enumerates I2C I/O commands.
 *
 * @since 1.0
 */
enum I2cIoCmd {
    I2C_IO_TRANSFER = 0,      /**< Execute one or more I2C messages. */
    I2C_IO_OPEN = 1,          /**< Open the I2C device. */
    I2C_IO_CLOSE = 2,         /**< Close the I2C device. */
};

/**
 * @brief The following i2c interfaces are only available for the mini platform
 *
 * @since 1.0
 */

int32_t I2cRead(DevHandle handle, uint8_t *buf, uint16_t len);

int32_t I2cWrite(DevHandle handle, uint8_t *buf, uint16_t len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* I2C_IF_H */
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
