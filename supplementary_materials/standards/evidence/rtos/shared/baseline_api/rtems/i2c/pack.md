# Raw RTOS/Bus Pack: rtems / i2c

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/rtems/cpukit/include/rtems.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSAPIClassic
 *
 * @brief This header file defines the RTEMS Classic API.
 */

/*
 * Copyright (C) 2020 embedded brains GmbH & Co. KG
 * Copyright (C) 1988, 2008 On-Line Applications Research Corporation (OAR)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file is part of the RTEMS quality process and was automatically
 * generated.  If you find something that needs to be fixed or
 * worded better please post a report or patch to an RTEMS mailing list
 * or raise a bug report:
 *
 * https://www.rtems.org/bugs.html
 *
 * For information on updating and regenerating please refer to the How-To
 * section in the Software Requirements Engineering chapter of the
 * RTEMS Software Engineering manual.  The manual is provided as a part of
 * a release.  For development sources please refer to the online
 * documentation at:
 *
 * https://docs.rtems.org
 */

/* Generated from spec:/rtems/if/header */

#ifndef _RTEMS_H
#define _RTEMS_H

#include <rtems/config.h>
#include <rtems/extension.h>
#include <rtems/fatal.h>
#include <rtems/init.h>
#include <rtems/io.h>
#include <rtems/rtems/barrier.h>
#include <rtems/rtems/cache.h>
#include <rtems/rtems/clock.h>
#include <rtems/rtems/dpmem.h>
#include <rtems/rtems/event.h>
#include <rtems/rtems/intr.h>
#include <rtems/rtems/message.h>
#include <rtems/rtems/object.h>
#include <rtems/rtems/options.h>
#include <rtems/rtems/part.h>
#include <rtems/rtems/ratemon.h>
#include <rtems/rtems/region.h>
#include <rtems/rtems/scheduler.h>
#include <rtems/rtems/sem.h>
#include <rtems/rtems/signal.h>
#include <rtems/rtems/status.h>
#include <rtems/rtems/support.h>
#include <rtems/rtems/tasks.h>
#include <rtems/rtems/timer.h>
#include <rtems/rtems/types.h>

#if defined( RTEMS_MULTIPROCESSING )
  #include <rtems/rtems/mp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Generated from spec:/rtems/if/group */

/**
 * @defgroup RTEMSAPIClassic Classic
 *
 * @ingroup RTEMSAPI
 *
 * @brief This group contains the Classic API managers.
 */

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_H */
```

## Source: `data/rtos/rtems/cpukit/include/dev/i2c/i2c.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief Inter-Integrated Circuit (I2C) Driver API
 *
 * @ingroup I2C
 */

/*
 * Copyright (C) 2014, 2017 embedded brains GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_I2C_I2C_H
#define _DEV_I2C_I2C_H

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <rtems.h>
#include <rtems/seterr.h>
#include <rtems/thread.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct i2c_msg i2c_msg;

typedef struct i2c_bus i2c_bus;

typedef struct i2c_dev i2c_dev;

typedef struct i2c_rdwr_ioctl_data i2c_rdwr_ioctl_data;

/**
 * @defgroup I2C Inter-Integrated Circuit (I2C) Driver
 *
 * @ingroup RTEMSDeviceDrivers
 *
 * @brief Inter-Integrated Circuit (I2C) bus and device driver support.
 *
 * @{
 */

/**
 * @defgroup I2CBus I2C Bus Driver
 *
 * @ingroup I2C
 *
 * @{
 */

/**
 * @name I2C IO Control Commands
 *
 * @{
 */

/**
 * @brief Obtains the bus.
 *
 * This command has no argument.
 */
#define I2C_BUS_OBTAIN 0x800

/**
 * @brief Releases the bus.
 *
 * This command has no argument.
 */
#define I2C_BUS_RELEASE 0x801

/**
 * @brief Gets the bus control.
 *
 * The argument type is a pointer to i2c_bus pointer.
 */
#define I2C_BUS_GET_CONTROL 0x802

/**
 * @brief Sets the bus clock in Hz.
 *
 * The argument type is unsigned long.
 */
#define I2C_BUS_SET_CLOCK 0x803

/** @} */

