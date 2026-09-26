# Raw RTOS/Bus Pack: zephyr / i2c

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/zephyr/include/zephyr/drivers/i2c.h`

```c
/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup i2c_interface
 * @brief Main header file for I2C (Inter-Integrated Circuit) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_H_

/**
 * @brief Interfaces for Inter-Integrated Circuit (I2C) controllers.
 * @defgroup i2c_interface I2C
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following #defines are used to configure the I2C controller.
 */

/** I2C Standard Speed: 100 kHz */
#define I2C_SPEED_STANDARD		(0x1U)

/** I2C Fast Speed: 400 kHz */
#define I2C_SPEED_FAST			(0x2U)

/** I2C Fast Plus Speed: 1 MHz */
#define I2C_SPEED_FAST_PLUS		(0x3U)

/** I2C High Speed: 3.4 MHz */
#define I2C_SPEED_HIGH			(0x4U)

/** I2C Ultra Fast Speed: 5 MHz */
#define I2C_SPEED_ULTRA			(0x5U)

/** Device Tree specified speed */
#define I2C_SPEED_DT			(0x7U)

#define I2C_SPEED_SHIFT			(1U)
#define I2C_SPEED_SET(speed)		(((speed) << I2C_SPEED_SHIFT) \
						& I2C_SPEED_MASK)
#define I2C_SPEED_MASK			(0x7U << I2C_SPEED_SHIFT) /* 3 bits */
#define I2C_SPEED_GET(cfg) 		(((cfg) & I2C_SPEED_MASK) \
						>> I2C_SPEED_SHIFT)

/** Use 10-bit addressing. DEPRECATED - Use I2C_MSG_ADDR_10_BITS instead. */
#define I2C_ADDR_10_BITS		BIT(0)

/** Peripheral to act as Controller. */
#define I2C_MODE_CONTROLLER		BIT(4)

/**
 * @brief Complete I2C DT information
 *
 * @param bus is the I2C bus
 * @param addr is the target address
 */
struct i2c_dt_spec {
	const struct device *bus;
	uint16_t addr;
};

/**
 * @brief Structure initializer for i2c_dt_spec from devicetree (on I3C bus)
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i2c_dt_spec</tt> by reading the relevant bus and address data from
 * the devicetree.
 *
 * @param node_id Devicetree node identifier for the I2C device whose
 *                struct i2c_dt_spec to create an initializer for
 */
#define I2C_DT_SPEC_GET_ON_I3C(node_id)					\
	.bus = DEVICE_DT_GET(DT_BUS(node_id)),				\
	.addr = DT_PROP_BY_IDX(node_id, reg, 0)

/**
 * @brief Structure initializer for i2c_dt_spec from devicetree (on I2C bus)
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i2c_dt_spec</tt> by reading the relevant bus and address data from
 * the devicetree.
 *
 * @param node_id Devicetree node identifier for the I2C device whose
 *                struct i2c_dt_spec to create an initializer for
 */
#define I2C_DT_SPEC_GET_ON_I2C(node_id)					\
	.bus = DEVICE_DT_GET(DT_BUS(node_id)),				\
	.addr = DT_REG_ADDR(node_id)

/**
 * @brief Structure initializer for i2c_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * i2c_dt_spec</tt> by reading the relevant bus and address data from
 * the devicetree.
 *
 * @param node_id Devicetree node identifier for the I2C device whose
 *                struct i2c_dt_spec to create an initializer for
 */
#define I2C_DT_SPEC_GET(node_id)					\
	{								\
		COND_CODE_1(DT_ON_BUS(node_id, i3c),			\
			    (I2C_DT_SPEC_GET_ON_I3C(node_id)),		\
			    (I2C_DT_SPEC_GET_ON_I2C(node_id)))		\
	}

/**
 * @brief Structure initializer for i2c_dt_spec from devicetree instance
 *
 * This is equivalent to
 * <tt>I2C_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define I2C_DT_SPEC_INST_GET(inst) \
	I2C_DT_SPEC_GET(DT_DRV_INST(inst))


/*
 * I2C_MSG_* are I2C Message flags.
 */

/** Write message to I2C bus. */
#define I2C_MSG_WRITE			(0U << 0U)

/** Read message from I2C bus. */
#define I2C_MSG_READ			BIT(0)

/** @cond INTERNAL_HIDDEN */
#define I2C_MSG_RW_MASK			BIT(0)
/** @endcond  */

/** Send STOP after this message. */
#define I2C_MSG_STOP			BIT(1)

/** RESTART I2C transaction for this message.
 *
 * @note Not all I2C drivers have or require explicit support for this
 * feature. Some drivers require this be present on a read message
 * that follows a write, or vice-versa.  Some drivers will merge
 * adjacent fragments into a single transaction using this flag; some
 * will not. */
#define I2C_MSG_RESTART			BIT(2)

/** Use 10-bit addressing for this message.
 *
 * @note Not all SoC I2C implementations support this feature. */
#define I2C_MSG_ADDR_10_BITS		BIT(3)

