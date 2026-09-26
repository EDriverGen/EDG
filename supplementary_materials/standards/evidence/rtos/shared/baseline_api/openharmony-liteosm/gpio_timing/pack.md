# Raw RTOS/Bus Pack: openharmony-liteosm / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/framework/include/platform/gpio_if.h`

```c
/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

/**
 * @addtogroup GPIO
 * @{
 *
 * @brief Provides standard general-purpose input/output (GPIO) interfaces for driver development.
 *
 * You can use this module to perform operations on a GPIO pin, including setting the input/output direction,
 * reading/writing the level value, and setting the interrupt service routine (ISR) function.
 *
 * @since 1.0
 */

/**
 * @file gpio_if.h
 *
 * @brief Declares the standard GPIO interface functions.
 *
 * @since 1.0
 */

#ifndef GPIO_IF_H
#define GPIO_IF_H

#include "platform_if.h"
#include "osal_irq.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/**
 * @brief Enumerates GPIO level values.
 *
 * @since 1.0
 */
enum GpioValue {
    GPIO_VAL_LOW  = 0, /**< Low GPIO level */
    GPIO_VAL_HIGH = 1, /**< High GPIO level */
    GPIO_VAL_ERR,      /**< Invalid GPIO level */
};

/**
 * @brief Enumerates GPIO directions.
 *
 * @since 1.0
 */
enum GpioDirType {
    GPIO_DIR_IN  = 0,  /**< Input direction */
    GPIO_DIR_OUT = 1,  /**< Output direction */
    GPIO_DIR_ERR,      /**< Invalid direction */
};

/**
 * @brief Enumerates GPIO irq types.
 *
 * @since 1.0
 */
enum GpioIrqType {
    /** Trigger is not set */
    GPIO_IRQ_TRIGGER_NONE = OSAL_IRQF_TRIGGER_NONE,
    /** Rising edge triggered */
    GPIO_IRQ_TRIGGER_RISING = OSAL_IRQF_TRIGGER_RISING,
    /** Falling edge triggered */
    GPIO_IRQ_TRIGGER_FALLING = OSAL_IRQF_TRIGGER_FALLING,
    /** High-level triggered */
    GPIO_IRQ_TRIGGER_HIGH = OSAL_IRQF_TRIGGER_HIGH,
    /** Low-level triggered */
    GPIO_IRQ_TRIGGER_LOW = OSAL_IRQF_TRIGGER_LOW,
    /** execute interrupt service routine in thread context */
    GPIO_IRQ_USING_THREAD = (0x1 << 8),
};

/**
 * @brief Defines the function type of a GPIO interrupt service routine (ISR).
 *
 * This function is used when you call {@link GpioSetIrq} to register the ISR for a GPIO pin.
 *
 * @param gpio Indicates the GPIO number of the ISR.
 * @param data Indicates the pointer to the private data passed to this ISR (The data is specified when the ISR
 * is registered).
 *
 * @return Returns <b>0</b> if the ISR function type is successfully defined; returns a negative value otherwise.
 * @see GpioSetIrq
 * @since 1.0
 */
typedef int32_t (*GpioIrqFunc)(uint16_t gpio, void *data);

/**
 * @brief Reads the level value of a GPIO pin.
 *
 * Before using this function, you need to call {@link GpioSetDir} to set the GPIO pin direction to input.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param val Indicates the pointer to the read level value. For details, see {@link GpioValue}.
 *
 * @return Returns <b>0</b> if the GPIO pin level value is successfully read; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioRead(uint16_t gpio, uint16_t *val);

/**
 * @brief Writes the level value for a GPIO pin.
 *
 * Before using this function, you need to call {@link GpioSetDir} to set the GPIO pin direction to output.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param val Indicates the level value to be written. For details, see {@link GpioValue}.
 *
 * @return Returns <b>0</b> if the GPIO pin level value is successfully written; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioWrite(uint16_t gpio, uint16_t val);

/**
 * @brief Sets the input/output direction for a GPIO pin.
 *
 * Generally, you can set the direction to input when external level signals need to be read, and set the
 * direction to output when the level signals need to be sent externally.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param dir Indicates the direction to set. For details, see {@link GpioDirType}.
 *
 * @return Returns <b>0</b> if the GPIO pin direction is successfully set; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioSetDir(uint16_t gpio, uint16_t dir);

/**
 * @brief Obtains the input/output direction of a GPIO pin.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param dir Indicates the pointer to the obtained input/output direction. For details, see {@link GpioDirType}.
 *
 * @return Returns <b>0</b> if the GPIO pin direction is successfully obtained; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioGetDir(uint16_t gpio, uint16_t *dir);

/**
 * @brief Sets the ISR function for a GPIO pin.
 *
 * Before using a GPIO pin as an interrupt, you must call this function to set an ISR function for this GPIO pin,
 * including the ISR parameters and the interrupt trigger mode.
 * After the setting is successful, you also need to call {@link GpioEnableIrq} to enable the interrupt, so that
 * the ISR function can respond correctly.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param mode Indicates the interrupt trigger mode. For details, see {@link OSAL_IRQF_TRIGGER_RISING}.
 * @param func Indicates the ISR function to set, which is specified by {@link GpioIrqFunc}.
 * @param arg Indicates the pointer to the parameters passed to the ISR function.
 *
 * @return Returns <b>0</b> if the ISR function of the GPIO pin is successfully set; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, GpioIrqFunc func, void *arg);

/**
 * @brief Cancels the setting of the ISR function for a GPIO pin.
 *
 * If you do not need the GPIO pin as an interrupt, call this function to cancel the ISR function set via
 * {@link GpioSetIrq}. Since this ISR function is no longer valid, you are advised to use {@link GpioDisableIrq} to
 * disable the GPIO pin interrupt.
 *
 * @param gpio Indicates the GPIO pin number.
 * @param arg Indicates the pointer to the parameters passed to the ISR function.
 *
 * @return Returns <b>0</b> if the ISR function of the GPIO pin is successfully cancelled; returns a negative value
 * otherwise.
 * @since 1.0
 */
int32_t GpioUnsetIrq(uint16_t gpio, void *arg);

/**
 * @brief Enables a GPIO pin interrupt.
 *
 * Before enabling the interrupt, you must call {@link GpioSetIrq} to set the ISR function for the GPIO pin.
 *
 * @param gpio Indicates the GPIO pin number.
 *
 * @return Returns <b>0</b> if the GPIO pin interrupt is successfully enabled; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioEnableIrq(uint16_t gpio);

/**
 * @brief Disables a GPIO pin interrupt.
 *
 * You can call this function when you need to temporarily mask a GPIO pin interrupt or cancel an ISR function.
 *
 * @param gpio Indicates the GPIO pin number.
 *
 * @return Returns <b>0</b> if the GPIO pin interrupt is successfully disabled; returns a negative value otherwise.
 * @since 1.0
 */
int32_t GpioDisableIrq(uint16_t gpio);

/**
 * @brief Gets the GPIO global number.
 *
 * Before using a GPIO pin, you can pass in the GPIO name to get the GPIO global number instead of calculating
 * it manually.
 *
 * @param gpioName Indicates the GPIO pin's name.
 *
 * @return Returns greater than or equal to <b>0</b> if gets the GPIO global number successfully ; returns a
 * negative value otherwise.
 * @since 1.0
 */
int32_t GpioGetByName(const char *gpioName);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* GPIO_IF_H */
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

## Source: `data/rtos/openharmony-liteosm-project/drivers_hdf_core/framework/include/osal/osal_irq.h`

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
 * @file osal_irq.h
 *
 * @brief Declares interrupt request (IRQ) interfaces and common IRQ trigger modes.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef OSAL_IRQ_H
#define OSAL_IRQ_H

#include "hdf_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Enumerates interrupt trigger modes.
 *
 * @since 1.0
 * @version 1.0
 */
typedef enum {
    OSAL_IRQF_TRIGGER_NONE = 0, /**< Edge-triggered is not set */
    OSAL_IRQF_TRIGGER_RISING = 1, /**< Rising edge triggered */
    OSAL_IRQF_TRIGGER_FALLING = 2, /**< Falling edge triggered */
    OSAL_IRQF_TRIGGER_HIGH = 4, /**< High-level triggered */
    OSAL_IRQF_TRIGGER_LOW = 8, /**< Low-level triggered */
} OSAL_IRQ_TRIGGER_MODE;

/**
 * @brief Defines an IRQ type.
 *
 * @since 1.0
 * @version 1.0
 */
typedef uint32_t (*OsalIRQHandle)(uint32_t irqId, void *dev);

/**
 * @brief Registers the function for processing the specified IRQ.
 *
 * @param irqId Indicates the IRQ ID.
 * @param config Indicates the interrupt trigger mode. For details, see {@link OSAL_IRQ_TRIGGER_MODE}.
 * @param handle Indicates the interrupt processing function.
 * @param name Indicates the pointer to the device name for registering an IRQ.
 * @param dev Indicates the pointer to the parameter passed to the interrupt processing function.
 *
 * @return Returns a value listed below: \n
 * HDF_STATUS | Description
 * ----------------------| -----------------------
 * HDF_SUCCESS | The operation is successful.
 * HDF_FAILURE | Failed to invoke the system function to register the IRQ.
 * HDF_ERR_INVALID_PARAM | Invalid parameter.
 *
 * @since 1.0
 * @version 1.0
 */
int32_t OsalRegisterIrq(uint32_t irqId, uint32_t config, OsalIRQHandle handle, const char *name, void *dev);

/**
 * @brief Unregisters the interrupt processing function so that the system will no longer process the specified IRQ.
 *
 * @param irqId Indicates the IRQ ID.
 * @param dev Indicates the pointer to the parameter passed to the interrupt processing function
 *        in {@link OsalRegisterIrq}.
 *
 * @return Returns a value listed below: \n
 * HDF_STATUS | Description
 * ----------------------| -----------------------
 * HDF_SUCCESS | The operation is successful.
 * HDF_FAILURE | Failed to invoke the system function to unregister the IRQ.
 * HDF_ERR_INVALID_PARAM | Invalid parameter.
 *
 * @since 1.0
 * @version 1.0
 */
int32_t OsalUnregisterIrq(uint32_t irqId, void *dev);

/**
 * @brief Enables the processing of the specified IRQ.
 *
 * @param irqId Indicates the IRQ ID.
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
int32_t OsalEnableIrq(uint32_t irqId);

/**
 * @brief Disables the IRQ function of a device.
 *
 * @param irqId Indicates the IRQ ID.
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
int32_t OsalDisableIrq(uint32_t irqId);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OSAL_IRQ_H */
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
