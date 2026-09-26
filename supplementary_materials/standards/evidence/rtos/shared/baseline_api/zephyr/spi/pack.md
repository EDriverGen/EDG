# Raw RTOS/Bus Pack: zephyr / spi

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/zephyr/include/zephyr/drivers/spi.h`

```c
/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup spi_interface
 * @brief Main header file for SPI (Serial Peripheral Interface) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_H_

/**
 * @brief Interfaces for Serial Peripheral Interface (SPI)
 *        controllers.
 * @defgroup spi_interface SPI
 * @since 1.0
 * @version 1.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/spi/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/stats/stats.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SPI operational mode
 * @{
 */

/**
 * @brief Master (controller) mode.
 *
 * In this case the device used with the API will function as a controller,
 * meaning it will control the CLK line on the SPI bus and the chip select,
 * and therefore have full control over the timing of the transaction.
 */
#define SPI_OP_MODE_MASTER	0U

/**
 * @brief Slave (peripheral) mode.
 *
 * With this mode, the device will function as a peripheral,
 * meaning it will need to wait for it's select line to be asserted,
 * and will be need to be subject to pacing by a controller's clock in order to
 * send and receive data during a transaction.
 */
#define SPI_OP_MODE_SLAVE	BIT(0)  /**< Slave mode. */

/** @cond INTERNAL_HIDDEN */
#define SPI_OP_MODE_MASK	0x1U
/** @endcond */

/**
 * @brief Get SPI Operational mode bitmask from a @ref spi_operation_t
 */
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)
/** @} */


/**
 * @name SPI Clock Modes
 * @{
 */

/**
 * @brief Clock Polarity (Clock Idle State)
 *
 * @details
 * Used in @ref spi_operation_t definition.
 * If set, clock idle state will be 1 and active state will be 0.
 * If unset, clock idle state will be 0 and active state will be 1.
 * Unset is the default.
 */
#define SPI_MODE_CPOL		BIT(1)

/**
 * @brief Clock Phase (Clock data capture edge)
 *
 * @details
 * Used in @ref spi_operation_t definition.
 * If set, data is captured on transition from active to idle CLK state.
 * If unset, data is captured on transition from idle to active state.
 * Unset is the default.
 */
#define SPI_MODE_CPHA		BIT(2)

/**
 * @brief Controller loopback mode
 *
 * @details
 * For testing purposes, enable hardware loopback,
 * which means that transmit data is fed back to the receiver of the same controller.
 *
 * Not all controllers support this feature.
 */
#define SPI_MODE_LOOP		BIT(3)

/** @cond INTERNAL_HIDDEN */
#define SPI_MODE_MASK		(0xEU)
/** @endcond */

/**
 * @brief Get SPI clock polarity and phase mode bitmask from a @ref spi_operation_t
 */
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/** @} */


/**
 * @name SPI Data Word Configurations
 *
 * A SPI Data word is a value that is shifted in/out of the controller's hardware FIFO
 * and is the atomic unit of communication on the spi bus.
 * A word is also called a "data frame" in this API.
 * A transfer is made up of an arbitrary number of words.
 * The following options specify configurations of the SPI word for the operation.
 *
 * @{
 */

/** Words are most significant bit first, used for @ref spi_operation_t */
#define SPI_TRANSFER_MSB	(0U)
/** Words are least significant bit first, used for @ref spi_operation_t */
#define SPI_TRANSFER_LSB	BIT(4)

/** @cond INTERNAL_HIDDEN */
#define SPI_WORD_SIZE_SHIFT	(5U)
#define SPI_WORD_SIZE_MASK	(0x3FU << SPI_WORD_SIZE_SHIFT)
/** @endcond */

/**
 * @brief Get SPI word size in bits from a @ref spi_operation_t
 *
 * @param operation A @ref spi_operation_t from which to get the configured word size.
 * @return The size (in bits) of a spi word for the operation.
 */
#define SPI_WORD_SIZE_GET(operation)					\
	(((operation) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)

/**
 * @brief Get a bitmask to set the word size in a @ref spi_operation_t
 *
 * @param word_size The size of a SPI data frame in bits.
 * @return A bitmask to apply to a @ref spi_operation_t
 */
#define SPI_WORD_SET(word_size)			\
	((word_size) << SPI_WORD_SIZE_SHIFT)

/** @} */


/**
 * @name SPI Transfer control flags
 * @{
 */

/**
 * @brief Keep chip select active after transaction
 *
 * After one of the spi transceive calls described in this API,
 * if this flag is set in the spi config operation, then
 * attempt to keep the CS active after the call, if supported and possible.
 */
#define SPI_HOLD_ON_CS		BIT(12)

/**
 * @brief Retain ownership of the spi device
 *
 * This is a software control parameter that will prevent the spi device
 * from being accessed by other API callers after the transaction,
 * and therefore should be used with caution.
 *
 * The identifying piece of information for who "locks" the device
 * is the spi_config pointer given to the transaction API, so this same
 * config should be re-used to do another transaction or release the lock.
 *
 * See @ref spi_release for how to release the  lock.
 */
#define SPI_LOCK_ON		BIT(13)

/**
 * @brief Chip select active state configuration
 *
 * If this flag is set, the CS will be active high.
 * If this flag is unset, the CS will be active low.
 *
 * Default is active low (unset) as that is most common for spi peripherals.
 *
 * Not all controllers are able to handle this natively, in which case a
 * gpio can still be used to control the CS through software with a @ref spi_cs_control
 */
#define SPI_CS_ACTIVE_HIGH	BIT(14)

/** @} */


/**
 * @name SPI MISO lines
 * @{
 *
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 * Without @kconfig{CONFIG_SPI_EXTENDED_MODES} being enabled, single is the
 * only supported one.
 */
#define SPI_LINES_SINGLE	(0U << 16)     /**< Single line */
#define SPI_LINES_DUAL		(1U << 16)     /**< Dual lines */
#define SPI_LINES_QUAD		(2U << 16)     /**< Quad lines */
#define SPI_LINES_OCTAL		(3U << 16)     /**< Octal lines */

#define SPI_LINES_MASK		(0x3U << 16)   /**< Mask for MISO lines in spi_operation_t */

/** @} */

/**
 * @name SPI GPIO Chip Select control
 * @{
 */

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller internal CS logic.
 *
 */
struct spi_cs_control {
	union {
		struct {
			/**
			 * GPIO devicetree specification of CS GPIO.
			 * The device pointer can be set to NULL to fully inhibit CS control if
			 * necessary. The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
			 * equivalent to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct
			 * spi_config.
			 */
			struct gpio_dt_spec gpio;
			/**
			 * Delay in microseconds to wait before starting the
			 * transmission and before releasing the CS line.
			 */
			uint32_t delay;
		};
		struct {
			/**
			 * CS enable lead time, i.e. how long should the CS be asserted
			 * before the first clock. Specified in nanoseconds.
			 */
			uint32_t setup_ns;
			/**
			 * CS enable lag time, i.e. how long should the CS be asserted
			 * after the last clock, before the CS de-asserts.
			 * Specified in nanoseconds.
			 */
			uint32_t hold_ns;
		};
	};
	/* To keep track of which form of this struct is valid */
	bool cs_is_gpio;
};

/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a SPI device's chip select pin
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     gpio1: gpio@abcd0001 { ... };
 *
 *     gpio2: gpio@abcd0002 { ... };
 *
 *     spi@abcd0003 {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: spi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(a)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio1)), 10, GPIO_ACTIVE_LOW }
 *     SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(b)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio2)), 20, GPIO_ACTIVE_LOW }
 * @endcode
 *
 * @param spi_dev a SPI device node identifier
 * @return #gpio_dt_spec struct corresponding with spi_dev's chip select
 */