/**
 * @brief Default I2C bus clock in Hz.
 */
#define I2C_BUS_CLOCK_DEFAULT 100000

/**
 * @brief I2C bus control.
 */
struct i2c_bus {
  /**
   * @brief Transfers I2C messages.
   *
   * @param[in] bus The bus control.
   * @param[in] msgs The messages to transfer.
   * @param[in] msg_count The count of messages to transfer.  It must be
   * positive.
   *
   * @retval 0 Successful operation.
   * @retval negative Negative error number in case of an error.
   */
  int ( *transfer )( i2c_bus *bus, i2c_msg *msgs, uint32_t msg_count );

  /**
   * @brief Sets the bus clock.
   *
   * @param[in] bus The bus control.
   * @param[in] clock The desired bus clock in Hz.
   *
   * @retval 0 Successful operation.
   * @retval negative Negative error number in case of an error.
   */
  int ( *set_clock )( i2c_bus *bus, unsigned long clock );

  /**
   * @brief Destroys the bus.
   *
   * @param[in] bus The bus control.
   */
  void ( *destroy )( i2c_bus *bus );

  /**
   * @brief Mutex to protect the bus access.
   */
  rtems_recursive_mutex mutex;

  /**
   * @brief Default slave device address.
   */
  uint16_t default_address;

  /**
   * @brief Use 10-bit addresses.
   */
  bool ten_bit_address;

  /**
   * @brief Use SMBus PEC.
   */
  bool use_pec;

  /**
   * @brief Transfer retry count.
   */
  unsigned long retries;

  /**
   * @brief Transaction timeout in ticks.
   */
  rtems_interval timeout;

  /**
   * @brief Controller functionality.
   */
  unsigned long functionality;
};

/**
 * @brief Initializes a bus control.
 *
 * After a sucessful initialization the bus control must be destroyed via
 * i2c_bus_destroy().  A registered bus control will be automatically destroyed
 * in case the device file is unlinked.  Make sure to call i2c_bus_destroy() in
 * a custom destruction handler.
 *
 * @param[in] bus The bus control.
 *
 * @retval 0 Successful operation.
 * @retval -1 An error occurred.  The errno is set to indicate the error.
 *
 * @see i2c_bus_register()
 */
int i2c_bus_init( i2c_bus *bus );

/**
 * @brief Allocates a bus control from the heap and initializes it.
 *
 * After a sucessful allocation and initialization the bus control must be
 * destroyed via i2c_bus_destroy_and_free().  A registered bus control will be
 * automatically destroyed in case the device file is unlinked.  Make sure to
 * call i2c_bus_destroy_and_free() in a custom destruction handler.
 *
 * @param[in] size The size of the bus control.  This enables the addition of
 * bus controller specific data to the base bus control.  The bus control is
 * zero initialized.
 *
 * @retval non-NULL The new bus control.
 * @retval NULL An error occurred.  The errno is set to indicate the error.
 *
 * @see i2c_bus_register()
 */
i2c_bus *i2c_bus_alloc_and_init( size_t size );

/**
 * @brief Destroys a bus control.
 *
 * @param[in] bus The bus control.
 */
void i2c_bus_destroy( i2c_bus *bus );

/**
 * @brief Destroys a bus control and frees its memory.
 *
 * @param[in] bus The bus control.
 */
void i2c_bus_destroy_and_free( i2c_bus *bus );

/**
 * @brief Registers a bus control.
 *
 * This function claims ownership of the bus control regardless if the
 * registration is successful or not.
 *
 * @param[in] bus The bus control.
 * @param[in] bus_path The path to the bus device file.
 *
 * @retval 0 Successful operation.
 * @retval -1 An error occurred.  The errno is set to indicate the error.
 */
int i2c_bus_register( i2c_bus *bus, const char *bus_path );

/**
 * @brief Try to obtain the bus.
 *
 * @param[in] bus The bus control.
 *
 * @retval 0 Successful operation.
 * @retval EBUSY if mutex is already locked.
 */
int i2c_bus_try_obtain( i2c_bus *bus );

/**
 * @brief Obtains the bus.
 *
 * @param[in] bus The bus control.
 */
void i2c_bus_obtain( i2c_bus *bus );