/**
 * @brief One I2C Message.
 *
 * This defines one I2C message to transact on the I2C bus.
 *
 * @note Some of the configurations supported by this API may not be
 * supported by specific SoC I2C hardware implementations, in
 * particular features related to bus transactions intended to read or
 * write data from different buffers within a single transaction.
 * Invocations of i2c_transfer() may not indicate an error when an
 * unsupported configuration is encountered.  In some cases drivers
 * will generate separate transactions for each message fragment, with
 * or without presence of @ref I2C_MSG_RESTART in #flags.
 */
struct i2c_msg {
	/** Data buffer in bytes */
	uint8_t		*buf;

	/** Length of buffer in bytes */
	uint32_t	len;

	/** Flags for this message */
	uint8_t		flags;
};

/**
 * @brief I2C callback for asynchronous transfer requests
 *
 * @param dev I2C device which is notifying of transfer completion or error
 * @param result Result code of the transfer request. 0 is success, -errno for failure.
 * @param data Transfer requester supplied data which is passed along to the callback.
 */
typedef void (*i2c_callback_t)(const struct device *dev, int result, void *data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
struct i2c_target_config;

typedef int (*i2c_api_configure_t)(const struct device *dev,
				   uint32_t dev_config);
typedef int (*i2c_api_get_config_t)(const struct device *dev,
				    uint32_t *dev_config);
typedef int (*i2c_api_full_io_t)(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr);
typedef int (*i2c_api_target_register_t)(const struct device *dev,
					struct i2c_target_config *cfg);
typedef int (*i2c_api_target_unregister_t)(const struct device *dev,
					  struct i2c_target_config *cfg);

#ifdef CONFIG_I2C_CALLBACK
typedef int (*i2c_api_transfer_cb_t)(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr,
				 i2c_callback_t cb,
				 void *userdata);
#endif /* CONFIG_I2C_CALLBACK */

#ifdef CONFIG_I2C_RTIO
typedef void (*i2c_api_iodev_submit)(const struct device *dev,
				     struct rtio_iodev_sqe *iodev_sqe);
#endif /* CONFIG_I2C_RTIO */

typedef int (*i2c_api_recover_bus_t)(const struct device *dev);

__subsystem struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_get_config_t get_config;
	i2c_api_full_io_t transfer;
	i2c_api_target_register_t target_register;
	i2c_api_target_unregister_t target_unregister;
#ifdef CONFIG_I2C_CALLBACK
	i2c_api_transfer_cb_t transfer_cb;
#endif
#ifdef CONFIG_I2C_RTIO
	i2c_api_iodev_submit iodev_submit;
#endif
	i2c_api_recover_bus_t recover_bus;
};

typedef int (*i2c_target_api_register_t)(const struct device *dev);
typedef int (*i2c_target_api_unregister_t)(const struct device *dev);

__subsystem struct i2c_target_driver_api {
	i2c_target_api_register_t driver_register;
	i2c_target_api_unregister_t driver_unregister;
};

/**
 * @endcond
 */

/** Target device responds to 10-bit addressing. */
#define I2C_TARGET_FLAGS_ADDR_10_BITS	BIT(0)

/** @brief Function called when a write to the device is initiated.
 *
 * This function is invoked by the controller when the bus completes a
 * start condition for a write operation to the address associated
 * with a particular device.
 *
 * A success return shall cause the controller to ACK the next byte
 * received.  An error return shall cause the controller to NACK the
 * next byte received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @return 0 if the write is accepted, or a negative error code.
 */
typedef int (*i2c_target_write_requested_cb_t)(
		struct i2c_target_config *config);

/** @brief Function called when a write to the device is continued.
 *
 * This function is invoked by the controller when it completes
 * reception of a byte of data in an ongoing write operation to the
 * device.
 *
 * A success return shall cause the controller to ACK the next byte
 * received.  An error return shall cause the controller to NACK the
 * next byte received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val the byte received by the controller.
 *
 * @return 0 if more data can be accepted, or a negative error
 * code.
 */
typedef int (*i2c_target_write_received_cb_t)(
		struct i2c_target_config *config, uint8_t val);

/** @brief Function called when a read from the device is initiated.
 *
 * This function is invoked by the controller when the bus completes a
 * start condition for a read operation from the address associated
 * with a particular device.
 *
 * The value returned in @p *val will be transmitted.  A success
 * return shall cause the controller to react to additional read
 * operations.  An error return shall cause the controller to ignore
 * bus operations until a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val pointer to storage for the first byte of data to return
 * for the read request.
 *
 * @return 0 if more data can be requested, or a negative error code.
 */
typedef int (*i2c_target_read_requested_cb_t)(
		struct i2c_target_config *config, uint8_t *val);

/** @brief Function called when a read from the device is continued.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data for a read operation from the address
 * associated with the device device.
 *
 * The value returned in @p *val will be transmitted.  A success
 * return shall cause the controller to react to additional read
 * operations.  An error return shall cause the controller to ignore
 * bus operations until a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param val pointer to storage for the next byte of data to return
 * for the read request.
 *
 * @return 0 if data has been provided, or a negative error code.
 */