#define SPI_CS_GPIOS_DT_SPEC_GET(spi_dev)			\
	GPIO_DT_SPEC_GET_BY_IDX_OR(DT_BUS(spi_dev), cs_gpios,	\
				   DT_REG_ADDR_RAW(spi_dev), {})

/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a SPI device's chip select pin
 *
 * This is equivalent to
 * <tt>SPI_CS_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return #gpio_dt_spec struct corresponding with spi_dev's chip select
 */
#define SPI_CS_GPIOS_DT_SPEC_INST_GET(inst) \
	SPI_CS_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))

/** @cond INTERNAL_HIDDEN */
#define SPI_CS_CONTROL_MAX_DELAY(node_id)			\
	MAX(DT_PROP_OR(node_id, spi_cs_setup_delay_ns, 0),	\
	    DT_PROP_OR(node_id, spi_cs_hold_delay_ns, 0))


#define SPI_CS_CONTROL_INIT_GPIO(node_id, ...)						\
	.gpio = SPI_CS_GPIOS_DT_SPEC_GET(node_id),					\
	.delay = COND_CODE_1(IS_EMPTY(__VA_ARGS__),					\
			(DIV_ROUND_UP(SPI_CS_CONTROL_MAX_DELAY(node_id), 1000)),	\
			(__VA_ARGS__)),

#define SPI_CS_CONTROL_INIT_NATIVE(node_id)						\
	.setup_ns = DT_PROP_OR(node_id, spi_cs_setup_delay_ns, 0),			\
	.hold_ns = DT_PROP_OR(node_id, spi_cs_hold_delay_ns, 0),

#define SPI_DEPRECATE_DELAY_WARN							\
	__WARN("Delay parameter in SPI DT macros is deprecated, use DT prop instead")
/** @endcond */

/**
 * @brief Initialize and get a pointer to a @p spi_cs_control from a
 *        devicetree node identifier
 *
 * This helper is useful for initializing a device on a SPI bus. It
 * initializes a struct spi_cs_control and returns a pointer to it.
 * Here, @p node_id is a node identifier for a SPI device, not a SPI
 * controller.
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     spi@abcd0001 {
 *             cs-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
 *             spidev: spi-device@0 { ... };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     struct spi_cs_control ctrl =
 *             SPI_CS_CONTROL_INIT(DT_NODELABEL(spidev));
 * @endcode
 *
 * This example is roughly equivalent to:
 *
 * @code{.c}
 *     struct spi_cs_control ctrl = {
 *             .gpio = SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(spidev)),
 *             .delay = DT_PROP(node_id, cs_delay_ns) / 1000,
 *             .cs_is_gpio = true,
 *     };
 * @endcode
 *
 * For non-gpio CS, the idea is similar but the lead and lag fields of the cs struct
 * will be populated instead.
 *
 * @param node_id Devicetree node identifier for a device on a SPI bus
 *
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_INIT(node_id, ...)					\
{										\
	COND_CODE_0(IS_EMPTY(__VA_ARGS__), (SPI_DEPRECATE_DELAY_WARN), ())	\
	COND_CODE_1(DT_SPI_DEV_HAS_CS_GPIOS(node_id),				\
			(SPI_CS_CONTROL_INIT_GPIO(node_id, __VA_ARGS__)),	\
			(SPI_CS_CONTROL_INIT_NATIVE(node_id)))			\
	.cs_is_gpio = DT_SPI_DEV_HAS_CS_GPIOS(node_id),				\
}

/**
 * @brief Get a pointer to a @p spi_cs_control from a devicetree node
 *
 * This is equivalent to
 * <tt>SPI_CS_CONTROL_INIT(DT_DRV_INST(inst), delay)</tt>.
 *
 * Therefore, @p DT_DRV_COMPAT must already be defined before using
 * this macro.
 *
 * @param inst Devicetree node instance number
 *
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_INIT_INST(inst)			\
	SPI_CS_CONTROL_INIT(DT_DRV_INST(inst))

/** @} */

/**
 * @typedef spi_operation_t
 * Opaque type to hold the SPI operation flags.
 */
#if defined(CONFIG_SPI_EXTENDED_MODES)
typedef uint32_t spi_operation_t;
#else
typedef uint16_t spi_operation_t;
#endif

/**
 * @brief SPI controller configuration structure
 *
 * @warning Most drivers use pointer comparison to determine whether a passed
 * configuration is different from one used in a previous transaction.
 * Changes to fields in the structure may not be detected.
 */