/**
 * @brief Releases the bus.
 *
 * @param[in] bus The bus control.
 */
void i2c_bus_release( i2c_bus *bus );

/**
 * @brief Transfers I2C messages.
 *
 * The bus is obtained before the transfer and released afterwards. This is the
 * same like calling @ref i2c_bus_do_transfer with flags set to 0.
 *
 * @param[in] bus The bus control.
 * @param[in] msgs The messages to transfer.
 * @param[in] msg_count The count of messages to transfer.  It must be
 * positive.
 *
 * @retval 0 Successful operation.
 * @retval negative Negative error number in case of an error.
 */
int i2c_bus_transfer( i2c_bus *bus, i2c_msg *msgs, uint32_t msg_count );

/**
 * @brief Transfers I2C messages with optional flags.
 *
 * The bus is obtained before the transfer and released afterwards. If the flag
 * I2C_BUS_NOBLOCK is set and the bus is already obtained, nothing will be
 * transfered and the function returns with an -EAGAIN.
 *
 * @param[in] bus The bus control.
 * @param[in] msgs The messages to transfer.
 * @param[in] msg_count The count of messages to transfer.  It must be
 * positive.
 * @param[in] flags Options for the whole transfer.
 *
 * @retval 0 Successful operation.
 * @retval -EAGAIN if @ref I2C_BUS_NOBLOCK is set and the bus is already
 * obtained.
 * @retval negative Negative error number in case of an error.
 */
int i2c_bus_do_transfer(
  i2c_bus *bus,
  i2c_msg *msgs,
  uint32_t msg_count,
  uint32_t flags
);

/**
 * @brief I2C bus transfer flag to indicate that the task should not block if
 * the bus is busy on a new transfer.
 */
#define I2C_BUS_NOBLOCK ( 1u << 0 )

/** @} */

/**
 * @defgroup I2CDevice I2C Device Driver
 *
 * @ingroup I2C
 *
 * @{
 */

/**
 * @brief Base number for device IO control commands.
 */
#define I2C_DEV_IO_CONTROL 0x900

/**
 * @brief I2C slave device control.
 */
struct i2c_dev {
  /**
   * @brief Reads from the device.
   *
   * @retval non-negative Bytes transferred from device.
   * @retval negative Negative error number in case of an error.
   */
  ssize_t ( *read )( i2c_dev *dev, void *buf, size_t n, off_t offset );

  /**
   * @brief Writes to the device.
   *
   * @retval non-negative Bytes transferred to device.
   * @retval negative Negative error number in case of an error.
   */
  ssize_t ( *write )( i2c_dev *dev, const void *buf, size_t n, off_t offset );

  /**
   * @brief Device IO control.
   *
   * @retval 0 Successful operation.
   * @retval negative Negative error number in case of an error.
   */
  int ( *ioctl )( i2c_dev *dev, ioctl_command_t command, void *arg );

  /**
   * @brief Gets the file size.
   */
  off_t ( *get_size )( i2c_dev *dev );

  /**
   * @brief Gets the file block size.
   */
  blksize_t ( *get_block_size )( i2c_dev *dev );

  /**
   * @brief Destroys the device.
   */
  void ( *destroy )( i2c_dev *dev );

  /**
   * @brief The bus control.
   */
  i2c_bus *bus;

  /**
   * @brief The device address.
   */
  uint16_t address;

  /**
   * @brief File descriptor of the bus.
   *
   * This prevents destruction of the bus since we hold a reference to it with
   * this.
   */
  int bus_fd;
};

/**
 * @brief Initializes a device control.
 *
 * After a sucessful initialization the device control must be destroyed via
 * i2c_dev_destroy().  A registered device control will be automatically
 * destroyed in case the device file is unlinked.  Make sure to call
 * i2c_dev_destroy_and_free() in a custom destruction handler.
 *
 * @param[in] device The device control.
 * @param[in] bus_path The path to the bus device file.
 * @param[in] address The address of the device.
 *
 * @retval 0 Successful operation.
 * @retval -1 An error occurred.  The errno is set to indicate the error.
 *
 * @see i2c_dev_register()
 */