typedef int (*i2c_target_read_processed_cb_t)(
		struct i2c_target_config *config, uint8_t *val);

#if defined(CONFIG_I2C_TARGET_BUFFER_MODE) || defined(__DOXYGEN__)
/** @brief Function called when a write to the device is completed.
 *
 * This function is invoked by the controller when it completes
 * reception of data from the source buffer to the destination
 * buffer in an ongoing write operation to the device.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param ptr pointer to the buffer that contains the data to be transferred.
 *
 * @param len the length of the data to be transferred.
 */
typedef void (*i2c_target_buf_write_received_cb_t)(
		struct i2c_target_config *config, uint8_t *ptr, uint32_t len);

/** @brief Function called when a read from the device is initiated.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data by buffer for a read operation from the address
 * associated with the device.
 *
 * The value returned in @p **ptr and @p *len will be transmitted. A success
 * return shall cause the controller to react to additional read operations.
 * An error return shall cause the controller to ignore bus operations until
 * a new start condition is received.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @param ptr pointer to storage for the address of data buffer to return
 * for the read request.
 *
 * @param len pointer to storage for the length of the data to be transferred
 * for the read request.
 *
 * @return 0 if data has been provided, or a negative error code.
 */
typedef int (*i2c_target_buf_read_requested_cb_t)(
		struct i2c_target_config *config, uint8_t **ptr, uint32_t *len);
#endif

/** @brief Function called when a stop condition is observed after a
 * start condition addressed to a particular device.
 *
 * This function is invoked by the controller when the bus is ready to
 * provide additional data for a read operation from the address
 * associated with the device device.  After the function returns the
 * controller shall enter a state where it is ready to react to new
 * start conditions.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 *
 * @return Ignored.
 */
typedef int (*i2c_target_stop_cb_t)(struct i2c_target_config *config);

/**
 * @brief I2C error reasons.
 *
 * Values that correspond to events or errors responsible for stopping
 * an I2C transfer.
 */
enum i2c_error_reason {
	I2C_ERROR_TIMEOUT = 0,	/* Timeout error         */
	I2C_ERROR_ARBITRATION,	/* Bus arbitration size  */
	I2C_ERROR_SIZE,		/* Bad frame size        */
	I2C_ERROR_DMA,		/* DMA transfer error    */
	I2C_ERROR_GENERIC,	/* Any other bus error   */
};

/** @brief Function called when an error is detected on the I2C bus
 * while acting as a target.
 *
 * This function is invoked by the controller when a bus error,
 * arbitration lost, or other critical error is detected during
 * a transaction addressed to this device.
 *
 * @param config the configuration structure associated with the
 * device to which the operation is addressed.
 * @param error_code an integer code identifying the error type.
 */
typedef void (*i2c_target_error_cb_t)(struct i2c_target_config *config,
				      enum i2c_error_reason error_code);

/** @brief Structure providing callbacks to be implemented for devices
 * that supports the I2C target API.
 *
 * This structure may be shared by multiple devices that implement the
 * same API at different addresses on the bus.
 */
struct i2c_target_callbacks {
	/** @copybrief i2c_target_write_requested_cb_t */
	i2c_target_write_requested_cb_t write_requested;
	/** @copybrief i2c_target_read_requested_cb_t */
	i2c_target_read_requested_cb_t read_requested;
	/** @copybrief i2c_target_write_received_cb_t */
	i2c_target_write_received_cb_t write_received;
	/** @copybrief i2c_target_read_processed_cb_t */
	i2c_target_read_processed_cb_t read_processed;
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE) || defined(__DOXYGEN__)
	/**
	 * @kconfig_dep{CONFIG_I2C_TARGET_BUFFER_MODE}
	 * @copybrief i2c_target_buf_write_received_cb_t
	 */
	i2c_target_buf_write_received_cb_t buf_write_received;
	/**
	 * @kconfig_dep{CONFIG_I2C_TARGET_BUFFER_MODE}
	 * @copybrief i2c_target_buf_read_requested_cb_t
	 */
	i2c_target_buf_read_requested_cb_t buf_read_requested;
#endif
	/** @copybrief i2c_target_stop_cb_t */
	i2c_target_stop_cb_t stop;
	/** @copybrief i2c_target_error_cb_t */
	i2c_target_error_cb_t error;
};

/** @brief Structure describing a device that supports the I2C
 * target API.
 *
 * Instances of this are passed to the i2c_target_register() and
 * i2c_target_unregister() functions to indicate addition and removal
 * of a target device, respective.
 *
 * Fields other than @c node must be initialized by the module that
 * implements the device behavior prior to passing the object
 * reference to i2c_target_register().
 */
struct i2c_target_config {
	/** Private, do not modify */
	sys_snode_t node;

	/** Flags for the target device defined by I2C_TARGET_FLAGS_* constants */
	uint8_t flags;

	/** Address for this target device */
	uint16_t address;

	/** Callback functions */
	const struct i2c_target_callbacks *callbacks;
};