struct spi_config {
	/** @brief Bus frequency in Hertz. */
	uint32_t frequency;
	/**
	 * @brief Operation flags.
	 *
	 * It is a bit field with the following parts:
	 *
	 * - 0:      Master or slave.
	 * - 1..3:   Clock polarity, phase and loop mode.
	 * - 4:      LSB or MSB first.
	 * - 5..10:  Size of a data frame (word) in bits.
	 * - 11:     Full/half duplex.
	 * - 12:     Hold on the CS line if possible.
	 * - 13:     Keep resource locked for the caller.
	 * - 14:     Active high CS logic.
	 * - 15:     Motorola or TI frame format (optional).
	 *
	 * If @kconfig{CONFIG_SPI_EXTENDED_MODES} is enabled:
	 *
	 * - 16..17: MISO lines (Single/Dual/Quad/Octal).
	 * - 18..31: Reserved for future use.
	 */
	spi_operation_t operation;
	/** @brief Slave number from 0 to host controller slave limit. */
	uint16_t slave;
	/**
	 * @brief GPIO chip-select line (optional, must be initialized to zero
	 * if not used).
	 */
	struct spi_cs_control cs;
	/**
	 * @brief Delay between SPI words on SCK line in nanoseconds, if supported.
	 * Value of zero will attempt to use half of the SCK period.
	 */
	uint16_t word_delay;
};

/** @cond INTERNAL_HIDDEN */
/* converts from the special DT zero value to half of the frequency, for drivers usage mostly */
static inline uint16_t spi_get_word_delay(const struct spi_config *cfg)
{
	uint32_t freq = cfg->frequency;

	if (cfg->word_delay != 0) {
		return cfg->word_delay;
	}

	if (freq == 0) {
		return 0;
	}

	uint64_t period_ns = NSEC_PER_SEC / freq;

	period_ns = MIN(period_ns, UINT16_MAX);
	period_ns /= 2;

	return (uint16_t)period_ns;
}
/** @endcond */

/**
 * @brief Structure initializer for spi_config from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_config</tt> by reading the relevant @p frequency, @p slave, and
 * @p cs data from the devicetree.
 *
 * @param node_id Devicetree node identifier for the SPI device whose
 *                struct spi_config to create an initializer for
 * @param operation_ the desired @p operation field in the struct spi_config
 */
#define SPI_CONFIG_DT(node_id, operation_, ...)				\
	{								\
		.frequency = DT_PROP(node_id, spi_max_frequency),	\
		.operation = (operation_) |				\
			DT_PROP(node_id, duplex) |			\
			DT_PROP(node_id, frame_format) |			\
			COND_CODE_1(DT_PROP(node_id, spi_cpol), SPI_MODE_CPOL, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, spi_cpha), SPI_MODE_CPHA, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, spi_hold_cs), SPI_HOLD_ON_CS, (0))	| \
			COND_CODE_1(DT_PROP(node_id, spi_lsb_first), SPI_TRANSFER_LSB, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, spi_cs_high), SPI_CS_ACTIVE_HIGH, (0)),	\
		.slave = DT_REG_ADDR(node_id),				\
		.cs = SPI_CS_CONTROL_INIT(node_id, __VA_ARGS__),	\
		.word_delay = DT_PROP(node_id, spi_interframe_delay_ns),\
	}

/**
 * @brief Structure initializer for spi_config from devicetree instance
 *
 * This is equivalent to
 * <tt>SPI_CONFIG_DT(DT_DRV_INST(inst), operation_)</tt>.
 *
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 */
#define SPI_CONFIG_DT_INST(inst, operation_, ...)		\
	SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, __VA_ARGS__)

/**
 * @brief Complete SPI DT information
 */
struct spi_dt_spec {
	/** SPI bus */
	const struct device *bus;
	/** Slave specific configuration */
	struct spi_config config;
};

/**
 * @brief Structure initializer for spi_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_dt_spec</tt> by reading the relevant bus, frequency, slave, and cs
 * data from the devicetree.
 *
 * Important: multiple fields are automatically constructed by this macro
 * which must be checked before use. @ref spi_is_ready_dt performs the required
 * @ref device_is_ready checks.
 *
 * @param node_id Devicetree node identifier for the SPI device whose
 *                struct spi_dt_spec to create an initializer for
 * @param operation_ the desired @p operation field in the struct spi_config
 */
#define SPI_DT_SPEC_GET(node_id, operation_, ...)				\
	{									\
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),				\
		.config = SPI_CONFIG_DT(node_id, operation_, __VA_ARGS__),	\
	}

/**
 * @brief Structure initializer for spi_dt_spec from devicetree instance
 *
 * This is equivalent to
 * <tt>SPI_DT_SPEC_GET(DT_DRV_INST(inst), operation_)</tt>.
 *
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 */
#define SPI_DT_SPEC_INST_GET(inst, operation_, ...) \
	SPI_DT_SPEC_GET(DT_DRV_INST(inst), operation_, __VA_ARGS__)

/**
 * @brief Value that will never compare true with any valid overrun character
 */
#define SPI_MOSI_OVERRUN_UNKNOWN 0x100

/**
 * @brief The value sent on MOSI when all TX bytes are sent, but RX continues
 *
 * For drivers where the MOSI line state when receiving is important, this value
 * can be queried at compile-time to determine whether allocating a constant
 * array is necessary.
 *
 * @param node_id Devicetree node identifier for the SPI device to query
 *
 * @retval SPI_MOSI_OVERRUN_UNKNOWN if controller does not export the value
 * @retval byte default MOSI value otherwise
 */
#define SPI_MOSI_OVERRUN_DT(node_id) \
	DT_PROP_OR(node_id, overrun_character, SPI_MOSI_OVERRUN_UNKNOWN)

/**
 * @brief The value sent on MOSI when all TX bytes are sent, but RX continues
 *
 * This is equivalent to
 * <tt>SPI_MOSI_OVERRUN_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 *
 * @retval SPI_MOSI_OVERRUN_UNKNOWN if controller does not export the value
 * @retval byte default MOSI value otherwise
 */
#define SPI_MOSI_OVERRUN_DT_INST(inst) \
	DT_INST_PROP_OR(inst, overrun_character, SPI_MOSI_OVERRUN_UNKNOWN)

/**
 * @brief SPI buffer structure
 *
 * A SPI buffer describes either a real data buffer or an indication of NOP
 * For a NOP indicator:
 *   If buffer is used for TX, only 0's will be sent for the length on the bus
 *   If buffer is used for RX, that length of data received by bus will be ignored/skipped
 */
struct spi_buf {
	/** Valid pointer to a data buffer, or NULL for NOP indication */
	void *buf;
	/** Length of the buffer @a buf in bytes, or length of NOP */
	size_t len;
};