int i2c_dev_init( i2c_dev *dev, const char *bus_path, uint16_t address );

/**
 * @brief Allocates a device control from the heap and initializes it.
 *
 * After a sucessful allocation and initialization the device control must be
 * destroyed via i2c_dev_destroy_and_free().  A registered device control will
 * be automatically destroyed in case the device file is unlinked.  Make sure
 * to call i2c_dev_destroy_and_free() in a custom destruction handler.
 *
 * @param[in] size The size of the device control.  This enables the addition
 * of device specific data to the base device control.  The device control is
 * zero initialized.
 * @param[in] bus_path The path to the bus device file.
 * @param[in] address The address of the device.
 *
 * @retval non-NULL The new device control.
 * @retval NULL An error occurred.  The errno is set to indicate the error.
 *
 * @see i2c_dev_register()
 */
i2c_dev *i2c_dev_alloc_and_init(
  size_t      size,
  const char *bus_path,
  uint16_t    address
);

/**
 * @brief Destroys a device control.
 *
 * @param[in] dev The device control.
 */
void i2c_dev_destroy( i2c_dev *dev );

/**
 * @brief Destroys a device control and frees its memory.
 *
 * @param[in] dev The device control.
 */
void i2c_dev_destroy_and_free( i2c_dev *dev );

/**
 * @brief Registers a device control.
 *
 * This function claims ownership of the device control regardless if the
 * registration is successful or not.
 *
 * @param[in] dev The dev control.
 * @param[in] dev_path The path to the device file of the device.
 *
 * @retval 0 Successful operation.
 * @retval -1 An error occurred.  The errno is set to indicate the error.
 */
int i2c_dev_register( i2c_dev *dev, const char *dev_path );

/** @} */ /* end of i2c device driver */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DEV_I2C_I2C_H */
```

## Source: `data/rtos/rtems/cpukit/include/linux/i2c.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief RTEMS Port of Linux I2C API
 *
 * @ingroup I2CLinux
 */

/*
 * Copyright (c) 2014 embedded brains GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UAPI_LINUX_I2C_H
#define _UAPI_LINUX_I2C_H

#include <stdint.h>

/**
 * @defgroup I2CLinux Linux I2C User-Space API
 *
 * @ingroup I2C
 *
 * @brief RTEMS port of Linux I2C user-space API.
 *
 * Additional documentation is available through the Linux sources, see:
 *
 * - /usr/src/linux/include/uapi/linux/i2c.h,
 * - /usr/src/linux/include/uapi/linux/i2c-dev.h
 * - https://www.kernel.org/doc/Documentation/i2c/i2c-protocol
 * - https://www.kernel.org/doc/Documentation/i2c/dev-interface
 *
 * @{
 */

/**
 * @name I2C Message Flags
 *
 * @{
 */

/**
 * @brief I2C message flag to indicate a 10-bit address.
 *
 * The controller must support this as indicated by the I2C_FUNC_10BIT_ADDR
 * functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_TEN 0x0010

/**
 * @brief I2C message flag to indicate a read transfer (from slave to master).
 *
 * @see i2c_msg.
 */
#define I2C_M_RD 0x0001

/**
 * @brief I2C message flag to signal a stop condition even if this is not the
 * last message.
 *
 * The controller must support this as indicated by the
 * @ref I2C_FUNC_PROTOCOL_MANGLING functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_STOP 0x8000

/**
 * @brief I2C message flag to omit start condition and slave address.
 *
 * The controller must support this as indicated by the
 * @ref I2C_FUNC_NOSTART functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_NOSTART 0x4000

/**
 * @brief I2C message flag to reverse the direction flag.
 *
 * The controller must support this as indicated by the
 * @ref I2C_FUNC_PROTOCOL_MANGLING functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_REV_DIR_ADDR 0x2000

/**
 * @brief I2C message flag to ignore a non-acknowledge.
 *
 * The controller must support this as indicated by the
 * @ref I2C_FUNC_PROTOCOL_MANGLING functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_IGNORE_NAK 0x1000

/**
 * @brief I2C message flag to omit a master acknowledge/non-acknowledge in a
 * read transfer.
 *
 * The controller must support this as indicated by the
 * @ref I2C_FUNC_PROTOCOL_MANGLING functionality.
 *
 * @see i2c_msg.
 */
