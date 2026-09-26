# Raw RTOS/Bus Pack: zephyr / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/zephyr/include/zephyr/drivers/gpio.h`

```c
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
#define GPIO_DISCONNECTED	0

/** @cond INTERNAL_HIDDEN */

/* Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    BIT(18)

/* Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   BIT(19)

/* Initializes output based on logic level */
#define GPIO_OUTPUT_INIT_LOGICAL BIT(20)

/** @endcond */

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
/** Configures GPIO pin as output and initializes it to a logic 0. */
#define GPIO_OUTPUT_INACTIVE    (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_LOW |		\
				 GPIO_OUTPUT_INIT_LOGICAL)
/** Configures GPIO pin as output and initializes it to a logic 1. */
#define GPIO_OUTPUT_ACTIVE      (GPIO_OUTPUT |			\
				 GPIO_OUTPUT_INIT_HIGH |	\
				 GPIO_OUTPUT_INIT_LOGICAL)

/** @} */

/**
 * @name GPIO interrupt configuration flags
 * The `GPIO_INT_*` flags are used to specify how input GPIO pins will trigger
 * interrupts. The interrupts can be sensitive to pin physical or logical level.
 * Interrupts sensitive to pin logical level take into account GPIO_ACTIVE_LOW
 * flag. If a pin was configured as Active Low, physical level low will be
 * considered as logical level 1 (an active state), physical level high will
 * be considered as logical level 0 (an inactive state).
 * The GPIO controller should reset the interrupt status, such as clearing the
 * pending bit, etc, when configuring the interrupt triggering properties.
 * Applications should use the `GPIO_INT_MODE_ENABLE_ONLY` and
 * `GPIO_INT_MODE_DISABLE_ONLY` flags to enable and disable interrupts on the
 * pin without changing any GPIO settings.
 * @{
 */

/** Disables GPIO pin interrupt. */
#define GPIO_INT_DISABLE               BIT(21)

/** @cond INTERNAL_HIDDEN */

/* Enables GPIO pin interrupt. */
#define GPIO_INT_ENABLE                BIT(22)

/* GPIO interrupt is sensitive to logical levels.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LEVELS_LOGICAL        BIT(23)

/* GPIO interrupt is edge sensitive.
 *
 * Note: by default interrupts are level sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_EDGE                  BIT(24)

/* Trigger detection when input state is (or transitions to) physical low or
 * logical 0 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LOW_0                 BIT(25)

/* Trigger detection on input state is (or transitions to) physical high or
 * logical 1 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_HIGH_1                BIT(26)

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
/* Disable/Enable interrupt functionality without changing other interrupt
 * related register, such as clearing the pending register.
 *
 * This is a component flag that should be combined with `GPIO_INT_ENABLE` or
 * `GPIO_INT_DISABLE` flags to produce a meaningful configuration.
 */
#define GPIO_INT_ENABLE_DISABLE_ONLY   BIT(27)
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

#define GPIO_INT_MASK                  (GPIO_INT_DISABLE | \
					GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** @endcond */

/** Configures GPIO interrupt to be triggered on pin rising edge and enables it.
 */
#define GPIO_INT_EDGE_RISING           (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin falling edge and enables
 * it.
 */
#define GPIO_INT_EDGE_FALLING          (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin rising or falling edge and
 * enables it.
 */
#define GPIO_INT_EDGE_BOTH             (GPIO_INT_ENABLE | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0 | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin physical level low and
 * enables it.
 */
#define GPIO_INT_LEVEL_LOW             (GPIO_INT_ENABLE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin physical level high and
 * enables it.
 */
#define GPIO_INT_LEVEL_HIGH            (GPIO_INT_ENABLE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 0 and enables it.
 */
#define GPIO_INT_EDGE_TO_INACTIVE      (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 1 and enables it.
 */
#define GPIO_INT_EDGE_TO_ACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_EDGE | \
					GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin logical level 0 and enables
 * it.
 */
#define GPIO_INT_LEVEL_INACTIVE        (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin logical level 1 and enables
 * it.
 */
#define GPIO_INT_LEVEL_ACTIVE          (GPIO_INT_ENABLE | \
					GPIO_INT_LEVELS_LOGICAL | \
					GPIO_INT_HIGH_1)

/** @} */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK		(GPIO_INPUT | GPIO_OUTPUT)
/** @endcond */

/**
 * @brief Identifies a set of pins associated with a port.
 *
 * The pin with index n is present in the set if and only if the bit
 * identified by (1U << n) is set.
 */
typedef uint32_t gpio_port_pins_t;

/**
 * @brief Provides values for a set of pins associated with a port.
 *
 * The value for a pin with index n is high (physical mode) or active
 * (logical mode) if and only if the bit identified by (1U << n) is set.
 * Otherwise the value for the pin is low (physical mode) or inactive
 * (logical mode).
 *
 * Values of this type are often paired with a `gpio_port_pins_t` value
 * that specifies which encoded pin values are valid for the operation.
 */
typedef uint32_t gpio_port_value_t;

/**
 * @brief Provides a type to hold a GPIO pin index.
 *
 * This reduced-size type is sufficient to record a pin number,
 * e.g. from a devicetree GPIOS property.
 */
typedef uint8_t gpio_pin_t;

/**
 * @brief Provides a type to hold GPIO devicetree flags.
 *
 * All GPIO flags that can be expressed in devicetree fit in the low 16
 * bits of the full flags field, so use a reduced-size type to record
 * that part of a GPIOS property.
 *
 * The lower 8 bits are used for standard flags. The upper 8 bits are reserved
 * for SoC specific flags.
 */
typedef uint16_t gpio_dt_flags_t;

/**
 * @brief Provides a type to hold GPIO configuration flags.
 *
 * This type is sufficient to hold all flags used to control GPIO
 * configuration, whether pin or interrupt.
 */
typedef uint32_t gpio_flags_t;

/**
 * @brief Container for GPIO pin information specified in devicetree
 *
 * This type contains a pointer to a GPIO device, pin number for a pin
 * controlled by that device, and the subset of pin configuration
 * flags which may be given in devicetree.
 *
 * @see GPIO_DT_SPEC_GET_BY_IDX
 * @see GPIO_DT_SPEC_GET_BY_IDX_OR
 * @see GPIO_DT_SPEC_GET
 * @see GPIO_DT_SPEC_GET_OR
 */
struct gpio_dt_spec {
	/** GPIO device controlling the pin */
	const struct device *port;
	/** The pin's number on the device */
	gpio_pin_t pin;
	/** The pin's configuration flags as specified in devicetree */
	gpio_dt_flags_t dt_flags;
};

/**
 * @brief Static initializer for a @p gpio_dt_spec
 *
 * This returns a static initializer for a @p gpio_dt_spec structure given a
 * devicetree node identifier, a property specifying a GPIO and an index.
 *
 * Example devicetree fragment:
 *
 *	n: node {
 *		foo-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>,
 *			    <&gpio1 2 GPIO_ACTIVE_LOW>;
 *	}
 *
 * Example usage:
 *
 *	const struct gpio_dt_spec spec = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n),
 *								 foo_gpios, 1);
 *	// Initializes 'spec' to:
 *	// {
 *	//         .port = DEVICE_DT_GET(DT_NODELABEL(gpio1)),
 *	//         .pin = 2,
 *	//         .dt_flags = GPIO_ACTIVE_LOW
 *	// }
 *
 * The 'gpio' field must still be checked for readiness, e.g. using
 * device_is_ready(). It is an error to use this macro unless the node
 * exists, has the given property, and that property specifies a GPIO
 * controller, pin number, and flags as shown above.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @return static initializer for a struct gpio_dt_spec for the property
 */
#define GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)			       \
	{								       \
		.port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)),\
		.pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),		       \
		.dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),	       \
	}

/**
 * @brief Like GPIO_DT_SPEC_GET_BY_IDX(), with a fallback to a default value
 *
 * If the devicetree node identifier 'node_id' refers to a node with a
 * property 'prop', and the index @p idx is valid for that property,
 * this expands to <tt>GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)</tt>. The @p
 * default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property,
 *         or default_value if the node or property do not exist
 */
#define GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, idx, default_value)	\
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, idx),		\
		    (GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)),	\
		    (default_value))

/**
 * @brief Equivalent to GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, 0).
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_GET(node_id, prop) \
	GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, 0)

/**
 * @brief Equivalent to
 *        GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, 0, default_value).
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX_OR()
 */
#define GPIO_DT_SPEC_GET_OR(node_id, prop, default_value) \
	GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, prop, 0, default_value)

/**
 * @brief Static initializer for a @p gpio_dt_spec from a DT_DRV_COMPAT
 * instance's GPIO property at an index.
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, idx) \
	GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Static initializer for a @p gpio_dt_spec from a DT_DRV_COMPAT
 *        instance's GPIO property at an index, with fallback
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param idx logical index into "prop"
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, idx, default_value)		\
	COND_CODE_1(DT_PROP_HAS_IDX(DT_DRV_INST(inst), prop, idx),		\
		    (GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(inst), prop, idx)),	\
		    (default_value))

/**
 * @brief Equivalent to GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, 0).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_INST_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET(inst, prop) \
	GPIO_DT_SPEC_INST_GET_BY_IDX(inst, prop, 0)

/**
 * @brief Equivalent to
 *        GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, 0, default_value).
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value fallback value to expand to
 * @return static initializer for a struct gpio_dt_spec for the property
 * @see GPIO_DT_SPEC_INST_GET_BY_IDX()
 */
#define GPIO_DT_SPEC_INST_GET_OR(inst, prop, default_value) \
	GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, prop, 0, default_value)

/*
 * @cond INTERNAL_HIDDEN
 */

/**
 * Auxiliary conditional macro that generates a bitmask for the range
 * from @p "prop" array defined by the (off_idx, sz_idx) pair,
 * or 0 if the range does not exist.
 *
 * @param node_id devicetree node identifier
 * @param prop lowercase-and-underscores array property name
 * @param off_idx logical index of bitmask offset value into "prop" array
 * @param sz_idx logical index of bitmask size value into "prop" array
 */
#define Z_GPIO_GEN_BITMASK_COND(node_id, prop, off_idx, sz_idx)		     \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, off_idx),		     \
		(COND_CODE_0(DT_PROP_BY_IDX(node_id, prop, sz_idx),	     \
			(0),						     \
			(GENMASK64(DT_PROP_BY_IDX(node_id, prop, off_idx) +  \
				DT_PROP_BY_IDX(node_id, prop, sz_idx) - 1,   \
				DT_PROP_BY_IDX(node_id, prop, off_idx))))    \
		), (0))

/**
 * A helper conditional macro returning generated bitmask for one element
 * from @p "gpio-reserved-ranges"
 *
 * @param odd_it the value of an odd sequential iterator
 * @param node_id devicetree node identifier
 */
#define Z_GPIO_GEN_RESERVED_RANGES_COND(odd_it, node_id)		      \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, gpio_reserved_ranges, odd_it),   \
		(Z_GPIO_GEN_BITMASK_COND(node_id,			      \
			gpio_reserved_ranges,				      \
			GET_ARG_N(odd_it, Z_SPARSE_LIST_EVEN_NUMBERS),	      \
			odd_it)),					      \
		(0))

/**
 * @endcond
 */

/**
 * @brief Makes a bitmask of reserved GPIOs from DT @p "gpio-reserved-ranges"
 *        property and @p "ngpios" argument
 *
 * This macro returns the value as a bitmask of the @p "gpio-reserved-ranges"
 * property. This property defines the disabled (or 'reserved') GPIOs in the
 * range @p 0...ngpios-1 and is specified as an array of value's pairs that
 * define the start offset and size of the reserved ranges.
 *
 * For example, setting "gpio-reserved-ranges = <3 2>, <10 1>;"
 * means that GPIO offsets 3, 4 and 10 cannot be used even if @p ngpios = <18>.
 *
 * The implementation constraint is inherited from common DT limitations:
 * a maximum of 64 pairs can be used (with result limited to bitsize
 * of gpio_port_pins_t type).
 *
 * NB: Due to the nature of C macros, some incorrect tuple definitions
 *    (for example, overlapping or out of range) will produce undefined results.
 *
 *    Also be aware that if @p ngpios is less than 32 (bit size of DT int type),
 *    then all unused MSBs outside the range defined by @p ngpios will be
 *    marked as reserved too.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	a {
 *		compatible = "some,gpio-controller";
 *		ngpios = <32>;
 *		gpio-reserved-ranges = <0  4>, <5  3>, <9  5>, <11 2>, <15 2>,
 *					<18 2>, <21 1>, <23 1>, <25 4>, <30 2>;
 *	};
 *
 *	b {
 *		compatible = "some,gpio-controller";
 *		ngpios = <18>;
 *		gpio-reserved-ranges = <3 2>, <10 1>;
 *	};
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t ngpios;
 *		uint32_t gpios_reserved;
 *	};
 *
 *	static const struct some_config dev_cfg_a = {
 *		.ngpios = DT_PROP_OR(DT_LABEL(a), ngpios, 0),
 *		.gpios_reserved = GPIO_DT_RESERVED_RANGES_NGPIOS(DT_LABEL(a),
 *					DT_PROP(DT_LABEL(a), ngpios)),
 *	};
 *
 *	static const struct some_config dev_cfg_b = {
 *		.ngpios = DT_PROP_OR(DT_LABEL(b), ngpios, 0),
 *		.gpios_reserved = GPIO_DT_RESERVED_RANGES_NGPIOS(DT_LABEL(b),
 *					DT_PROP(DT_LABEL(b), ngpios)),
 *	};
 *@endcode
 *
 * This expands to:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t ngpios;
 *		uint32_t gpios_reserved;
 *	};
 *
 *	static const struct some_config dev_cfg_a = {
 *		.ngpios = 32,
 *		.gpios_reserved = 0xdeadbeef,
 *		               // 0b1101 1110 1010 1101 1011 1110 1110 1111
 *	};
 *	static const struct some_config dev_cfg_b = {
 *		.ngpios = 18,
 *		.gpios_reserved = 0xfffc0418,
 *		               // 0b1111 1111 1111 1100 0000 0100 0001 1000
 *		               // unused MSBs were marked as reserved too
 *	};
 * @endcode
 *
 * @param node_id GPIO controller node identifier.
 * @param ngpios number of GPIOs.
 * @return the bitmask of reserved gpios
 */
#define GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, ngpios)			       \
	((gpio_port_pins_t)						       \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, gpio_reserved_ranges),	       \
		(GENMASK64(BITS_PER_LONG_LONG - 1, ngpios)		       \
		| FOR_EACH_FIXED_ARG(Z_GPIO_GEN_RESERVED_RANGES_COND,	       \
			(|),						       \
			node_id,					       \
			LIST_DROP_EMPTY(Z_SPARSE_LIST_ODD_NUMBERS))),	       \
		(0)))

/**
 * @brief Makes a bitmask of reserved GPIOs from the @p "gpio-reserved-ranges"
 *        and @p "ngpios" DT properties values
 *
 * @param node_id GPIO controller node identifier.
 * @return the bitmask of reserved gpios
 */
#define GPIO_DT_RESERVED_RANGES(node_id)				       \
	GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, DT_PROP(node_id, ngpios))

/**
 * @brief Makes a bitmask of reserved GPIOs from a DT_DRV_COMPAT instance's
 *        @p "gpio-reserved-ranges" property and @p "ngpios" argument
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return the bitmask of reserved gpios
 * @param ngpios  number of GPIOs
 * @see GPIO_DT_RESERVED_RANGES()
 */
#define GPIO_DT_INST_RESERVED_RANGES_NGPIOS(inst, ngpios)		       \
		GPIO_DT_RESERVED_RANGES_NGPIOS(DT_DRV_INST(inst), ngpios)

/**
 * @brief Make a bitmask of reserved GPIOs from a DT_DRV_COMPAT instance's GPIO
 *        @p "gpio-reserved-ranges" and @p "ngpios" properties
 *
 * @param inst DT_DRV_COMPAT instance number
 * @return the bitmask of reserved gpios
 * @see GPIO_DT_RESERVED_RANGES()
 */
#define GPIO_DT_INST_RESERVED_RANGES(inst)				       \
		GPIO_DT_RESERVED_RANGES(DT_DRV_INST(inst))

/**
 * @brief Makes a bitmask of allowed GPIOs from DT @p "gpio-reserved-ranges"
 *        property and @p "ngpios" argument
 *
 * This macro is paired with GPIO_DT_RESERVED_RANGES_NGPIOS(), however unlike
 * the latter, it returns a bitmask of ALLOWED gpios.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	a {
 *		compatible = "some,gpio-controller";
 *		ngpios = <32>;
 *		gpio-reserved-ranges = <0 8>, <9 5>, <15 16>;
 *	};
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t port_pin_mask;
 *	};
 *
 *	static const struct some_config dev_cfg = {
 *		.port_pin_mask = GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(
 *					DT_LABEL(a), 32),
 *	};
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *	struct some_config {
 *		uint32_t port_pin_mask;
 *	};
 *
 *	static const struct some_config dev_cfg = {
 *		.port_pin_mask = 0x80004100,
 *				// 0b1000 0000 0000 0000 0100 0001 00000 000
 *	};
 * @endcode
 *
 * @param node_id GPIO controller node identifier.
 * @param ngpios  number of GPIOs
 * @return the bitmask of allowed gpios
 */
#define GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(node_id, ngpios)		       \
	((gpio_port_pins_t)						       \
	COND_CODE_0(ngpios,						       \
		(0),							       \
		(COND_CODE_1(DT_NODE_HAS_PROP(node_id, gpio_reserved_ranges),  \
			((GENMASK64(ngpios - 1, 0) &			       \
			~GPIO_DT_RESERVED_RANGES_NGPIOS(node_id, ngpios))),    \
			(GENMASK64(ngpios - 1, 0)))			       \
		)							       \
	))

/**
 * @brief Makes a bitmask of allowed GPIOs from a DT_DRV_COMPAT instance's
 *        @p "gpio-reserved-ranges" property and @p "ngpios" argument
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param ngpios number of GPIOs
 * @return the bitmask of allowed gpios
 * @see GPIO_DT_NGPIOS_PORT_PIN_MASK_EXC()
 */
#define GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC(inst, ngpios)	\
		GPIO_DT_PORT_PIN_MASK_NGPIOS_EXC(DT_DRV_INST(inst), ngpios)

/**
 * @brief Maximum number of pins that are supported by `gpio_port_pins_t`.
 */
#define GPIO_MAX_PINS_PER_PORT (sizeof(gpio_port_pins_t) * __CHAR_BIT__)

/**
 * This structure is common to all GPIO drivers and is expected to be
 * the first element in the object pointed to by the config field
 * in the device structure.
 */
struct gpio_driver_config {
	/** Mask identifying pins supported by the controller.
	 *
	 * Initialization of this mask is the responsibility of device
	 * instance generation in the driver.
	 */
	gpio_port_pins_t port_pin_mask;
};

/**
 * This structure is common to all GPIO drivers and is expected to be the first
 * element in the driver's struct driver_data declaration.
 */
struct gpio_driver_data {
	/** Mask identifying pins that are configured as active low.
	 *
	 * Management of this mask is the responsibility of the
	 * wrapper functions in this header.
	 */
	gpio_port_pins_t invert;
};

struct gpio_callback;

/**
 * @typedef gpio_callback_handler_t
 * @brief Define the application callback handler function signature
 *
 * @param port Device struct for the GPIO device.
 * @param cb Original struct gpio_callback owning this handler
 * @param pins Mask of pins that triggers the callback handler
 *
 * Note: cb pointer can be used to retrieve private data through
 * CONTAINER_OF() if original struct gpio_callback is stored in
 * another private structure.
 */
typedef void (*gpio_callback_handler_t)(const struct device *port,
					struct gpio_callback *cb,
					gpio_port_pins_t pins);

/**
 * @brief GPIO callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct gpio_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct gpio_callback {
	/** This is meant to be used in the driver and the user should not
	 * mess with it (see drivers/gpio/gpio_utils.h)
	 */
	sys_snode_t node;

	/** Actual callback function being called when relevant. */
	gpio_c
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

/* excerpt lines 145-192 */
 * kernel during system initialization. This macro should only be used when the
 * device is not being allocated from a devicetree node. If you are allocating a
 * device from a devicetree node, use DEVICE_DT_DEINIT_DEFINE() or
 * DEVICE_DT_INST_DEINIT_DEFINE() instead.
 *
 * Note: deinit_fn will only be used if CONFIG_DEVICE_DEINIT_SUPPORT is enabled.
 *
 * @param dev_id A unique token which is used in the name of the global device
 * structure as a C identifier.
 * @param name A string name for the device, which will be stored in
 * @ref device.name. This name can be used to look up the device with
 * device_get_binding(). This must be less than Z_DEVICE_MAX_NAME_LEN characters
 * (including terminating `NULL`) in order to be looked up from user mode.
 * @param init_fn Pointer to the device's initialization function, which will be
 * run by the kernel during system initialization. Can be `NULL`.
 * @param deinit_fn Pointer to the device's de-initialization function. Can be
 * `NULL`. It must release any acquired resources (e.g. pins, bus, clock...) and
 * leave the device in its reset state.
 * @param pm Pointer to the device's power management resources, a
 * @ref pm_device, which will be stored in @ref device.pm field. Use `NULL` if
 * the device does not use PM.
 * @param data Pointer to the device's private mutable data, which will be
 * stored in @ref device.data.
 * @param config Pointer to the device's private constant data, which will be
 * stored in @ref device.config.
 * @param level The device's initialization level (PRE_KERNEL_1, PRE_KERNEL_2 or
 * POST_KERNEL).
 * @param prio The device's priority within its initialization level. See
 * SYS_INIT() for details.
 * @param api Pointer to the device's API structure. Can be `NULL`.
 */
#define DEVICE_DEINIT_DEFINE(dev_id, name, init_fn, deinit_fn, pm, data,       \
			     config, level, prio, api)                         \
	Z_DEVICE_STATE_DEFINE(dev_id);                                         \
	Z_DEVICE_DEFINE(DT_INVALID_NODE, dev_id, name, init_fn, deinit_fn, 0U, \
			pm, data, config, level, prio, api,                    \
			&Z_DEVICE_STATE_NAME(dev_id))

/**
 * @brief Create a device object and set it up for boot time initialization.
 *
 * @see DEVICE_DEINIT_DEFINE()
 */
#define DEVICE_DEFINE(dev_id, name, init_fn, pm, data, config, level, prio,    \
		      api)                                                     \
	DEVICE_DEINIT_DEFINE(dev_id, name, init_fn, NULL, pm, data, config,    \
			     level, prio, api)


/* excerpt lines 212-259 */
 * kernel during system initialization. The global device object's name as a C
 * identifier is derived from the node's dependency ordinal or hash.
 * @ref device.name is set to `DEVICE_DT_NAME(node_id)`.
 *
 * The device is declared with extern visibility, so a pointer to a global
 * device object can be obtained with `DEVICE_DT_GET(node_id)` from any source
 * file that includes `<zephyr/device.h>` (even from extensions, when
 * @kconfig{CONFIG_LLEXT_EXPORT_SYMBOL_GROUP_DEVICE} is enabled). Before using
 * the pointer, the referenced object should be checked using device_is_ready().
 *
 * Note: deinit_fn will only be used if CONFIG_DEVICE_DEINIT_SUPPORT is enabled.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Pointer to the device's initialization function, which will be
 * run by the kernel during system initialization. Can be `NULL`.
 * @param deinit_fn Pointer to the device's de-initialization function. Can be
 * `NULL`. It must release any acquired resources (e.g. pins, bus, clock...) and
 * leave the device in its reset state.
 * @param pm Pointer to the device's power management resources, a
 * @ref pm_device, which will be stored in @ref device.pm. Use `NULL` if the
 * device does not use PM.
 * @param data Pointer to the device's private mutable data, which will be
 * stored in @ref device.data.
 * @param config Pointer to the device's private constant data, which will be
 * stored in @ref device.config field.
 * @param level The device's initialization level (PRE_KERNEL_1, PRE_KERNEL_2 or
 * POST_KERNEL).
 * @param prio The device's priority within its initialization level. See
 * SYS_INIT() for details.
 * @param api Pointer to the device's API structure. Can be `NULL`.
 */
#define DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm, data, config, \
				level, prio, api, ...)                         \
	Z_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));                    \
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),                  \
			DEVICE_DT_NAME(node_id), init_fn, deinit_fn,           \
			Z_DEVICE_DT_FLAGS(node_id), pm, data, config, level,   \
			prio, api,                                             \
			&Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)),     \
			__VA_ARGS__)

/**
 * @brief Create a device object from a devicetree node identifier and set it up
 * for boot time initialization.
 *
 * @see DEVICE_DT_DEINIT_DEFINE()
 */
#define DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio, api, \

/* excerpt lines 896-943 */
 * A device whose initialization was deferred (by marking it as
 * ``zephyr,deferred-init`` on devicetree) needs to be initialized manually via
 * this call. De-initialized devices can also be initialized again via this
 * call.
 *
 * @param dev device to be initialized.
 *
 * @retval -EALREADY Device is already initialized.
 * @retval -errno For other errors.
 */
__syscall int device_init(const struct device *dev);

/**
 * @brief De-initialize a device.
 *
 * When a device is de-initialized, it will release any resources it has
 * acquired (e.g. pins, memory, clocks, DMA channels, etc.) and its status will
 * be left as in its reset state.
 *
 * Note: this will be available if CONFIG_DEVICE_DEINIT_SUPPORT is enabled.
 *
 * @warning It is the responsibility of the caller to ensure that the device is
 * ready to be de-initialized.
 *
 * @param dev device to be de-initialized.
 *
 * @retval 0 If successful
 * @retval -EPERM If device has not been initialized.
 * @retval -ENOTSUP If device does not support de-initialization, or if the
 *         feature is not enabled (see CONFIG_DEVICE_DEINIT_SUPPORT)
 * @retval -errno For any other errors.
 */
__syscall int device_deinit(const struct device *dev);

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Synthesize a unique name for the device state associated with
 * @p dev_id.
 */
#define Z_DEVICE_STATE_NAME(dev_id) _CONCAT(__devstate_, dev_id)

/**
 * @brief Utility macro to define and initialize the device state.

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

/* excerpt lines 1553-1633 */
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE(node_id, ph, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop)

/**
 * @brief Get a phandle-array specifier cell value at an index
 *
 * It might help to read the argument order as being similar to
 * `node->phandle_array[index].cell`. That is, the cell value is in
 * the @p pha property of @p node_id, inside the specifier at index
 * @p idx.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     gpio0: gpio@abcd1234 {
 *             #gpio-cells = <2>;
 *     };
 *
 *     gpio1: gpio@1234abcd {
 *             #gpio-cells = <2>;
 *     };
 *
 *     led: led_0 {
 *             gpios = <&gpio0 17 0x1>, <&gpio1 5 0x3>;
 *     };
 * @endcode
 *
 * Bindings fragment for the `gpio0` and `gpio1` nodes:
 *
 * @code{.yaml}
 *     gpio-cells:
 *       - pin
 *       - flags
 * @endcode
 *
 * Above, `gpios` has two elements:
 *
 * - index 0 has specifier <17 0x1>, so its `pin` cell is 17, and its
 *   `flags` cell is 0x1
 * - index 1 has specifier <5 0x3>, so `pin` is 5 and `flags` is 0x3
 *
 * Example usage:
 *
 * @code{.c}
 *     #define LED DT_NODELABEL(led)
 *
 *     DT_PHA_BY_IDX(LED, gpios, 0, pin)   // 17
 *     DT_PHA_BY_IDX(LED, gpios, 1, flags) // 0x3
 * @endcode
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into @p pha
 * @param cell lowercase-and-underscores cell name within the specifier
 *             at @p pha index @p idx
 * @return the cell's value
 */
#define DT_PHA_BY_IDX(node_id, pha, idx, cell) \
	DT_CAT7(node_id, _P_, pha, _IDX_, idx, _VAL_, cell)

/**
 * @brief Like DT_PHA_BY_IDX(), but with a fallback to @p default_value.
 *
 * If the value exists, this expands to DT_PHA_BY_IDX(node_id, pha,
 * idx, cell). The @p default_value parameter is not expanded in this
 * case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @internal
 * Implementation note: the _IDX_##idx##_VAL_##cell##_EXISTS macros are
 * defined, so it's safe to use DT_PROP_OR() here, because that uses an
 * IS_ENABLED() on the _EXISTS macro.
 * @endinternal
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into @p pha
 * @param cell lowercase-and-underscores cell name within the specifier

/* excerpt lines 2246-2293 */
/**
 * @brief Get the vendor at index @p idx as a string literal
 *
 * The vendor is a string extracted from vendor prefixes if an entry exists
 * that matches the node's compatible prefix. There may be as many as one
 * vendor prefixes file per directory in DTS_ROOT.
 *
 * Example vendor-prefixes.txt:
 *
 *	vnd	A stand-in for a real vendor
 *	zephyr	Zephyr-specific binding
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	n1: node-1 {
 *		compatible = "vnd,model1", "gpio", "zephyr,model2";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NODE_VENDOR_BY_IDX(DT_NODELABEL(n1), 0) // "A stand-in for a real vendor"
 *	DT_NODE_VENDOR_BY_IDX(DT_NODELABEL(n1), 2) // "Zephyr-specific binding"
 * @endcode
 *
 * Notice that the compatible at index 1 doesn't match any entries in the
 * vendor prefix file and therefore index 1 is not a valid vendor index. Use
 * DT_NODE_VENDOR_HAS_IDX(node_id, idx) to determine if an index is valid.
 *
 * @param node_id node identifier
 * @param idx index of the vendor to return
 * @return string literal of the idx-th vendor
 */
#define DT_NODE_VENDOR_BY_IDX(node_id, idx) \
	DT_CAT3(node_id, _COMPAT_VENDOR_IDX_, idx)

/**
 * @brief Does a node's compatible property have a vendor at an index?
 *
 * If this returns 1, then DT_NODE_VENDOR_BY_IDX(node_id, idx) is valid. If it
 * returns 0, it is an error to use DT_NODE_VENDOR_BY_IDX(node_id, idx) with
 * index @p idx.
 *
 * @param node_id node identifier
 * @param idx index of the vendor to check
 * @return 1 if @p idx is a valid vendor index,

/* excerpt lines 2325-2372 */
#define DT_NODE_VENDOR_OR(node_id, default_value) \
	DT_NODE_VENDOR_BY_IDX_OR(node_id, 0, default_value)

/**
 * @brief Get the model at index "idx" as a string literal
 *
 * The model is a string extracted from the compatible after the vendor prefix.
 *
 * Example vendor-prefixes.txt:
 *
 *	vnd	A stand-in for a real vendor
 *	zephyr	Zephyr-specific binding
 *
 * Example devicetree fragment:
 *
 *	n1: node-1 {
 *		compatible = "vnd,model1", "gpio", "zephyr,model2";
 *	};
 *
 * Example usage:
 *
 *	DT_NODE_MODEL_BY_IDX(DT_NODELABEL(n1), 0) // "model1"
 *	DT_NODE_MODEL_BY_IDX(DT_NODELABEL(n1), 2) // "model2"
 *
 * Notice that the compatible at index 1 doesn't match any entries in the
 * vendor prefix file and therefore index 1 is not a valid model index. Use
 * DT_NODE_MODEL_HAS_IDX(node_id, idx) to determine if an index is valid.
 *
 * @param node_id node identifier
 * @param idx index of the model to return
 * @return string literal of the idx-th model
 */
#define DT_NODE_MODEL_BY_IDX(node_id, idx) \
	DT_CAT3(node_id, _COMPAT_MODEL_IDX_, idx)

/**
 * @brief Does a node's compatible property have a model at an index?
 *
 * If this returns 1, then DT_NODE_MODEL_BY_IDX(node_id, idx) is valid. If it
 * returns 0, it is an error to use DT_NODE_MODEL_BY_IDX(node_id, idx) with
 * index "idx".
 *
 * @param node_id node identifier
 * @param idx index of the model to check
 * @return 1 if "idx" is a valid model index,
 *         0 otherwise.
 */
#define DT_NODE_MODEL_HAS_IDX(node_id, idx) \

/* excerpt lines 2744-2911 */
#define DT_IRQ_BY_NAME(node_id, name, cell) \
	DT_CAT5(node_id, _IRQ_NAME_, name, _VAL_, cell)

/**
 * @brief Get an interrupt specifier's value
 * Equivalent to DT_IRQ_BY_IDX(node_id, 0, cell).
 * @param node_id node identifier
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_IRQ(node_id, cell) DT_IRQ_BY_IDX(node_id, 0, cell)

/**
 * @brief Get an interrupt specifier's interrupt controller by index
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>, <2 2>;
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>, <&pic0 4>;
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>, <6>;
 *                     interrupt-names = "int1", "int2";
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(foo), 0) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(foo), 1) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(bar), 0) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(bar), 1) // &pic0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(qux), 0) // &pic0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(qux), 1) // &pic0
 *
 * @param node_id node identifier
 * @param idx interrupt specifier's index
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_IRQ_INTC_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _IRQ_IDX_, idx, _CONTROLLER)

/**
 * @brief Get an interrupt specifier's interrupt controller by name
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>, <2 2>;
 *             interrupt-names = "int1", "int2";
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>, <&pic0 4>;
 *             interrupt-names = "int1", "int2";
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>, <6>;
 *                     interrupt-names = "int1", "int2";
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(foo), int1) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(foo), int2) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(bar), int1) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(bar), int2) // &pic0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(qux), int1) // &pic0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(qux), int2) // &pic0
 *
 * @param node_id node identifier
 * @param name interrupt specifier's name
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_IRQ_INTC_BY_NAME(node_id, name) \
	DT_CAT4(node_id, _IRQ_NAME_, name, _CONTROLLER)

/**
 * @brief Get an interrupt specifier's interrupt controller
 * @note Equivalent to DT_IRQ_INTC_BY_IDX(node_id, 0)
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>;
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>;
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC(DT_NODELABEL(foo)) // &gpio0
 *     DT_IRQ_INTC(DT_NODELABEL(bar)) // &gpio0
 *     DT_IRQ_INTC(DT_NODELABEL(qux)) // &pic0
 *
 * @param node_id node identifier
 * @return node_id of interrupt specifier's interrupt controller
 * @see DT_IRQ_INTC_BY_IDX()
 */
#define DT_IRQ_INTC(node_id) \
	DT_IRQ_INTC_BY_IDX(node_id, 0)

/**
 * @cond INTERNAL_HIDDEN
 */

/* DT helper macro to encode a node's IRQN to level 1 according to the multi-level scheme */
#define DT_IRQN_L1_INTERNAL(node_id, idx) DT_IRQ_BY_IDX(node_id, idx, irq)
/* DT helper macro to encode a node's IRQN to level 2 according to the multi-level scheme */
#define DT_IRQN_L2_INTERNAL(node_id, idx)                                                          \
	(IRQ_TO_L2(DT_IRQN_L1_INTERNAL(node_id, idx)) |                                            \
	 DT_IRQ(DT_IRQ_INTC_BY_IDX(node_id, idx), irq))
/* DT helper macro to encode a node's IRQN to level 3 according to the multi-level scheme */
#define DT_IRQN_L3_INTERNAL(node_id, idx)                                                          \
	(IRQ_TO_L3(DT_IRQN_L1_INTERNAL(node_id, idx)) |                                            \
	 IRQ_TO_L2(DT_IRQ(DT_IRQ_INTC_BY_IDX(node_id, idx), irq)) |                                \
	 DT_IRQ(DT_IRQ_INTC(DT_IRQ_INTC_BY_IDX(node_id, idx)), irq))
/* DT helper macro for the macros above */
#define DT_IRQN_LVL_INTERNAL(node_id, idx, level) DT_CAT3(DT_IRQN_L, level, _INTERNAL)(node_id, idx)

/**
 * DT helper macro to encode a node's interrupt number according to the Zephyr's multi-level scheme
 * See doc/kernel/services/interrupts.rst for details
 */

/* excerpt lines 3365-3431 */
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @see DT_PROP_LEN
 */
#define DT_FOREACH_PROP_ELEM(node_id, prop, fn)		\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM)(fn)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop with
 *        separator.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             my-gpios = <&gpioa 0 GPIO_ACTICE_HIGH>,
 *                        <&gpiob 1 GPIO_ACTIVE_HIGH>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     struct gpio_dt_spec specs[] = {
 *             DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(n), my_gpios,
 *                                      GPIO_DT_SPEC_GET_BY_IDX, (,))
 *     };
 * @endcode
 *
 * This expands as a first step to:
 *
 * @code{.c}
 *     struct gpio_dt_spec specs[] = {
 *             GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), my_gpios, 0),
 *             GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), my_gpios, 1)
 *     };
 * @endcode
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_FOREACH_PROP_ELEM().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_PROP_ELEM
 */
#define DT_FOREACH_PROP_ELEM_SEP(node_id, prop, fn, sep) \
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM_SEP)(fn, sep)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop with
 * multiple arguments.
 *
 * The macro @p fn must take multiple parameters:
 * `fn(node_id, prop, idx, ...)`. @p node_id and @p prop are the same as what
 * is passed to DT_FOREACH_PROP_ELEM(), and @p idx is the current index into
 * the array. The @p idx values are integer literals starting from 0. The
 * remaining arguments are passed-in by the caller.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_FOREACH_PROP_ELEM().
 *
 * @param node_id node identifier

/* excerpt lines 5599-5646 */
 * in this case. The MACRO_MAP_CAT above is giving us the exact
 * tokens it wants prefixed with _S_.
 */
#define DT_S_PREFIX(name) _S_##name

/**
 * @brief Concatenation helper, 2 arguments
 *
 * This and the following macros are used to paste things together
 * with "##" *after* forcing expansion on each argument.
 *
 * We could try to use something like UTIL_CAT(), but the compiler
 * error messages from the util macros can be extremely long when they
 * are misused. This unfortunately happens often with devicetree.h,
 * since its macro-based API is fiddly and can be hard to get right.
 *
 * Keeping things brutally simple here hopefully makes some errors
 * easier to read.
 */
#define DT_CAT(a1, a2) a1 ## a2
/** @brief Concatenation helper, 3 arguments */
#define DT_CAT3(a1, a2, a3) a1 ## a2 ## a3
/** @brief Concatenation helper, 4 arguments */
#define DT_CAT4(a1, a2, a3, a4) a1 ## a2 ## a3 ## a4
/** @brief Internal concatenation helper, 5 arguments */
#define DT_CAT5(a1, a2, a3, a4, a5) a1 ## a2 ## a3 ## a4 ## a5
/** @brief Concatenation helper, 6 arguments */
#define DT_CAT6(a1, a2, a3, a4, a5, a6) a1 ## a2 ## a3 ## a4 ## a5 ## a6
/** @brief concatenation helper, 7 arguments */
#define DT_CAT7(a1, a2, a3, a4, a5, a6, a7) \
	a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7
/** @brief concatenation helper, 8 arguments */
#define DT_CAT8(a1, a2, a3, a4, a5, a6, a7, a8) \
	a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7 ## a8
/*
 * If you need to define a bigger DT_CATN(), do so here. Don't leave
 * any "holes" of undefined macros, please.
 */

/** @brief Helper for node identifier macros to expand args */
#define DT_DASH(...) MACRO_MAP_CAT(DT_DASH_PREFIX, __VA_ARGS__)
/** @brief Helper for DT_DASH(): prepends _ to a name */
#define DT_DASH_PREFIX(name) _##name
/** @brief Helper for DT_NODE_HAS_STATUS */
#define DT_NODE_HAS_STATUS_INTERNAL(node_id, status) \
	IS_ENABLED(DT_CAT3(node_id, _STATUS_, status))

/** @brief Helper macro to OR multiple has property checks in a loop macro

/* excerpt lines 5668-5702 */
#else
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


/* excerpt lines 382-480 */
 * @param stack Pointer to the thread stack.
 *
 * @retval 0 on success.
 * @retval -EBUSY if the thread stack is in use.
 * @retval -EINVAL if @p stack is invalid.
 * @retval -ENOSYS if dynamic thread stack allocation is disabled
 *
 * @see @kconfig{CONFIG_DYNAMIC_THREAD}
 */
__syscall int k_thread_stack_free(k_thread_stack_t *stack);

/**
 * @brief Create a thread.
 *
 * This routine initializes a thread, then schedules it for execution.
 *
 * The new thread may be scheduled for immediate execution or a delayed start.
 * If the newly spawned thread does not have a delayed start the kernel
 * scheduler may preempt the current thread to allow the new thread to
 * execute.
 *
 * Thread options are architecture-specific, and can include K_ESSENTIAL,
 * K_FP_REGS, and K_SSE_REGS. Multiple options may be specified by separating
 * them using "|" (the logical OR operator).
 *
 * Stack objects passed to this function may be statically allocated with
 * either of these macros in order to be portable:
 *
 * - K_THREAD_STACK_DEFINE() - For stacks that may support either user or
 *   supervisor threads.
 * - K_KERNEL_STACK_DEFINE() - For stacks that may support supervisor
 *   threads only. These stacks use less memory if CONFIG_USERSPACE is
 *   enabled.
 *
 * Alternatively, the stack may be dynamically allocated using
 * @ref k_thread_stack_alloc.
 *
 * The stack_size parameter has constraints. It must either be:
 *
 * - The original size value passed to K_THREAD_STACK_DEFINE() or
 *   K_KERNEL_STACK_DEFINE()
 * - The return value of K_THREAD_STACK_SIZEOF(stack) if the stack was
 *   defined with K_THREAD_STACK_DEFINE()
 * - The return value of K_KERNEL_STACK_SIZEOF(stack) if the stack was
 *   defined with K_KERNEL_STACK_DEFINE().
 *
 * Using other values, or sizeof(stack) may produce undefined behavior.
 *
 * @param new_thread Pointer to uninitialized struct k_thread
 * @param stack Pointer to the stack space.
 * @param stack_size Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 * @param delay Scheduling delay, or K_NO_WAIT (for no delay).
 *
 * @return ID of new thread.
 *
 */
__syscall k_tid_t k_thread_create(struct k_thread *new_thread,
				  k_thread_stack_t *stack,
				  size_t stack_size,
				  k_thread_entry_t entry,
				  void *p1, void *p2, void *p3,
				  int prio, uint32_t options, k_timeout_t delay);

/**
 * @brief Drop a thread's privileges permanently to user mode
 *
 * This allows a supervisor thread to be re-used as a user thread.
 * This function does not return, but control will transfer to the provided
 * entry point as if this was a new user thread.
 *
 * The implementation ensures that the stack buffer contents are erased.
 * Any thread-local storage will be reverted to a pristine state.
 *
 * Memory domain membership, resource pool assignment, kernel object
 * permissions, priority, and thread options are preserved.
 *
 * A common use of this function is to re-use the main thread as a user thread
 * once all supervisor mode-only tasks have been completed.
 *
 * @param entry Function to start executing from
 * @param p1 1st entry point parameter
 * @param p2 2nd entry point parameter
 * @param p3 3rd entry point parameter
 */
FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
						   void *p1, void *p2,
						   void *p3);

/**
 * @brief Grant a thread access to a set of kernel objects
 *
 * This is a convenience function. For the provided thread, grant access to
 * the remaining arguments, which must be pointers to kernel objects.

/* excerpt lines 640-801 */
/**
 * @brief Assign the system heap as a thread's resource pool
 *
 * Similar to k_thread_heap_assign(), but the thread will use
 * the kernel heap to draw memory.
 *
 * Use with caution, as a malicious thread could perform DoS attacks on the
 * kernel heap.
 *
 * @param thread Target thread to assign the system heap for resource requests
 *
 */
void k_thread_system_pool_assign(struct k_thread *thread);
#endif /* (K_HEAP_MEM_POOL_SIZE > 0) */

/**
 * @brief Sleep until a thread exits
 *
 * The caller will be put to sleep until the target thread exits, either due
 * to being aborted, self-exiting, or taking a fatal error. This API returns
 * immediately if the thread isn't running.
 *
 * This API may only be called from ISRs with a K_NO_WAIT timeout,
 * where it can be useful as a predicate to detect when a thread has
 * aborted.
 *
 * @param thread Thread to wait to exit
 * @param timeout upper bound time to wait for the thread to exit.
 * @retval 0 success, target thread has exited or wasn't running
 * @retval -EBUSY returned without waiting
 * @retval -EAGAIN waiting period timed out
 * @retval -EDEADLK target thread is joining on the caller, or target thread
 *                  is the caller
 */
__syscall int k_thread_join(struct k_thread *thread, k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration,
 * specified as a k_timeout_t object.
 *
 * @param timeout Desired duration of sleep.
 *
 * @return Zero if the requested time has elapsed or the time left to
 * sleep rounded up to the nearest millisecond (e.g. if the thread was
 * awoken by the \ref k_wakeup call).  Will be clamped to INT_MAX in
 * the case where the remaining time is unrepresentable in an int32_t.
 */
__syscall int32_t k_sleep(k_timeout_t timeout);

/**
 * @brief Put the current thread to sleep.
 *
 * This routine puts the current thread to sleep for @a duration milliseconds.
 *
 * @param ms Number of milliseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * millisecond.
 */
static inline int32_t k_msleep(int32_t ms)
{
	return k_sleep(Z_TIMEOUT_MS(ms));
}

/**
 * @brief Put the current thread to sleep with microsecond resolution.
 *
 * This function is unlikely to work as expected without kernel tuning.
 * In particular, because the lower bound on the duration of a sleep is
 * the duration of a tick, @kconfig{CONFIG_SYS_CLOCK_TICKS_PER_SEC} must be
 * adjusted to achieve the resolution desired. The implications of doing
 * this must be understood before attempting to use k_usleep(). Use with
 * caution.
 *
 * @param us Number of microseconds to sleep.
 *
 * @return Zero if the requested time has elapsed or if the thread was woken up
 * by the \ref k_wakeup call, the time left to sleep rounded up to the nearest
 * microsecond.
 */
__syscall int32_t k_usleep(int32_t us);

/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @note The clock used for the microsecond-resolution delay here may
 * be skewed relative to the clock used for system timeouts like
 * k_sleep().  For example k_busy_wait(1000) may take slightly more or
 * less time than k_sleep(K_MSEC(1)), with the offset dependent on
 * clock tolerances.
 *
 * @note In case when @kconfig{CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE} and
 * @kconfig{CONFIG_PM} options are enabled, this function may not work.
 * The timer/clock used for delay processing may be disabled/inactive.
 */
__syscall void k_busy_wait(uint32_t usec_to_wait);

/**
 * @brief Check whether it is possible to yield in the current context.
 *
 * This routine checks whether the kernel is in a state where it is possible to
 * yield or call blocking API's. It should be used by code that needs to yield
 * to perform correctly, but can feasibly be called from contexts where that
 * is not possible. For example in the PRE_KERNEL initialization step, or when
 * being run from the idle thread.
 *
 * @return True if it is possible to yield in the current context, false otherwise.
 */
bool k_can_yield(void);

/**
 * @brief Yield the current thread.
 *
 * This routine causes the current thread to yield execution to another
 * thread of the same or higher priority. If there are no other ready threads
 * of the same or higher priority, the routine returns immediately.
 */
__syscall void k_yield(void);

/**
 * @brief Wake up a sleeping thread.
 *
 * This routine prematurely wakes up @a thread from sleeping.
 *
 * If @a thread is not currently sleeping, the routine has no effect.
 *
 * @param thread ID of thread to wake.
 */
__syscall void k_wakeup(k_tid_t thread);

/**
 * @brief Query thread ID of the current thread.
 *
 * This unconditionally queries the kernel via a system call.
 *
 * @note Use k_current_get() unless absolutely sure this is necessary.
 *       This should only be used directly where the thread local
 *       variable cannot be used or may contain invalid values
 *       if thread local storage (TLS) is enabled. If TLS is not
 *       enabled, this is the same as k_current_get().
 *
 * @return ID of current thread.
 */
__attribute_const__
__syscall k_tid_t k_sched_current_thread_query(void);

/**
 * @brief Test whether startup is in the before-main-task phase.
 *
 * This routine allows the caller to customize its actions, depending on
 * whether it being invoked before the kernel is fully active.
 *
 * @isr_ok
 *
 * @return true if invoked before post-kernel initialization
 * @return false if invoked during/after post-kernel initialization

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/zephyr/doc/hardware/peripherals/gpio.rst`