/**
 * @brief SPI scatter-gather buffer array structure
 *
 * A spi_buf_set is a flexible description of a whole single SPI bus transfer.
 *
 * Since the set is an array of pointers to buffers, it means that pieces of a spi transfer
 * definition can be re-used across different transfers, without having to redefine or allocate
 * new memory for them each time.
 * This accomplishes what is called "scatter-gather" buffer management at the driver level with
 * user-provided buffers.
 */
struct spi_buf_set {
	/** Pointer to an array of spi_buf, or NULL */
	const struct spi_buf *buffers;
	/** Number of buffers in the array pointed to: by @a buffers */
	size_t count;
};

/**
 * @name SPI Stats
 * @{
 */
#if defined(CONFIG_SPI_STATS)
STATS_SECT_START(spi)
STATS_SECT_ENTRY32(rx_bytes)
STATS_SECT_ENTRY32(tx_bytes)
STATS_SECT_ENTRY32(transfer_error)
STATS_SECT_END;

STATS_NAME_START(spi)
STATS_NAME(spi, rx_bytes)
STATS_NAME(spi, tx_bytes)
STATS_NAME(spi, transfer_error)
STATS_NAME_END(spi);

/**
 * @brief SPI specific device state which allows for SPI device class specific additions
 */
struct spi_device_state {
	struct device_state devstate;
	struct stats_spi stats;
};

/**
 * @brief Get pointer to SPI statistics structure
 */
#define Z_SPI_GET_STATS(dev_)				\
	CONTAINER_OF(dev_->state, struct spi_device_state, devstate)->stats

/**
 * @brief Increment the rx bytes for a SPI device
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_RX_BYTES_INCN(dev_, n)			\
	STATS_INCN(Z_SPI_GET_STATS(dev_), rx_bytes, n)

/**
 * @brief Increment the tx bytes for a SPI device
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_TX_BYTES_INCN(dev_, n)			\
	STATS_INCN(Z_SPI_GET_STATS(dev_), tx_bytes, n)

/**
 * @brief Increment the transfer error counter for a SPI device
 *
 * The transfer error count is incremented when there occurred a transfer error
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_TRANSFER_ERROR_INC(dev_)			\
	STATS_INC(Z_SPI_GET_STATS(dev_), transfer_error)

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Define a statically allocated and section assigned SPI device state
 */
#define Z_SPI_DEVICE_STATE_DEFINE(dev_id)	\
	static struct spi_device_state Z_DEVICE_STATE_NAME(dev_id)	\
	__attribute__((__section__(".z_devstate")));

/**
 * @brief Define an SPI device init wrapper function
 *
 * This does device instance specific initialization of common data (such as stats)
 * and calls the given init_fn
 */
#define Z_SPI_INIT_FN(dev_id, init_fn)					\
	static inline int UTIL_CAT(dev_id, _init)(const struct device *dev) \
	{								\
		struct spi_device_state *state =			\
			CONTAINER_OF(dev->state, struct spi_device_state, devstate); \
		stats_init(&state->stats.s_hdr, STATS_SIZE_32, 3,	\
			   STATS_NAME_INIT_PARMS(spi));			\
		stats_register(dev->name, &(state->stats.s_hdr));	\
		return init_fn(dev);					\
	}
/** @endcond */

#define SPI_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn,	\
				    pm_device, data_ptr, cfg_ptr,	\
				    level, prio, api_ptr, ...)		\
	Z_SPI_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));		\
	Z_SPI_INIT_FN(Z_DEVICE_DT_DEV_ID(node_id), init_fn)		\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),		\
			DEVICE_DT_NAME(node_id),			\
			&UTIL_CAT(Z_DEVICE_DT_DEV_ID(node_id), _init),	\
			deinit_fn, Z_DEVICE_DT_FLAGS(node_id),		\
			pm_device, data_ptr, cfg_ptr, level, prio,	\
			api_ptr,					\
			&(Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)).devstate), \
			__VA_ARGS__)

static inline void spi_transceive_stats(const struct device *dev, int error,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	uint32_t tx_bytes;
	uint32_t rx_bytes;

	if (error) {
		SPI_STATS_TRANSFER_ERROR_INC(dev);
	}

	if (tx_bufs) {
		tx_bytes = tx_bufs->count ? tx_bufs->buffers->len : 0;
		SPI_STATS_TX_BYTES_INCN(dev, tx_bytes);
	}

	if (rx_bufs) {
		rx_bytes = rx_bufs->count ? rx_bufs->buffers->len : 0;
		SPI_STATS_RX_BYTES_INCN(dev, rx_bytes);
	}
}
/** @} */

#else /*CONFIG_SPI_STATS*/

/**
 * @name SPI DT Device Macros
 * @{
 */

/**
 * @brief Like DEVICE_DT_DEINIT_DEFINE() with SPI specifics.
 *
 * @details Defines a device which implements the SPI API. May
 * generate a custom device_state container struct and init_fn
 * wrapper when needed depending on SPI @kconfig{CONFIG_SPI_STATS}.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Name of the init function of the driver.
 * @param deinit_fn Name of the deinit function of the driver.
 * @param pm PM device resources reference (NULL if device does not use PM).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the configuration
 *                information for this instance of the driver.
 * @param level The initialization level. See SYS_INIT() for details.
 * @param prio Priority within the selected initialization level. See SYS_INIT()
 *             for details.
 * @param api Provides an initial pointer to the API function struct used by
 *                the driver. Can be NULL.
 */
#define SPI_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm, data,	\
				    config, level, prio, api, ...)		\
	Z_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));			\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),			\
			DEVICE_DT_NAME(node_id), init_fn, deinit_fn,		\
			Z_DEVICE_DT_FLAGS(node_id), pm, data, config,		\
			level, prio, api,					\
			&Z_
/* ... truncated ... */
```

## Source: `data/rtos/zephyr/include/zephyr/drivers/gpio.h`

```c

/* excerpt lines 1-60 */
/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2019 Piotr Mienkowski
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup gpio_interface
 * @brief Main header file for GPIO driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_H_

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/tracing/tracing.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for General Purpose Input/Output (GPIO)
 *        controllers.
 * @defgroup gpio_interface GPIO
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup gpio_interface_ext Device-specific GPIO API extensions
 *
 * @{
 * @}
 */

/**
 * @name GPIO input/output configuration flags
 * @{
 */