#define I2C_M_NO_RD_ACK 0x0800

/**
 * @brief I2C message flag to indicate that the message data length is the
 * first received byte.
 *
 * The message data buffer must be large enough to store up to 32 bytes, the
 * initial length byte and the SMBus PEC (if used).  Initialize the message
 * length to one.  The message length is incremented by the count of received
 * data bytes.
 *
 * @see i2c_msg.
 */
#define I2C_M_RECV_LEN 0x0400

/** @} */

/**
 * @brief I2C transfer message.
 */
struct i2c_msg {
  /**
   * @brief The slave address.
   *
   * In case the @ref I2C_M_TEN flag is set, then this is a 10-bit address,
   * otherwise it is a 7-bit address.
   */
  uint16_t addr;

  /**
   * @brief The message flags.
   *
   * Valid flags are
   * - @ref I2C_M_TEN,
   * - @ref I2C_M_RD,
   * - @ref I2C_M_STOP,
   * - @ref I2C_M_NOSTART,
   * - @ref I2C_M_REV_DIR_ADDR,
   * - @ref I2C_M_IGNORE_NAK,
   * - @ref I2C_M_NO_RD_ACK, and
   * - @ref I2C_M_RECV_LEN.
   */
  uint16_t flags;

  /**
   * @brief The message data length in bytes.
   */
  uint16_t len;

  /**
   * @brief Pointer to the message data.
   */
  uint8_t *buf;
};

/**
 * @name I2C Controller Functionality
 *
 * @{
 */

#define I2C_FUNC_I2C                    0x00000001
#define I2C_FUNC_10BIT_ADDR             0x00000002
#define I2C_FUNC_PROTOCOL_MANGLING      0x00000004
#define I2C_FUNC_SMBUS_PEC              0x00000008
#define I2C_FUNC_NOSTART                0x00000010
#define I2C_FUNC_SMBUS_BLOCK_PROC_CALL  0x00008000
#define I2C_FUNC_SMBUS_QUICK            0x00010000
#define I2C_FUNC_SMBUS_READ_BYTE        0x00020000
#define I2C_FUNC_SMBUS_WRITE_BYTE       0x00040000
#define I2C_FUNC_SMBUS_READ_BYTE_DATA   0x00080000
#define I2C_FUNC_SMBUS_WRITE_BYTE_DATA  0x00100000
#define I2C_FUNC_SMBUS_READ_WORD_DATA   0x00200000
#define I2C_FUNC_SMBUS_WRITE_WORD_DATA  0x00400000
#define I2C_FUNC_SMBUS_PROC_CALL        0x00800000
#define I2C_FUNC_SMBUS_READ_BLOCK_DATA  0x01000000
#define I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
#define I2C_FUNC_SMBUS_READ_I2C_BLOCK   0x04000000
#define I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  0x08000000

#define I2C_FUNC_SMBUS_BYTE \
  ( I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE )

#define I2C_FUNC_SMBUS_BYTE_DATA \
  ( I2C_FUNC_SMBUS_READ_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE_DATA )

#define I2C_FUNC_SMBUS_WORD_DATA \
  ( I2C_FUNC_SMBUS_READ_WORD_DATA | I2C_FUNC_SMBUS_WRITE_WORD_DATA )

#define I2C_FUNC_SMBUS_BLOCK_DATA \
  ( I2C_FUNC_SMBUS_READ_BLOCK_DATA | I2C_FUNC_SMBUS_WRITE_BLOCK_DATA )

#define I2C_FUNC_SMBUS_I2C_BLOCK \
  ( I2C_FUNC_SMBUS_READ_I2C_BLOCK | I2C_FUNC_SMBUS_WRITE_I2C_BLOCK )

#define I2C_FUNC_SMBUS_EMUL                                                 \
  ( I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA | \
    I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_PROC_CALL |                   \
    I2C_FUNC_SMBUS_WRITE_BLOCK_DATA | I2C_FUNC_SMBUS_I2C_BLOCK |            \
    I2C_FUNC_SMBUS_PEC )

/** @} */

/**
 * @brief Maximum SMBus data block count.
 */