/**
 * @brief Validate that I2C bus is ready.
 *
 * @param spec I2C specification from devicetree
 *
 * @retval true if the I2C bus is ready for use.
 * @retval false if the I2C bus is not ready for use.
 */
static inline bool i2c_is_ready_dt(const struct i2c_dt_spec *spec)
{
	/* Validate bus is ready */
	return device_is_ready(spec->bus);
}

/**
 * @brief Check if the current message is a read operation
 *
 * @param msg The message to check
 * @return true if the I2C message is a read operation
 * @return false if the I2C message is a write operation
 */
static inline bool i2c_is_read_op(const struct i2c_msg *msg)
{
	return (msg->flags & I2C_MSG_READ) == I2C_MSG_READ;
}

/**
 * @brief Check if the current message includes a stop.
 *
 * @param msg The message to check
 * @return true if the I2C message includes a stop
 * @return false if the I2C message includes a stop
 */
static inline bool i2c_is_stop_op(const struct i2c_msg *msg)
{
	return (msg->flags & I2C_MSG_STOP) == I2C_MSG_STOP;
}

/**
 * @brief Dump out an I2C message
 *
 * Dumps out a list of I2C messages. For any that are writes (W), the data is
 * displayed in hex. Setting dump_read will dump the data for read messages too,
 * which only makes sense when called after the messages have been processed.
 *
 * It looks something like this (with name "testing"):
 *
 * @code
 * D: I2C msg: testing, addr=56
 * D:    W len=01: 06
 * D:    W len=0e:
 * D: contents:
 * D: 00 01 02 03 04 05 06 07 |........
 * D: 08 09 0a 0b 0c 0d       |......
 * D:    W len=01: 0f
 * D:    R len=01: 6c
 * @endcode
 *
 * @param dev Target for the messages being sent. Its name will be printed in the log.
 * @param msgs Array of messages to dump.
 * @param num_msgs Number of messages to dump.
 * @param addr Address of the I2C target device.
 * @param dump_read Dump data from I2C reads, otherwise only writes have data dumped.
 */
void i2c_dump_msgs_rw(const struct device *dev, const struct i2c_msg *msgs, uint8_t num_msgs,
		      uint16_t addr, bool dump_read);

/**
 * @brief Dump out an I2C message, before it is executed.
 *
 * This is equivalent to:
 *
 *     i2c_dump_msgs_rw(dev, msgs, num_msgs, addr, false);
 *
 * The read messages' data isn't dumped.
 *
 * @param dev Target for the messages being sent. Its name will be printed in the log.
 * @param msgs Array of messages to dump.
 * @param num_msgs Number of messages to dump.
 * @param addr Address of the I2C target device.
 */
static inline void i2c_dump_msgs(const struct device *dev, const struct i2c_msg *msgs,
				 uint8_t num_msgs, uint16_t addr)
{
	i2c_dump_msgs_rw(dev, msgs, num_msgs, addr, false);
}

#if defined(CONFIG_I2C_STATS) || defined(__DOXYGEN__)

#include <zephyr/stats/stats.h>

/** @cond INTERNAL_HIDDEN */

STATS_SECT_START(i2c)
STATS_SECT_ENTRY32(bytes_read)
STATS_SECT_ENTRY32(bytes_written)
STATS_SECT_ENTRY32(message_count)
STATS_SECT_ENTRY32(transfer_call_count)
STATS_SECT_END;

STATS_NAME_START(i2c)
STATS_NAME(i2c, bytes_read)
STATS_NAME(i2c, bytes_written)
STATS_NAME(i2c, message_count)
STATS_NAME(i2c, transfer_call_count)
STATS_NAME_END(i2c);

/** @endcond */


/**
 * @brief I2C specific device state which allows for i2c device class specific additions
 */
struct i2c_device_state {
	struct device_state devstate;
	struct stats_i2c stats;
};

/**
 * @brief Updates the i2c stats for i2c transfers
 *
 * @param dev I2C device to update stats for
 * @param msgs Array of struct i2c_msg
 * @param num_msgs Number of i2c_msgs
 */
static inline void i2c_xfer_stats(const struct device *dev, struct i2c_msg *msgs,
				  uint8_t num_msgs)
{
	struct i2c_device_state *state =
		CONTAINER_OF(dev->state, struct i2c_device_state, devstate);
	uint32_t bytes_read = 0U;
	uint32_t bytes_written = 0U;

	STATS_INC(state->stats, transfer_call_count);
	STATS_INCN(state->stats, message_count, num_msgs);
	for (uint8_t i = 0U; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			bytes_read += msgs[i].len;
		} else {
			bytes_written += msgs[i].len;
		}
	}
	STATS_INCN(state->stats, bytes_read, bytes_read);
	STATS_INCN(state->stats, bytes_written, bytes_written);
}

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Define a statically allocated and section assigned i2c device state
 */
#define Z_I2C_DEVICE_STATE_DEFINE(dev_id)				\
	static struct i2c_device_state Z_DEVICE_STATE_NAME(dev_id)	\
	__attribute__((__section__(".z_devstate")))