/** Enables pin as input. */
#define GPIO_INPUT              BIT(16)

/** Enables pin as output, no change to the output state. */
#define GPIO_OUTPUT             BIT(17)

/** Disables pin for both input and output. */

/* excerpt lines 849-896 */
 * @param spec GPIO specification from devicetree
 *
 * @retval true if the GPIO spec is ready for use.
 * @retval false if the GPIO spec is not ready for use.
 */
static inline bool gpio_is_ready_dt(const struct gpio_dt_spec *spec)
{
	/* Validate port is ready */
	return device_is_ready(spec->port);
}

/**
 * @brief Configure pin interrupt.
 *
 * @note This function can also be used to configure interrupts on pins
 *       not controlled directly by the GPIO module. That is, pins which are
 *       routed to other modules such as I2C, SPI, UART.
 *
 * @isr_ok
 *
 * @param port Pointer to device structure for the driver instance.
 * @param pin Pin number.
 * @param flags Interrupt configuration flags as defined by GPIO_INT_*.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If the operation is not implemented by the driver.
 * @retval -ENOTSUP If any of the configuration options is not supported
 *                  (unless otherwise directed by flag documentation).
 * @retval -EINVAL  Invalid argument.
 * @retval -EBUSY   Interrupt line required to configure pin interrupt is
 *                  already in use.
 * @retval -EIO I/O error when accessing an external GPIO chip.
 * @retval -EWOULDBLOCK if operation would block.
 */
__syscall int gpio_pin_interrupt_configure(const struct device *port,
					   gpio_pin_t pin,
					   gpio_flags_t flags);

static inline int z_impl_gpio_pin_interrupt_configure(const struct device *port,
						      gpio_pin_t pin,
						      gpio_flags_t flags)
{
	const struct gpio_driver_api *api =
		(const struct gpio_driver_api *)port->api;
	__unused const struct gpio_driver_config *const cfg =
		(const struct gpio_driver_config *)port->config;
	const struct gpio_driver_data *const data =
		(const struct gpio_driver_data *)port->data;

/* ... additional content omitted by deterministic excerpt limit ... */
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

/* excerpt lines 4042-4089 */
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
 * @param inst instance number

/* excerpt lines 4933-4980 */
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

/* excerpt lines 5669-5702 */
#define DT_U64_C(_v) UINT64_C(_v)
#endif

/* Helpers for DT_NODELABEL_STRING_ARRAY. We define our own stringify
 * in order to avoid adding a dependency on toolchain.h..
 */
#define DT_NODELABEL_STRING_ARRAY_ENTRY_INTERNAL(nodelabel) DT_STRINGIFY_INTERNAL(nodelabel),
#define DT_STRINGIFY_INTERNAL(arg) DT_STRINGIFY_INTERNAL_HELPER(arg)
#define DT_STRINGIFY_INTERNAL_HELPER(arg) #arg

/** @endcond */

/* have these last so they have access to all previously defined macros */
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/devicetree/spi.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/devicetree/pwms.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/devicetree/ordinals.h>
#include <zephyr/devicetree/pinctrl.h>
#include <zephyr/devicetree/can.h>
#include <zephyr/devicetree/reset.h>
#include <zephyr/devicetree/mbox.h>
#include <zephyr/devicetree/port-endpoint.h>
#include <zephyr/devicetree/display.h>
#include <zephyr/devicetree/hwspinlock.h>
#include <zephyr/devicetree/map.h>
#include <zephyr/devicetree/wuc.h>
#include <zephyr/devicetree/mapped-partition.h>
#include <zephyr/devicetree/partitions.h>

#endif /* ZEPHYR_INCLUDE_DEVICETREE_H_ */

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

/* excerpt lines 125-255 */
}

typedef void (*k_thread_user_cb_t)(const struct k_thread *thread,
				   void *user_data);

/**
 * @brief Iterate over all the threads in the system.
 *
 * This routine iterates over all the threads in the system and
 * calls the user_cb function for each thread.
 *
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock to protect the _kernel.threads
 * list which means creation of new threads and terminations of existing
 * threads are blocked until this API returns.
 */
void k_thread_foreach(k_thread_user_cb_t user_cb, void *user_data);

/**
 * @brief Iterate over all the threads in running on specified cpu.
 *
 * This function is does otherwise the same thing as k_thread_foreach(),
 * but it only loops through the threads running on specified cpu only.
 * If CONFIG_SMP is not defined the implementation this is the same as
 * k_thread_foreach(), with an assert cpu == 0.
 *
 * @param cpu The filtered cpu number
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock to protect the _kernel.threads
 * list which means creation of new threads and terminations of existing
 * threads are blocked until this API returns.
 */
#ifdef CONFIG_SMP
void k_thread_foreach_filter_by_cpu(unsigned int cpu,
				    k_thread_user_cb_t user_cb, void *user_data);
#else
static inline
void k_thread_foreach_filter_by_cpu(unsigned int cpu,
				    k_thread_user_cb_t user_cb, void *user_data)
{
	__ASSERT(cpu == 0, "cpu filter out of bounds");
	ARG_UNUSED(cpu);
	k_thread_foreach(user_cb, user_data);
}
#endif

/**
 * @brief Iterate over all the threads in the system without locking.
 *
 * This routine works exactly the same like @ref k_thread_foreach
 * but unlocks interrupts when user_cb is executed.
 *
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock only when accessing the _kernel.threads
 * queue elements. It unlocks it during user callback function processing.
 * If a new task is created when this @c foreach function is in progress,
 * the added new task would not be included in the enumeration.
 * If a task is aborted during this enumeration, there would be a race here
 * and there is a possibility that this aborted task would be included in the
 * enumeration.
 * @note If the task is aborted and the memory occupied by its @c k_thread
 * structure is reused when this @c k_thread_foreach_unlocked is in progress
 * it might even lead to the system behave unstable.
 * This function may never return, as it would follow some @c next task
 * pointers treating given pointer as a pointer to the k_thread structure
 * while it is something different right now.
 * Do not reuse the memory that was occupied by k_thread structure of aborted
 * task if it was aborted after this function was called in any context.
 */
void k_thread_foreach_unlocked(
	k_thread_user_cb_t user_cb, void *user_data);