```
.. _gpio_api:

General-Purpose Input/Output (GPIO)
###################################

Overview
********

A General-Purpose Input/Output (GPIO) is a digital signal pin that does not have a specific function
but can be controlled by software to function as an input or an output.

The GPIO API provides a generic method to interact with General Purpose Input/Output (GPIO)
pins. It allows applications to configure pins as inputs or outputs, read and write
their state, and manage interrupts. Key features include:

**Pin Configuration**
  Configure pins as input, output, or disconnected.
  Support for internal pull-up/pull-down resistors and drive strength configuration.

**Data Access**
  Read input values and write output values.

**Interrupts**
  Configure interrupts on pin state changes (rising edge, falling edge,
  level low, level high) and register callbacks to handle these interrupts.

**Devicetree Integration**
  GPIOs are typically defined in the Devicetree, allowing
  drivers and applications to reference them in a hardware-agnostic way using
  :c:struct:`gpio_dt_spec`.

Devicetree Configuration
************************

GPIO controllers are defined in the Devicetree as nodes with the ``gpio-controller`` property.
The ``#gpio-cells`` property typically specifies that 2 cells are used to describe a GPIO: the pin
number and the flags.

Example of a GPIO controller definition:

.. code-block:: devicetree

   gpio0: gpio@40022000 {
       compatible = "ti,cc13xx-cc26xx-gpio";
       reg = <0x40022000 0x400>;
       interrupts = <0 0>;
       gpio-controller;
       #gpio-cells = <2>;
   };

Example of referencing a GPIO:

.. code-block:: devicetree

   leds {
       compatible = "gpio-leds";
       led0: led_0 {
           gpios = <&gpio0 25 GPIO_ACTIVE_HIGH>;
           label = "Green LED";
       };
   };

Basic Operation
***************

GPIO operations are usually performed on a :c:struct:`gpio_dt_spec` structure, which is a container
for the GPIO pin information specified in the Devicetree.

This structure is typically populated using :c:macro:`GPIO_DT_SPEC_GET` macro (or any of its
variants).

.. code-block:: c
   :caption: Populating a gpio_dt_spec structure for a GPIO pin defined as alias ``led0``

   #define LED0_NODE DT_ALIAS(led0)
   static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

The :c:struct:`gpio_dt_spec` structure can then be used to perform GPIO operations.

.. code-block:: c
   :caption: Configure a GPIO pin (``led`` from the previous snippet) as output, initially inactive,
             and then set it to its active level.

   int ret;

   ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
   if (ret < 0) {
       return ret;
   }

   ret = gpio_pin_set_dt(&led, 1);
   if (ret < 0) {
       return ret;
   }

Refer to :zephyr:code-sample:`blinky` for a complete example of basic operations on GPIOs using the
:c:struct:`gpio_dt_spec` structure.

GPIO operations can also be performed directly on a GPIO controller device, in which case you will
use the GPIO API functions that take a device pointer as an argument. For example
:c:func:`gpio_pin_configure` instead of :c:func:`gpio_pin_configure_dt`.

GPIO Hogs
*********

GPIO hogs provide a mechanism to automatically configure GPIO pins during system initialization.
This is useful for pins that need to be set to a specific state (e.g., reset lines, power enables)
and don't require runtime control by an application.

Hogs are defined in the Devicetree as children of a GPIO controller node.

- The ``gpio-hog`` property marks the node as a hog.
- The ``gpios`` property specifies the pin and its active state.
- One of ``input``, ``output-low``, or ``output-high`` properties specifies the configuration.

Example Devicetree overlay:

.. code-block:: devicetree

   &gpio0 {
       hog1 {
           gpio-hog;
           gpios = <1 GPIO_ACTIVE_LOW>;
           output-high;
       };

       hog2 {
           gpio-hog;
           gpios = <2 GPIO_ACTIVE_HIGH>;
           output-low;
       };
   };

If you need runtime control of a pin configured as a hog beyond the initial state being
automatically set during system initialization, you may consider using the :ref:`regulator_api`
instead, with a :dtcompatible:`regulator-fixed` Devicetree node.

Configuration Options
*********************

Main configuration options:

* :kconfig:option:`CONFIG_GPIO`
* :kconfig:option:`CONFIG_GPIO_SHELL`
* :kconfig:option:`CONFIG_GPIO_GET_DIRECTION`
* :kconfig:option:`CONFIG_GPIO_GET_CONFIG`
* :kconfig:option:`CONFIG_GPIO_HOGS`
* :kconfig:option:`CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT`

API Reference
*************

.. doxygengroup:: gpio_interface
```