/**
 * @brief Define an i2c device init wrapper function
 *
 * This does device instance specific initialization of common data (such as stats)
 * and calls the given init_fn
 */
#define Z_I2C_INIT_FN(dev_id, init_fn)					\
	static inline int UTIL_CAT(dev_id, _init)(const struct device *dev) \
	{								\
		struct i2c_device_state *state =			\
			CONTAINER_OF(dev->state, struct i2c_device_state, devstate); \
		stats_init(&state->stats.s_hdr, STATS_SIZE_32, 4,	\
			   STATS_NAME_INIT_PARMS(i2c));			\
		stats_register(dev->name, &(state->stats.s_hdr));	\
		if (!is_null_no_warn(init_fn)) {			\
			return init_fn(dev);				\
		}							\
									\
		return 0;						\
	}

/** @endcond */

/**
 * @brief Like DEVICE_DT_DEINIT_DEFINE() with I2C specifics.
 *
 * @details Defines a device which implements the I2C API. May
 * generate a custom device_state container struct and init_fn
 * wrapper when needed depending on I2C @kconfig{CONFIG_I2C_STATS}.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Name of the init function of the driver. Can be `NULL`.
 *
 * @param deinit_fn Name of the deinit function of the driver. Can be `NULL`.
 *
 * @param pm PM device resources reference (NULL if device does not use PM).
 *
 * @param data Pointer to the device's private data.
 *
 * @param config The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 */
#define I2C_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm,	\
				    data, config, level, prio, api, ...)\
	Z_I2C_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));		\
	Z_I2C_INIT_FN(Z_DEVICE_DT_DEV_ID(node_id), init_fn)		\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),		\
			DEVICE_DT_NAME(node_id),			\
			&UTIL_CAT(Z_DEVICE_DT_DEV_ID(node_id), _init),	\
			deinit_fn, Z_DEVICE_DT_FLAGS(node_id), pm, data,\
			config,	level, prio, api,			\
			&(Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)).devstate), \
			__VA_ARGS__)

#else /* CONFIG_I2C_STATS */

static inline void i2c_xfer_stats(const struct device *dev, struct i2c_msg *msgs,
				  uint8_t num_msgs)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msgs);
	ARG_UNUSED(num_msgs);
}

#define I2C_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm,	\
				    data, config, level, prio, api, ...)\
	DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm, data,	\
				config, level, prio, api, __VA_ARGS__)

#endif /* CONFIG_I2C_STATS */

/**
 * @brief Like I2C_DEVICE_DT_DEINIT_DEFINE() but without deinit_fn
 */
#define I2C_DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level,	\
			     prio, api, ...)				\
	I2C_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, NULL, pm, data,	\
				    config, level, prio, api,		\
				    __VA_ARGS__)

/**
 * @brief Like I2C_DEVICE_DT_DEINIT_DEFINE() for an instance of a DT_DRV_COMPAT compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to I2C_DEVICE_DT_DEINIT_DEFINE().
 *
 * @param ... other parameters as expected by I2C_DEVICE_DT_DEINIT_DEFINE().
 */
#define I2C_DEVICE_DT_INST_DEINIT_DEFINE(inst, ...)		\
	I2C_DEVICE_DT_DEINIT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Like I2C_DEVICE_DT_DEFINE() for an instance of a DT_DRV_
/* ... truncated ... */
```

## Source: `data/rtos/zephyr/include/zephyr/device.h`

```c

/* excerpt lines 1-60 */
/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_LLEXT
#include <zephyr/llext/symbol.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Model
 * @defgroup device_model Device Model
 * @since 1.0
 * @version 1.1.0
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Flag value used in lists of device dependencies to separate distinct
 * groups.
 */
#define Z_DEVICE_DEPS_SEP INT16_MIN

/**
 * @brief Flag value used in lists of device dependencies to indicate the end of
 * the list.
 */
#define Z_DEVICE_DEPS_ENDS INT16_MAX

/** @brief Determine if a DT node is mutable */
#define Z_DEVICE_IS_MUTABLE(node_id)                                                               \
	COND_CODE_1(IS_ENABLED(CONFIG_DEVICE_MUTABLE), (DT_PROP(node_id, zephyr_mutable)), (0))

/** @endcond */

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every @ref device has an associated handle. You can get a pointer to a

/* excerpt lines 740-821 */
		 * CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC_NUM
		 */
		while ((rv[i] != Z_DEVICE_DEPS_ENDS) &&
		       (rv[i] != DEVICE_HANDLE_NULL)) {
			++i;
		}
		*count = i;
	}

	return rv;
}

/**
 * @brief Visit every device that @p dev directly requires.
 *
 * Zephyr maintains information about which devices are directly required by
 * another device; for example an I2C-based sensor driver will require an I2C
 * controller for communication. Required devices can derive from
 * statically-defined devicetree relationships or dependencies registered at
 * runtime.
 *
 * This API supports operating on the set of required devices. Example uses
 * include making sure required devices are ready before the requiring device is
 * used, and releasing them when the requiring device is no longer needed.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor_cb function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest. The devices that this device depends on will
 * be used as the set of devices to visit. This parameter must not be null.
 * @param visitor_cb the function that should be invoked on each device in the
 * dependency set. This parameter must not be null.
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor_cb tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or
 * the negative value returned from the first visit that did not succeed.
 */