/**
 * @brief Iterate over the threads in running on current cpu without locking.
 *
 * This function does otherwise the same thing as
 * k_thread_foreach_unlocked(), but it only loops through the threads
 * running on specified cpu. If CONFIG_SMP is not defined the
 * implementation this is the same as k_thread_foreach_unlocked(), with an
 * assert requiring cpu == 0.
 *
 * @param cpu The filtered cpu number
 * @param user_cb Pointer to the user callback function.
 * @param user_data Pointer to user data.
 *
 * @note @kconfig{CONFIG_THREAD_MONITOR} must be set for this function
 * to be effective.
 * @note This API uses @ref k_spin_lock only when accessing the _kernel.threads
 * queue elements. It unlocks it during user callback function processing.
 * If a new task is created when this @c foreach function is in progress,
 * the added new task would not be included in the enumeration.
 * If a task is aborted during this enumeration, there would be a race here
 * and there is a possibility that this aborted task would be included in the
 * enumeration.
 * @note If the task is aborted and the memory occupied by its @c k_thread
 * structure is reused when this @c k_thread_foreach_unlocked is in progress
 * it might even lead to the system behave unstable.
 * This function may never return, as it would follow some @c next task
 * pointers treating given pointer as a pointer to the k_thread structure
 * while it is something different right now.
 * Do not reuse the memory that was occupied by k_thread structure of aborted
 * task if it was aborted after this function was called in any context.
 */
#ifdef CONFIG_SMP
void k_thread_foreach_unlocked_filter_by_cpu(unsigned int cpu,
					     k_thread_user_cb_t user_cb, void *user_data);
#else
static inline
void k_thread_foreach_unlocked_filter_by_cpu(unsigned int cpu,
					     k_thread_user_cb_t user_cb, void *user_data)
{
	__ASSERT(cpu == 0, "cpu filter out of bounds");
	ARG_UNUSED(cpu);
	k_thread_foreach_unlocked(user_cb, user_data);
}
#endif

/** @} */


/* excerpt lines 849-896 */
}

/**
 * @brief Abort a thread.
 *
 * This routine permanently stops execution of @a thread. The thread is taken
 * off all kernel queues it is part of (i.e. the ready queue, the timeout
 * queue, or a kernel object wait queue). However, any kernel resources the
 * thread might currently own (such as mutexes or memory blocks) are not
 * released. It is the responsibility of the caller of this routine to ensure
 * all necessary cleanup is performed.
 *
 * After k_thread_abort() returns, the thread is guaranteed not to be
 * running or to become runnable anywhere on the system.  Normally
 * this is done via blocking the caller (in the same manner as
 * k_thread_join()), but in interrupt context on SMP systems the
 * implementation is required to spin for threads that are running on
 * other CPUs.
 *
 * @param thread ID of thread to abort.
 */
__syscall void k_thread_abort(k_tid_t thread);

k_ticks_t z_timeout_expires(const struct _timeout *timeout);
k_ticks_t z_timeout_remaining(const struct _timeout *timeout);

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Get time when a thread wakes up, in system ticks
 *
 * This routine computes the system uptime when a waiting thread next
 * executes, in units of system ticks.  If the thread is not waiting,
 * it returns current system time.
 */
__syscall k_ticks_t k_thread_timeout_expires_ticks(const struct k_thread *thread);

static inline k_ticks_t z_impl_k_thread_timeout_expires_ticks(
						const struct k_thread *thread)
{
	return z_timeout_expires(&thread->base.timeout);
}

/**
 * @brief Get time remaining before a thread wakes up, in system ticks
 *
 * This routine computes the time remaining before a waiting thread
 * next executes, in units of system ticks.  If the thread is not

/* excerpt lines 1090-1180 */
 * @brief Set relative deadline expiration time for scheduler
 *
 * This sets the "deadline" expiration as a time delta from the
 * current time, in the same units used by k_cycle_get_32().  The
 * scheduler (when deadline scheduling is enabled) will choose the
 * next expiring thread when selecting between threads at the same
 * static priority.  Threads at different priorities will be scheduled
 * according to their static priority.
 *
 * @note Deadlines are stored internally using 32 bit unsigned
 * integers.  The number of cycles between the "first" deadline in the
 * scheduler queue and the "last" deadline must be less than 2^31 (i.e
 * a signed non-negative quantity).  Failure to adhere to this rule
 * may result in scheduled threads running in an incorrect deadline
 * order.
 *
 * @note Despite the API naming, the scheduler makes no guarantees
 * the thread WILL be scheduled within that deadline, nor does it take
 * extra metadata (like e.g. the "runtime" and "period" parameters in
 * Linux sched_setattr()) that allows the kernel to validate the
 * scheduling for achievability.  Such features could be implemented
 * above this call, which is simply input to the priority selection
 * logic.
 *
 * @kconfig_dep{CONFIG_SCHED_DEADLINE}
 *
 * @param thread A thread on which to set the deadline
 * @param deadline A time delta, in cycle units
 *
 */
__syscall void k_thread_deadline_set(k_tid_t thread, int deadline);

/**
 * @brief Set absolute deadline expiration time for scheduler
 *
 * This sets the "deadline" expiration as a timestamp in the same
 * units used by k_cycle_get_32(). The scheduler (when deadline scheduling
 * is enabled) will choose the next expiring thread when selecting between
 * threads at the same static priority.  Threads at different priorities
 * will be scheduled according to their static priority.
 *
 * Unlike @ref k_thread_deadline_set which sets a relative timestamp to a
 * "now" implicitly determined during its call, this routine sets an
 * absolute timestamp that is computed from a timestamp relative to
 * an explicit "now" that was determined before this routine is called.
 * This allows the caller to specify deadlines for multiple threads
 * using a common "now".
 *
 * @note Deadlines are stored internally using 32 bit unsigned
 * integers.  The number of cycles between the "first" deadline in the
 * scheduler queue and the "last" deadline must be less than 2^31 (i.e
 * a signed non-negative quantity).  Failure to adhere to this rule
 * may result in scheduled threads running in an incorrect deadline
 * order.
 *
 * @note Even if a provided timestamp is in the past, the kernel will
 * still schedule threads with deadlines in order from the earliest to
 * the latest.
 *
 * @note Despite the API naming, the scheduler makes no guarantees
 * the thread WILL be scheduled within that deadline, nor does it take
 * extra metadata (like e.g. the "runtime" and "period" parameters in
 * Linux sched_setattr()) that allows the kernel to validate the
 * scheduling for achievability.  Such features could be implemented
 * above this call, which is simply input to the priority selection
 * logic.
 *
 * @kconfig_dep{CONFIG_SCHED_DEADLINE}
 *
 * @param thread A thread on which to set the deadline
 * @param deadline A timestamp, in cycle units
 */