#define I2C_SMBUS_BLOCK_MAX 32

/**
 * @brief SMBus data.
 */
union i2c_smbus_data {
  uint8_t  byte;
  uint16_t word;
  uint8_t  block[ I2C_SMBUS_BLOCK_MAX + 2 ];
};

/**
 * @name SMBus Transfer Read and Write Markers
 *
 * @{
 */

#define I2C_SMBUS_READ 1

#define I2C_SMBUS_WRITE 0

/** @} */

/**
 * @name SMBus Transaction Types
 *
 * @{
 */

#define I2C_SMBUS_QUICK 0

#define I2C_SMBUS_BYTE 1

#define I2C_SMBUS_BYTE_DATA 2

#define I2C_SMBUS_WORD_DATA 3

#define I2C_SMBUS_PROC_CALL 4

#define I2C_SMBUS_BLOCK_DATA 5

#define I2C_SMBUS_I2C_BLOCK_BROKEN 6

#define I2C_SMBUS_BLOCK_PROC_CALL 7

#define I2C_SMBUS_I2C_BLOCK_DATA 8

/** @} */

/** @} */

#endif /* _UAPI_LINUX_I2C_H */
```

## Source: `data/rtos/rtems/cpukit/include/linux/i2c-dev.h`

```c
/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief RTEMS Port of Linux I2C Device API
 *
 * @ingroup I2CLinux
 */

/*
 * Copyright (c) 2014 embedded brains GmbH & Co. KG
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UAPI_LINUX_I2C_DEV_H
#define _UAPI_LINUX_I2C_DEV_H

#include <stdint.h>

/**
 * @addtogroup I2CLinux
 *
 * @{
 */

/**
 * @name I2C IO Control Commands
 *
 * @{
 */

/**
 * @brief Sets the count of transfer retries in case a slave
 * device does not acknowledge a transaction.
 *
 * The argument type is unsigned long.
 */
#define I2C_RETRIES 0x701

/**
 * @brief Sets the transfer timeout in 10ms units.
 *
 * The argument type is unsigned long.
 */
#define I2C_TIMEOUT 0x702

/**
 * @brief Sets the slave address.
 *
 * It is an error to set a slave address already used by another slave device.
 *
 * The argument type is unsigned long.
 */
#define I2C_SLAVE 0x703

/**
 * @brief Forces setting the slave address.
 *
 * The argument type is unsigned long.
 */
#define I2C_SLAVE_FORCE 0x706

/**
 * @brief Enables 10-bit addresses if argument is non-zero, otherwise
 * disables 10-bit addresses.
 *
 * The argument type is unsigned long.
 */
#define I2C_TENBIT 0x704

/**
 * @brief Gets the I2C controller functionality information.
 *
 * The argument type is a pointer to an unsigned long.
 */
#define I2C_FUNCS 0x705

/**
 * @brief Performs a combined read/write transfer.
 *
 * Only one stop condition is signalled.
 *
 * The argument type is a pointer to struct i2c_rdwr_ioctl_data.
 */
#define I2C_RDWR 0x707

/**
 * @brief Enables System Management Bus (SMBus) Packet Error Checking (PEC)
 * if argument is non-zero, otherwise disables PEC.
 *
 * The argument type is unsigned long.
 */
#define I2C_PEC 0x708

/**
 * @brief Performs an SMBus transfer.
 *
 * The argument type is a pointer to struct i2c_smbus_ioctl_data.
 */
#define I2C_SMBUS 0x720

/** @} */

/**
 * @brief Argument type for I2C_SMBUS IO control call.
 */
struct i2c_smbus_ioctl_data {
  uint8_t               read_write;
  uint8_t               command;
  uint32_t              size;
  union i2c_smbus_data *data;
};

/**
 * @brief Argument type for I2C_RDWR IO control call.
 */
struct i2c_rdwr_ioctl_data {
  struct i2c_msg *msgs;
  uint32_t        nmsgs;
};

/**
 * @brief Maximum count of messages for one IO control call.
 */
#define I2C_RDRW_IOCTL_MAX_MSGS 42

/** @} */

#endif /* _UAPI_LINUX_I2C_DEV_H */
```