int device_required_foreach(const struct device *dev,
			    device_visitor_callback_t visitor_cb,
			    void *context);

/**
 * @brief Visit every device that @p dev directly supports.
 *
 * Zephyr maintains information about which devices are directly supported by
 * another device; for example an I2C controller will support an I2C-based
 * sensor driver. Supported devices can derive from statically-defined
 * devicetree relationships.
 *
 * This API supports operating on the set of supported devices. Example uses
 * include iterating over the devices connected to a regulator when it is
 * powered on.
 *
 * There is no guarantee on the order in which required devices are visited.
 *
 * If the @p visitor_cb function returns a negative value iteration is halted,
 * and the returned value from the visitor is returned from this function.
 *
 * @note This API is not available to unprivileged threads.
 *
 * @param dev a device of interest. The devices that this device supports
 * will be used as the set of devices to visit. This parameter must not be null.
 * @param visitor_cb the function that should be invoked on each device in the
 * support set. This parameter must not be null.
 * @param context state that is passed through to the visitor function. This
 * parameter may be null if @p visitor_cb tolerates a null @p context.
 *
 * @return The number of devices that were visited if all visits succeed, or the
 * negative value returned from the first visit that did not succeed.
 */
int device_supported_foreach(const struct device *dev,
			     device_visitor_callback_t visitor_cb,
			     void *context);

#endif /* CONFIG_DEVICE_DEPS */

/**

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/zephyr/include/zephyr/devicetree.h`

```c

/* excerpt lines 1-60 */
/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2020 Nordic Semiconductor
 * Copyright (c) 2020, Linaro Ltd.
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * Not a generated file. Feel free to modify.
 */

/**
 * @file
 * @brief Devicetree main header
 *
 * API for accessing the current application's devicetree macros.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_H_
#define ZEPHYR_INCLUDE_DEVICETREE_H_

#include <zephyr/devicetree_generated.h>
#include <zephyr/irq_multilevel.h>

#if !defined(_LINKER) && !defined(_ASMLANGUAGE)
#include <stdint.h>
#endif

#include <zephyr/sys/util.h>

/**
 * @brief devicetree.h API
 * @defgroup devicetree Devicetree
 * @since 2.2
 * @version 1.2.0
 * @{
 * @}
 */

/*
 * Property suffixes
 * -----------------
 *
 * These are the optional parts that come after the _P_<property>
 * part in DT_N_<path-id>_P_<property-id> macros, or the "prop-suf"
 * nonterminal in the DT guide's macros.bnf file.
 *
 * Before adding new ones, check this list to avoid conflicts. If any
 * are missing from this list, please add them. It should be complete.
 *
 * _ENUM_IDX: property's value as an index into bindings enum
 * _ENUM_VAL_<val>_EXISTS property's value as a token exists
 * _EXISTS: property is defined
 * _FOREACH_PROP_ELEM: helper for "iterating" over values in the property
 * _FOREACH_PROP_ELEM_VARGS: foreach functions with variable number of arguments
 * _IDX_<i>: logical index into property
 * _IDX_<i>_EXISTS: logical index into property is defined
 * _IDX_<i>_PH: phandle array's phandle by index (or phandle, phandles)
 * _IDX_<i>_STRING_TOKEN: string array element value as a token
 * _IDX_<i>_STRING_UPPER_TOKEN: string array element value as a uppercased token
 * _IDX <i>_STRING_UNQUOTED: string array element value as a sequence of tokens, with no quotes
 * _IDX_<i>_VAL_<val>: phandle array's specifier value by index

/* excerpt lines 3684-3755 */
 * It doesn't matter whether or not the node has a matching binding,
 * or what the node's status value is. This is purely a check of
 * whether the node exists at all.
 *
 * @param node_id a node identifier
 * @return 1 if the node identifier refers to a node,
 *         0 otherwise.
 */
#define DT_NODE_EXISTS(node_id) IS_ENABLED(DT_CAT(node_id, _EXISTS))

/**
 * @brief Does a node identifier refer to a node with a status?
 *
 * Example uses:
 *
 * @code{.c}
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), okay)
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), disabled)
 * @endcode
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - has a status property matching the second argument
 *   (except that either a missing status or an `ok` status
 *   in the devicetree is treated as if it were `okay` instead)
 *
 * @param node_id a node identifier
 * @param status a status as one of the tokens okay or disabled, not a string
 * @return 1 if the node has the given status, 0 otherwise.
 */
#define DT_NODE_HAS_STATUS(node_id, status) \
	DT_NODE_HAS_STATUS_INTERNAL(node_id, status)

/**
 * @brief Does a node identifier refer to a node with a status `okay`?
 *
 * Example uses:
 *
 * @code{.c}
 *     DT_NODE_HAS_STATUS_OKAY(DT_PATH(soc, i2c_12340000))
 * @endcode
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - has a status property as `okay`
 *
 * As usual, both a missing status and an `ok` status are treated as
 * `okay`.
 *
 * @param node_id a node identifier
 * @return 1 if the node has status as `okay`, 0 otherwise.
 */