__syscall void k_thread_absolute_deadline_set(k_tid_t thread, int deadline);
#endif

/**
 * @brief Invoke the scheduler
 *
 * This routine invokes the scheduler to force a schedule point on the current
 * CPU. If invoked from within a thread, the scheduler will be invoked
 * immediately (provided interrupts were not locked when invoked). If invoked
 * from within an ISR, the scheduler will be invoked upon exiting the ISR.
 *
 * Invoking the scheduler allows the kernel to make an immediate determination
 * as to what the next thread to execute should be. Unlike yielding, this
 * routine is not guaranteed to switch to a thread of equal or higher priority
 * if any are available. For example, if the current thread is cooperative and
 * there is a still higher priority cooperative thread that is ready, then
 * yielding will switch to that higher priority thread whereas this routine
 * will not.
 *

/* excerpt lines 1253-1300 */
#endif

/**
 * @brief Suspend a thread.
 *
 * This routine prevents the kernel scheduler from making @a thread
 * the current thread. All other internal operations on @a thread are
 * still performed; for example, kernel objects it is waiting on are
 * still handed to it. Thread suspension does not impact any timeout
 * upon which the thread may be waiting (such as a timeout from a call
 * to k_sem_take() or k_sleep()). Thus if the timeout expires while the
 * thread is suspended, it is still suspended until k_thread_resume()
 * is called.
 *
 * When the target thread is active on another CPU, the caller will block until
 * the target thread is halted (suspended or aborted).  But if the caller is in
 * an interrupt context, it will spin waiting for that target thread active on
 * another CPU to halt.
 *
 * If @a thread is already suspended, the routine has no effect.
 *
 * @param thread ID of thread to suspend.
 */
__syscall void k_thread_suspend(k_tid_t thread);

/**
 * @brief Resume a suspended thread.
 *
 * This routine reverses the thread suspension from k_thread_suspend()
 * and allows the kernel scheduler to make @a thread the current thread
 * when it is next eligible for that role.
 *
 * If @a thread is not currently suspended, the routine has no effect.
 *
 * @param thread ID of thread to resume.
 */
__syscall void k_thread_resume(k_tid_t thread);

/**
 * @brief Start an inactive thread
 *
 * If a thread was created with K_FOREVER in the delay parameter, it will
 * not be added to the scheduling queue until this function is called
 * on it.
 *
 * @note This is a legacy API for compatibility.  Modern Zephyr
 * threads are initialized in the "sleeping" state and do not need
 * special handling for "start".

/* excerpt lines 2244-2291 */

/**
 * @}
 */

/**
 * @brief Kernel queue structure
 *
 * This structure is used to represent a kernel queue.
 * All the members are internal and should not be accessed directly.
 */
struct k_queue {
/**
 * @cond INTERNAL_HIDDEN
 */
	sys_sflist_t data_q;
	struct k_spinlock lock;
	_wait_q_t wait_q;

	Z_DECL_POLL_EVENT

	SYS_PORT_TRACING_TRACKING_FIELD(k_queue)
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_QUEUE_INITIALIZER(obj) \
	{ \
	.data_q = SYS_SFLIST_STATIC_INIT(&obj.data_q), \
	.lock = { }, \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q),	\
	Z_POLL_EVENT_OBJ_INIT(obj)		\
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup queue_apis Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/**

/* excerpt lines 2553-2600 */
};

/**
 * @brief futex kernel data structure
 *
 * z_futex_data are the helper data structure for k_futex to complete
 * futex contended operation on kernel side, structure z_futex_data
 * of every futex object is invisible in user mode.
 *
 * All the members are internal and should not be accessed directly.
 */
struct z_futex_data {
/**
 * @cond INTERNAL_HIDDEN
 */
	_wait_q_t wait_q;
	struct k_spinlock lock;
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_FUTEX_DATA_INITIALIZER(obj) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q) \
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup futex_apis FUTEX APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Pend the current thread on a futex
 *
 * Tests that the supplied futex contains the expected value, and if so,
 * goes to sleep until some other thread calls k_futex_wake() on it.
 *
 * @param futex Address of the futex.
 * @param expected Expected value of the futex, if it is different the caller
 *		   will not wait on it.

/* excerpt lines 2641-2688 */
 * @ingroup event_apis
 */

/**
 * @brief Kernel Event structure
 *
 * This structure is used to represent kernel events. All the members
 * are internal and should not be accessed directly.
 */

struct k_event {
/**
 * @cond INTERNAL_HIDDEN
 */
	_wait_q_t         wait_q;
	uint32_t          events;
	struct k_spinlock lock;

	SYS_PORT_TRACING_TRACKING_FIELD(k_event)

#ifdef CONFIG_OBJ_CORE_EVENT
	struct k_obj_core obj_core;
#endif
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_EVENT_INITIALIZER(obj) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&obj.wait_q), \
	.events = 0, \
	.lock = {}, \
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Initialize an event object
 *
 * This routine initializes an event object, prior to its first use.
 *
 * @param event Address of the event object.
 */

/* excerpt lines 3270-3317 */
 */
#define K_LIFO_DEFINE(name) \
	STRUCT_SECTION_ITERABLE(k_lifo, name) = \
		Z_LIFO_INITIALIZER(name)

/** @} */

/**
 * @cond INTERNAL_HIDDEN
 */
#define K_STACK_FLAG_ALLOC	((uint8_t)1)	/* Buffer was allocated */

typedef uintptr_t stack_data_t;

struct k_stack {
	_wait_q_t wait_q;
	struct k_spinlock lock;
	stack_data_t *base, *next, *top;

	uint8_t flags;

	SYS_PORT_TRACING_TRACKING_FIELD(k_stack)

#ifdef CONFIG_OBJ_CORE_STACK
	struct k_obj_core  obj_core;
#endif
};

#define Z_STACK_INITIALIZER(obj, stack_buffer, stack_num_entries) \
	{ \
	.wait_q = Z_WAIT_Q_INIT(&(obj).wait_q),	\
	.base = (stack_buffer), \
	.next = (stack_buffer), \
	.top = (stack_buffer) + (stack_num_entries), \
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @defgroup stack_apis Stack APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize a stack.
 *

/* excerpt lines 4549-4596 */
	/** @brief Flag indicating a synced work item that is being flushed.
	 *
	 * Accessed via k_work_busy_get().  May co-occur with other flags.
	 */
	K_WORK_FLUSHING = BIT(K_WORK_FLUSHING_BIT),
};

/**
 * @brief A structure used to submit work.
 *
 * All the members are internal and should not be accessed directly.
 */
struct k_work {
/**
 * @cond INTERNAL_HIDDEN
 */
	/* All fields are protected by the work module spinlock. */

	/* Node to link into k_work_q pending list. */
	sys_snode_t node;

	/* The function to be invoked by the work queue thread. */
	k_work_handler_t handler;

	/* The queue on which the work item was last submitted. */
	struct k_work_q *queue;

	/* State of the work item.
	 *
	 * The item can be DELAYED, QUEUED, and RUNNING simultaneously.
	 *
	 * It can be RUNNING and CANCELING simultaneously.
	 */
	uint32_t flags;
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_WORK_INITIALIZER(work_handler) { \
	.handler = (work_handler), \
}
/**
 * INTERNAL_HIDDEN @endcond
 */

/* excerpt lines 4756-4803 */
 *
 * All the members are internal and should not be accessed directly.
 */
struct k_work_q {
/**
 * @cond INTERNAL_HIDDEN
 */
	/* The thread that animates the work. */
	struct k_thread thread;

	/* The thread ID that animates the work. This may be an external thread
	 * if k_work_queue_run() is used.
	 */
	k_tid_t thread_id;

	/* All the following fields must be accessed only while the
	 * work module spinlock is held.
	 */

	/* List of k_work items to be worked. */
	sys_slist_t pending;

	/* Wait queue for idle work thread. */
	_wait_q_t notifyq;

	/* Wait queue for threads waiting for the queue to drain. */
	_wait_q_t drainq;

	/* Flags describing queue state. */
	uint32_t flags;

#if defined(CONFIG_WORKQUEUE_WORK_TIMEOUT)
	struct _timeout work_timeout_record;
	struct k_work *work;
	k_timeout_t work_timeout;
#endif /* defined(CONFIG_WORKQUEUE_WORK_TIMEOUT) */
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/* Provide the implementation for inline functions declared above */

static inline bool k_work_is_pending(const struct k_work *work)
{
	return k_work_busy_get(work) != 0;
}


/* excerpt lines 5159-5206 */
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Message Queue Structure
 *
 * All the members are internal and should not be accessed directly.
 */
struct k_msgq {
/**
 * @cond INTERNAL_HIDDEN
 */
	/** Message queue wait queue */
	_wait_q_t wait_q;
	/** Lock */
	struct k_spinlock lock;
	/** Message size */
	size_t msg_size;
	/** Maximal number of messages */
	uint32_t max_msgs;
	/** Start of message buffer */
	char *buffer_start;
	/** End of message buffer */
	char *buffer_end;
	/** Read pointer */
	char *read_ptr;
	/** Write pointer */
	char *write_ptr;
	/** Number of used messages */
	uint32_t used_msgs;

	Z_DECL_POLL_EVENT

	/** Message queue */
	uint8_t flags;

	SYS_PORT_TRACING_TRACKING_FIELD(k_msgq)

#ifdef CONFIG_OBJ_CORE_MSGQ
	struct k_obj_core  obj_core;
#endif
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**

/* excerpt lines 5512-5559 */
 * INTERNAL_HIDDEN @endcond
 */
};
/**
 * @brief Mailbox Structure
 *
 * All the members are internal and should not be accessed directly.
 */
struct k_mbox {
/**
 * @cond INTERNAL_HIDDEN
 */
	/** Transmit messages queue */
	_wait_q_t tx_msg_queue;
	/** Receive message queue */
	_wait_q_t rx_msg_queue;
	struct k_spinlock lock;

	SYS_PORT_TRACING_TRACKING_FIELD(k_mbox)

#ifdef CONFIG_OBJ_CORE_MAILBOX
	struct k_obj_core  obj_core;
#endif
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_MBOX_INITIALIZER(obj) \
	{ \
	.tx_msg_queue = Z_WAIT_Q_INIT(&obj.tx_msg_queue), \
	.rx_msg_queue = Z_WAIT_Q_INIT(&obj.rx_msg_queue), \
	}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Statically define and initialize a mailbox.
 *
 * The mailbox is to be accessed outside the module where it is defined using:
 *
 * @code extern struct k_mbox <name>; @endcode
 *
 * @param name Name of the mailbox.

/* excerpt lines 5666-5713 */
enum pipe_flags {
	PIPE_FLAG_OPEN = BIT(0),
	PIPE_FLAG_RESET = BIT(1),
};

/**
 * @brief Kernel pipe structure
 *
 * All the members are internal and should not be accessed directly.
 */
struct k_pipe {
/**
 * @cond INTERNAL_HIDDEN
 */
	size_t waiting;
	struct ring_buf buf;
	struct k_spinlock lock;
	_wait_q_t data;
	_wait_q_t space;
	uint8_t flags;

	Z_DECL_POLL_EVENT
#ifdef CONFIG_OBJ_CORE_PIPE
	struct k_obj_core  obj_core;
#endif
	SYS_PORT_TRACING_TRACKING_FIELD(k_pipe)
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/**
 * @cond INTERNAL_HIDDEN
 */
#define Z_PIPE_INITIALIZER(obj, pipe_buffer, pipe_buffer_size)	\
{								\
	.waiting = 0,						\
	.buf = RING_BUF_INIT(pipe_buffer, pipe_buffer_size),	\
	.data = Z_WAIT_Q_INIT(&obj.data),			\
	.space = Z_WAIT_Q_INIT(&obj.space),			\
	.flags = PIPE_FLAG_OPEN,				\
	Z_POLL_EVENT_OBJ_INIT(obj)				\
}
/**
 * INTERNAL_HIDDEN @endcond
 */

/**

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/zephyr/doc/hardware/peripherals/spi.rst`

```
.. _spi_api:

Serial Peripheral Interface (SPI) Bus
#####################################

Overview
********


API Reference
*************

.. doxygengroup:: spi_interface
```