#define DT_NODE_HAS_STATUS_OKAY(node_id) DT_NODE_HAS_STATUS(node_id, okay)

/**
 * @brief Does the devicetree have a status `okay` node with a compatible?
 *
 * Test for whether the devicetree has any nodes with status `okay`
 * and the given compatible. That is, this returns 1 if and only if
 * there is at least one @p node_id for which both of these
 * expressions return 1:
 *
 * @code{.c}
 *     DT_NODE_HAS_STATUS(node_id, okay)
 *     DT_NODE_HAS_COMPAT(node_id, compat)
 * @endcode
 *
 * As usual, both a missing status and an `ok` status are treated as
 * `okay`.
 *

/* excerpt lines 4002-4088 */
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node's bus controller
 *
 * Get the node identifier of the node's bus controller. This can be
 * used with DT_PROP() to get properties of the bus controller.
 *
 * It is an error to use this with nodes which do not have bus
 * controllers.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     i2c@deadbeef {
 *             status = "okay";
 *             clock-frequency = < 100000 >;
 *
 *             i2c_device: accelerometer@12 {
 *                     ...
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_PROP(DT_BUS(DT_NODELABEL(i2c_device)), clock_frequency) // 100000
 * @endcode
 *
 * @param node_id node identifier
 * @return a node identifier for the node's bus controller
 */
#define DT_BUS(node_id) DT_CAT(node_id, _BUS)

/**
 * @brief Is a node on a bus of a given type?
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     DT_ON_BUS(DT_NODELABEL(temp), i2c) // 1
 *     DT_ON_BUS(DT_NODELABEL(temp), spi) // 0
 * @endcode
 *
 * @param node_id node identifier
 * @param bus lowercase-and-underscores bus type as a C token (i.e.
 *            without quotes)
 * @return 1 if the node is on a bus of the given type,
 *         0 otherwise
 */
#define DT_ON_BUS(node_id, bus) IS_ENABLED(DT_CAT3(node_id, _BUS_, bus))

/**
 * @}
 */

/**
 * @defgroup devicetree-inst Instance-based devicetree APIs
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node identifier for an instance of a `DT_DRV_COMPAT` compatible
 * @param inst instance number
 * @return a node identifier for the node with `DT_DRV_COMPAT` compatible and
 *         instance number @p inst
 */
#define DT_DRV_INST(inst) DT_INST(inst, DT_DRV_COMPAT)

/**
 * @brief Get a `DT_DRV_COMPAT` parent's node identifier

/* excerpt lines 4904-5114 */
 * @param default_value a fallback value to expand to
 * @return the property's value as a sequence of tokens, with no quotes, or @p default_value
 */
#define DT_INST_STRING_UNQUOTED_OR(inst, name, default_value) \
	DT_STRING_UNQUOTED_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Test if any enabled node with the given compatible is on
 *        the given bus type
 *
 * This is like DT_ANY_INST_ON_BUS_STATUS_OKAY(), except it can also
 * be useful for handling multiple compatibles in single source file.
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_some_sensor, i2c) // 1
 * @endcode
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(compat, bus) \
	IS_ENABLED(DT_CAT4(DT_COMPAT_, compat, _BUS_, bus))

/**
 * @brief Test if any `DT_DRV_COMPAT` node is on a bus of a given type
 *        and has status okay
 *
 * This is a special-purpose macro which can be useful when writing
 * drivers for devices which can appear on multiple buses. One example
 * is a sensor device which may be wired on an I2C or SPI bus.
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) // 1
 * @endcode
 *
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_ANY_INST_ON_BUS_STATUS_OKAY(bus) \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(DT_DRV_COMPAT, bus)

/**
 * @brief Check if any `DT_DRV_COMPAT` node with status `okay` has a given
 *        property.
 *
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo = <1>;
 *             bar = <2>;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo = <2>;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz = <1>;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo) // 1
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(bar) // 1
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz) // 0
 * @endcode
 */
#define DT_ANY_INST_HAS_PROP_STATUS_OKAY(prop)							\
	UTIL_NOT(IS_EMPTY(									\
		DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ANY_INST_HAS_PROP_STATUS_OKAY_, prop)))

/**
 * @brief Check if all `DT_DRV_COMPAT` node with status `okay` has a given
 *        property. If all nodes are disabled, this will return 1.
 *
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo = <1>;
 *             bar = <2>;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo = <2>;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz = <1>;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ALL_INST_HAS_PROP_STATUS_OKAY(foo) // 1
 *     DT_ALL_INST_HAS_PROP_STATUS_OKAY(bar) // 0
 *     DT_ALL_INST_HAS_PROP_STATUS_OKAY(baz) // 0
 * @endcode
 */
#define DT_ALL_INST_HAS_PROP_STATUS_OKAY(prop) \
	IS_EMPTY(DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ALL_INST_HAS_PROP_STATUS_OKAY_, prop))

/**
 * @brief Check if any device node with status `okay` has a given
 *        property.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo = <1>;
 *             bar = <2>;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo = <2>;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz = <1>;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, foo) // 1
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, bar) // 1
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, baz) // 0

/* excerpt lines 5116-5163 */
 */
#define DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(compat, prop) \
	(DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(compat, DT_COMPAT_NODE_HAS_PROP_AND_OR, prop) 0)

/**
 * @brief Check if any `DT_DRV_COMPAT` node with status `okay` has a given
 *        boolean property that exists.
 *
 * This differs from @ref DT_ANY_INST_HAS_PROP_STATUS_OKAY because even when not present
 * on a node, the boolean property is generated with a value of 0 and therefore exists.
 *
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo;
 *             bar;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_HAS_BOOL_STATUS_OKAY(foo) // 1
 *     DT_ANY_INST_HAS_BOOL_STATUS_OKAY(bar) // 1

/* excerpt lines 5165-5212 */
 * @endcode
 */
#define DT_ANY_INST_HAS_BOOL_STATUS_OKAY(prop)							\
	UTIL_NOT(IS_EMPTY(									\
		DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ANY_INST_HAS_BOOL_STATUS_OKAY_, prop)))

/**
 * @brief Check if all `DT_DRV_COMPAT` node with status `okay` has a given
 *        boolean property that exists. If all nodes are disabled, this
 *        will return 1.
 * *
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo;
 *             bar;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ALL_INST_HAS_BOOL_STATUS_OKAY(foo) // 1
 *     DT_ALL_INST_HAS_BOOL_STATUS_OKAY(bar) // 0

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/zephyr/include/zephyr/kernel.h`

```c

/* excerpt lines 1-60 */
/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_

#if !defined(_ASMLANGUAGE)
#include <zephyr/kernel_includes.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <zephyr/tracing/tracing_macros.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Zephyr currently assumes the size of a couple standard types to simplify
 * print string formats. Let's make sure this doesn't change without notice.
 */
BUILD_ASSERT(sizeof(int32_t) == sizeof(int));
BUILD_ASSERT(sizeof(int64_t) == sizeof(long long));
BUILD_ASSERT(sizeof(intptr_t) == sizeof(long));

/**
 * @brief Kernel APIs
 * @defgroup kernel_apis Kernel APIs
 * @since 1.0
 * @version 1.0.0
 * @{
 * @}
 */

#define K_ANY NULL

#if (CONFIG_NUM_COOP_PRIORITIES + CONFIG_NUM_PREEMPT_PRIORITIES) == 0
#error Zero available thread priorities defined!
#endif

#define K_PRIO_COOP(x) (-(CONFIG_NUM_COOP_PRIORITIES - (x)))
#define K_PRIO_PREEMPT(x) (x)

#define K_HIGHEST_THREAD_PRIO (-CONFIG_NUM_COOP_PRIORITIES)
#define K_LOWEST_THREAD_PRIO CONFIG_NUM_PREEMPT_PRIORITIES
#define K_IDLE_PRIO K_LOWEST_THREAD_PRIO
#define K_HIGHEST_APPLICATION_THREAD_PRIO (K_HIGHEST_THREAD_PRIO)

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/zephyr/doc/hardware/peripherals/i2c.rst`

```
.. _i2c_api:

Inter-Integrated Circuit (I2C) Bus
##################################

Overview
********

.. note::

   The terminology used in Zephyr I2C APIs follows that of the
   `NXP I2C Bus Specification Rev 7.0 <i2c-specification_>`_. These changed
   from previous revisions as of its release October 1, 2021.

`I2C`_ (Inter-Integrated Circuit, pronounced "eye
squared see") is a commonly-used two-signal shared peripheral interface
bus.  Many system-on-chip solutions provide controllers that communicate
on an I2C bus.  Devices on the bus can operate in two roles: as a
"controller" that initiates transactions and controls the clock, or as a
"target" that responds to transaction commands.  A I2C controller on a
given SoC will generally support the controller role, and some will also
support the target mode.  Zephyr has API for both roles.

.. _i2c-controller-api:

I2C Controller API
==================

Zephyr's I2C controller API is used when an I2C peripheral controls the bus,
particularly the start and stop conditions and the clock.  This is
the most common mode, used to interact with I2C devices like sensors and
serial memory.

This API is supported in all in-tree I2C peripheral drivers and is
considered stable.

.. _i2c-target-api:

I2C Target API
==============

Zephyr's I2C target API is used when an I2C peripheral responds to
transactions initiated by a different controller on the bus.  It might
be used for a Zephyr application with transducer roles that are
controlled by another device such as a host processor.

This API is supported in very few in-tree I2C peripheral drivers.  The
API is considered experimental, as it is not compatible with the
capabilities of all I2C peripherals supported in controller mode.


Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I2C`

API Reference
*************

.. doxygengroup:: i2c_interface

.. _i2c-specification:
   https://www.nxp.com/docs/en/user-guide/UM10204.pdf

.. _I2C: i2c-specification_
```
