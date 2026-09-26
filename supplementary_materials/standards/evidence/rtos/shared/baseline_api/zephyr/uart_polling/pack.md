# Raw RTOS/Bus Pack: zephyr / uart_polling

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/zephyr/include/zephyr/drivers/uart.h`

```c
/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup uart_interface
 * @brief Main header file for UART driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_H_

/**
 * @brief Interfaces for Universal Asynchronous Receiver/Transmitter (UART)
 *        controllers.
 * @defgroup uart_interface UART
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <stddef.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Line control signals. */
enum uart_line_ctrl {
	UART_LINE_CTRL_BAUD_RATE = BIT(0), /**< Baud rate */
	UART_LINE_CTRL_RTS = BIT(1),       /**< Request To Send (RTS) */
	UART_LINE_CTRL_DTR = BIT(2),       /**< Data Terminal Ready (DTR) */
	UART_LINE_CTRL_DCD = BIT(3),       /**< Data Carrier Detect (DCD) */
	UART_LINE_CTRL_DSR = BIT(4),       /**< Data Set Ready (DSR) */
};

/**
 * @brief Reception stop reasons.
 *
 * Values that correspond to events or errors responsible for stopping
 * receiving.
 */
enum uart_rx_stop_reason {
	/** @brief Overrun error */
	UART_ERROR_OVERRUN = (1 << 0),
	/** @brief Parity error */
	UART_ERROR_PARITY  = (1 << 1),
	/** @brief Framing error */
	UART_ERROR_FRAMING = (1 << 2),
	/**
	 * @brief Break interrupt
	 *
	 * A break interrupt was received. This happens when the serial input
	 * is held at a logic '0' state for longer than the sum of
	 * start time + data bits + parity + stop bits.
	 */
	UART_BREAK = (1 << 3),
	/**
	 * @brief Collision error
	 *
	 * This error is raised when transmitted data does not match
	 * received data. Typically this is useful in scenarios where
	 * the TX and RX lines maybe connected together such as
	 * RS-485 half-duplex. This error is only valid on UARTs that
	 * support collision checking.
	 */
	UART_ERROR_COLLISION = (1 << 4),
	/** @brief Noise error */
	UART_ERROR_NOISE = (1 << 5),
};

/** @brief Parity modes */
enum uart_config_parity {
	UART_CFG_PARITY_NONE,   /**< No parity */
	UART_CFG_PARITY_ODD,    /**< Odd parity */
	UART_CFG_PARITY_EVEN,   /**< Even parity */
	UART_CFG_PARITY_MARK,   /**< Mark parity */
	UART_CFG_PARITY_SPACE,  /**< Space parity */
};

/** @brief Number of stop bits. */
enum uart_config_stop_bits {
	UART_CFG_STOP_BITS_0_5,  /**< 0.5 stop bit */
	UART_CFG_STOP_BITS_1,    /**< 1 stop bit */
	UART_CFG_STOP_BITS_1_5,  /**< 1.5 stop bits */
	UART_CFG_STOP_BITS_2,    /**< 2 stop bits */
};

/** @brief Number of data bits. */
enum uart_config_data_bits {
	UART_CFG_DATA_BITS_5,    /**< 5 data bits */
	UART_CFG_DATA_BITS_6,    /**< 6 data bits */
	UART_CFG_DATA_BITS_7,    /**< 7 data bits */
	UART_CFG_DATA_BITS_8,    /**< 8 data bits */
	UART_CFG_DATA_BITS_9,    /**< 9 data bits */
};

/**
 * @brief Hardware flow control options.
 *
 * With flow control set to none, any operations related to flow control
 * signals can be managed by user with uart_line_ctrl functions.
 * In other cases, flow control is managed by hardware/driver.
 */
enum uart_config_flow_control {
	UART_CFG_FLOW_CTRL_NONE,     /**< No flow control */
	UART_CFG_FLOW_CTRL_RTS_CTS,  /**< RTS/CTS flow control */
	UART_CFG_FLOW_CTRL_DTR_DSR,  /**< DTR/DSR flow control */
	UART_CFG_FLOW_CTRL_RS485,    /**< RS485 flow control */
};

/**
 * @brief UART controller configuration structure
 */
struct uart_config {
	uint32_t baudrate;  /**< Baudrate setting in bps */
	uint8_t parity;     /**< Parity bit, use @ref uart_config_parity */
	uint8_t stop_bits;  /**< Stop bits, use @ref uart_config_stop_bits */
	uint8_t data_bits;  /**< Data bits, use @ref uart_config_data_bits */
	uint8_t flow_ctrl;  /**< Flow control setting, use @ref uart_config_flow_control */
};

/**
 * @defgroup uart_interrupt Interrupt-driven UART API
 * @{
 */

/**
 * @brief Define the application callback function signature for
 * uart_irq_callback_user_data_set() function.
 *
 * @param dev UART device instance.
 * @param user_data Arbitrary user data.
 */
typedef void (*uart_irq_callback_user_data_t)(const struct device *dev,
					      void *user_data);

/**
 * @}
 *
 * @defgroup uart_async Async UART API
 * @since 1.14
 * @version 1.0.0
 * @{
 */

/**
 * @brief Types of events passed to callback in UART_ASYNC_API
 *
 * Receiving:
 * 1. To start receiving, uart_rx_enable has to be called with first buffer
 * 2. When receiving starts to current buffer,
 *    #UART_RX_BUF_REQUEST will be generated, in response to that user can
 *    either:
 *
 *    - Provide second buffer using uart_rx_buf_rsp, when first buffer is
 *      filled, receiving will automatically start to second buffer.
 *    - Ignore the event, this way when current buffer is filled
 *      #UART_RX_RDY event will be generated and receiving will be stopped.
 *
 * 3. If some data was received and timeout occurred #UART_RX_RDY event will be
 *    generated. It can happen multiples times for the same buffer. RX timeout
 *    is counted from last byte received i.e. if no data was received, there
 *    won't be any timeout event.
 * 4. #UART_RX_BUF_RELEASED event will be generated when the current buffer is
 *    no longer used by the driver. It will immediately follow #UART_RX_RDY event.
 *    Depending on the implementation buffer may be released when it is completely
 *    or partially filled.
 * 5. If there was second buffer provided, it will become current buffer and
 *    we start again at point 2.
 *    If no second buffer was specified receiving is stopped and
 *    #UART_RX_DISABLED event is generated. After that whole process can be
 *    repeated.
 *
 * Any time during reception #UART_RX_STOPPED event can occur. if there is any
 * data received, #UART_RX_RDY event will be generated. It will be followed by
 * #UART_RX_BUF_RELEASED event for every buffer currently passed to driver and
 * finally by #UART_RX_DISABLED event.
 *
 * Receiving can be disabled using uart_rx_disable, after calling that
 * function, if there is any data received, #UART_RX_RDY event will be
 * generated. #UART_RX_BUF_RELEASED event will be generated for every buffer
 * currently passed to driver and finally #UART_RX_DISABLED event will occur.
 *
 * Transmitting:
 * 1. Transmitting starts by uart_tx function.
 * 2. If whole buffer was transmitted #UART_TX_DONE is generated. If timeout
 *    occurred #UART_TX_ABORTED will be generated.
 *
 * Transmitting can be aborted using @ref uart_tx_abort, after calling that
 * function #UART_TX_ABORTED event will be generated.
 *
 */
enum uart_event_type {
	/** @brief Whole TX buffer was transmitted. */
	UART_TX_DONE,
	/**
	 * @brief Transmitting aborted due to timeout or uart_tx_abort call
	 *
	 * When flow control is enabled, there is a possibility that TX transfer
	 * won't finish in the allotted time. Some data may have been
	 * transferred, information about it can be found in event data.
	 */
	UART_TX_ABORTED,
	/**
	 * @brief Received data is ready for processing.
	 *
	 * This event is generated in the following cases:
	 * - When RX timeout occurred, and data was stored in provided buffer.
	 *   This can happen multiple times in the same buffer.
	 * - When provided buffer is full.
	 * - After uart_rx_disable().
	 * - After stopping due to external event (#UART_RX_STOPPED).
	 */
	UART_RX_RDY,
	/**
	 * @brief Driver requests next buffer for continuous reception.
	 *
	 * This event is triggered when receiving has started for a new buffer,
	 * i.e. it's time to provide a next buffer for a seamless switchover to
	 * it. For continuous reliable receiving, user should provide another RX
	 * buffer in response to this event, using uart_rx_buf_rsp function
	 *
	 * If uart_rx_buf_rsp is not called before current buffer
	 * is filled up, receiving will stop.
	 */
	UART_RX_BUF_REQUEST,
	/**
	 * @brief Buffer is no longer used by UART driver.
	 */
	UART_RX_BUF_RELEASED,
	/**
	 * @brief RX has been disabled and can be reenabled.
	 *
	 * This event is generated whenever receiver has been stopped, disabled
	 * or finished its operation and can be enabled again using
	 * uart_rx_enable
	 */
	UART_RX_DISABLED,
	/**
	 * @brief RX has stopped due to external event.
	 *
	 * Reason is one of uart_rx_stop_reason.
	 */
	UART_RX_STOPPED,
};

/** @brief UART TX event data. */
struct uart_event_tx {
	/** @brief Pointer to current buffer. */
	const uint8_t *buf;
	/** @brief Number of bytes sent. */
	size_t len;
};

/**
 * @brief UART RX event data.
 *
 * The data represented by the event is stored in rx.buf[rx.offset] to
 * rx.buf[rx.offset+rx.len].  That is, the length is relative to the offset.
 */
struct uart_event_rx {
	/** @brief Pointer to current buffer. */
	uint8_t *buf;
	/** @brief Currently received data offset in bytes. */
	size_t offset;
	/** @brief Number of new bytes received. */
	size_t len;
};

/** @brief UART RX buffer released event data. */
struct uart_event_rx_buf {
	/** @brief Pointer to buffer that is no longer in use. */
	uint8_t *buf;
};

/** @brief UART RX stopped data. */
struct uart_event_rx_stop {
	/** @brief Reason why receiving stopped */
	enum uart_rx_stop_reason reason;
	/** @brief Last received data. */
	struct uart_event_rx data;
};

/** @brief Structure containing information about current event. */
struct uart_event {
	/** @brief Type of event */
	enum uart_event_type type;
	/** @brief Event data */
	union uart_event_data {
		/** @brief #UART_TX_DONE and #UART_TX_ABORTED events data. */
		struct uart_event_tx tx;
		/** @brief #UART_RX_RDY event data. */
		struct uart_event_rx rx;
		/** @brief #UART_RX_BUF_RELEASED event data. */
		struct uart_event_rx_buf rx_buf;
		/** @brief #UART_RX_STOPPED event data. */
		struct uart_event_rx_stop rx_stop;
	} data;
};

/**
 * @typedef uart_callback_t
 * @brief Define the application callback function signature for
 * uart_callback_set() function.
 *
 * @param dev UART device instance.
 * @param evt Pointer to uart_event instance.
 * @param user_data Pointer to data specified by user.
 */
typedef void (*uart_callback_t)(const struct device *dev,
				struct uart_event *evt, void *user_data);

/**
 * @}
 */

/**
 * @brief Check whether an error was detected.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If no error was detected.
 * @retval err Error flags as defined in @ref uart_rx_stop_reason
 * @retval -ENOSYS If not implemented.
 */
__syscall int uart_err_check(const struct device *dev);

/**
 * @defgroup uart_polling Polling UART API
 * @{
 */

/**
 * @brief Read a character from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a character from the device,
 * stores to the location pointed to by p_char, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived.
 * @retval -1 If no character was available to read (i.e. the UART
 *            input buffer was empty).
 * @retval -ENOSYS If the operation is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
__syscall int uart_poll_in(const struct device *dev, unsigned char *p_char);

/**
 * @brief Read a 16-bit datum from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a 16-bit datum from the device,
 * stores to the location pointed to by p_u16, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_u16 Pointer to 16-bit data.
 *
 * @retval 0  If data arrived.
 * @retval -1 If no data was available to read (i.e., the UART
 *            input buffer was empty).
 * @retval -ENOTSUP If API is not enabled.
 * @retval -ENOSYS If the function is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
__syscall int uart_poll_in_u16(const struct device *dev, uint16_t *p_u16);

/**
 * @brief Write a character to the device for output.
 *
 * This routine checks if the transmitter is full. When the
 * transmitter is not full, it writes a character to the data
 * register. It waits and blocks the calling thread otherwise. This
 * function is a blocking call. It blocks the calling thread until the
 * character is sent.
 *
 * To send a character when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_char Character to send.
 */
__syscall void uart_poll_out(const struct device *dev,
			     unsigned char out_char);

/**
 * @brief Write a 16-bit datum to the device for output.
 *
 * This routine checks if the transmitter is full. When the
 * transmitter is not full, it writes a 16-bit datum to the data
 * register. It waits and blocks the calling thread, otherwise. This
 * function is a blocking call.
 *
 * To send a datum when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_u16 Wide data to send.
 */
__syscall void uart_poll_out_u16(const struct device *dev, uint16_t out_u16);

/**
 * @}
 */

/**
 * @brief Set UART configuration.
 *
 * Sets UART configuration using data from *cfg.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If configuration is not supported by device
 *                  or driver does not support setting configuration in runtime.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_configure(const struct device *dev,
			     const struct uart_config *cfg);

/**
 * @brief Get UART configuration.
 *
 * Stores current UART configuration to *cfg, can be used to retrieve initial
 * configuration after device was initialized using data from DTS.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If driver does not support getting current configuration.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_config_get(const struct device *dev,
			      struct uart_config *cfg);

/**
 * @addtogroup uart_interrupt
 * @{
 */

/**
 * @brief Fill FIFO with data.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_tx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). Likewise, *not* calling this function
 * from an ISR if uart_irq_tx_ready() returns true may lead to
 * undefined behavior, e.g. infinite interrupt loops. It's
 * mandatory to test return value of this function, as different
 * hardware has different FIFO depth (oftentimes just 1).
 *
 * @param dev UART device instance.
 * @param tx_data Data to transmit.
 * @param size Number of bytes to send.
 *
 * @return Number of bytes sent.
 * @retval -ENOSYS  if this function is not supported
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size);

/**
 * @brief Fill FIFO with wide data.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_tx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). Likewise, *not* calling this function
 * from an ISR if uart_irq_tx_ready() returns true may lead to
 * undefined behavior, e.g. infinite interrupt loops. It's
 * mandatory to test return value of this function, as different
 * hardware has different FIFO depth (oftentimes just 1).
 *
 * @param dev UART device instance.
 * @param tx_data Wide data to transmit.
 * @param size Number of datum to send.
 *
 * @return Number of datum sent.
 * @retval -ENOSYS If this function is not implemented
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill_u16(const struct device *dev, const uint16_t *tx_data, int size);

/**
 * @brief Read data from FIFO.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_rx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). It's unspecified whether "RX ready"
 * condition as returned by uart_irq_rx_ready() is level- or
 * edge- triggered. That means that once uart_irq_rx_ready() is
 * detected, uart_fifo_read() must be called until it reads all
 * available data in the FIFO (i.e. until it returns less data
 * than was requested).
 *
 * @param dev UART device instance.
 * @param rx_data Data container.
 * @param size Container size.
 *
 * @return Number of bytes read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size);

/**
 * @brief Read wide data from FIFO.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_rx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). It's unspecified whether "RX ready"
 * condition as returned by uart_irq_rx_ready() is level- or
 * edge- triggered. That means that once uart_irq_rx_ready() is
 * detected, uart_fifo_read() must be called until it reads all
 * available data in the FIFO (i.e. until it returns less data
 * than was requested).
 *
 * @param dev UART device instance.
 * @param rx_data Wide data container.
 * @param size Container size.
 *
 * @return Number of datum read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read_u16(const struct device *dev, uint16_t *rx_data, const int size);

/**
 * @brief Enable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_tx_enable(const struct device *dev);

/**
 * @brief Disable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_tx_disable(const struct device *dev);

/**
 * @brief Check if UART TX buffer can accept bytes
 *
 * @details Check if UART TX buffer can accept more bytes
 * for transmission (i.e. uart_fifo_fill() will succeed and return
 * non-zero). This function must be called in a UART interrupt
 * handler, or its result is undefined. Before calling this function
 * in the interrupt handler, uart_irq_update() must be called once per
 * the handler invocation.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If device is not ready to write a new byte.
 * @retval >0 Minimum number of bytes that can be written in a single call to
 *            @ref uart_fifo_fill. It may be possible to write more bytes, but
 *            the actual number written must be checked in the return code from
 *            @ref uart_fifo_fill.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_ready(const struct device *dev);

/**
 * @brief Enable RX interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_rx_enable(const struct device *dev);

/**
 * @brief Disable RX interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_rx_disable(const struct device *dev);

/**
 * @brief Check if UART TX block finished transmission
 *
 * @details Check if any outgoing data buffered in UART TX block was
 * fully transmitted and TX block is idle. When this condition is
 * true, UART device (or whole system) can be power off. Note that
 * this function is *not* useful to check if UART TX can accept more
 * data, use uart_irq_tx_ready() for that. This function must be called
 * in a UART interrupt handler, or its result is undefined. Before
 * calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If nothing remains to be transmitted.
 * @retval 0 If transmission is not completed.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_complete(const struct device *dev);

/**
 * @brief Check if UART RX buffer has a received char
 *
 * @details Check if UART RX buffer has at least one pending character
 * (i.e. uart_fifo_read() will succeed and return non-zero). This function
 * must be called in a UART interrupt handler, or its result is undefined.
 * Before calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation. It's unspecified whether
 * condition as returned by this function is level- or edge- triggered (i.e.
 * if this function returns true when RX FIFO is non-empty, or when a new
 * char was received since last call to it). See description of
 * uart_fifo_read() for implication of this.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If a received char is ready.
 * @retval 0 If a received char is not ready.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_rx_ready(const struct device *dev);

/**
 * @brief Enable error interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_err_enable(const struct device *dev);

/**
 * @brief Disable error interrupt.
 *
 * @param dev UART device instance.
 */
__syscall void uart_irq_err_disable(const struct device *dev);

/**
 * @brief Check if any IRQs is pending.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If an IRQ is pending.
 * @retval 0 If an IRQ is not pending.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_irq_is_pending(const struct device *dev);

/**
 * @brief Start processing interrupts in ISR.
 *
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 *
 * The purpose of this function is:
 *
 * * For devices with auto-acknowledge of interrupt status on register
 *   read to cache the value of this register (rx_ready, etc. then use
 *   this case).
 * * For devices with explicit acknowledgment of interrupts, to ack
 *   any pending interrupts and likewise to cache the original value.
 * * For devices with implicit acknowledgment, this function will be
 *   empty. But the ISR must perform the actions needs to ack the
 *   interrupts (usually, call uart_fifo_read() on rx_ready, and
 *   uart_fifo_fill() on tx_ready).
 *
 * @param dev UART device instance.
 *
 * @retval 1 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
__syscall int uart_irq_update(const struct device *dev);

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 * See description of uart_irq_update() for the requirements on ISR.
 *
 * @param dev UART device instance.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_callback_user_data_set(const struct device *dev,
						  uart_irq_callback_user_data_t cb,
						  void *user_data);

/**
 * @brief Set the IRQ callback function pointer (legacy).
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with the device point
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

/* excerpt lines 91-262 */
/**
 * @brief Get a node identifier for a devicetree path
 *
 * @note This macro returns a node identifier from path components. To get
 *       a path string from a node identifier, use DT_NODE_PATH() instead.
 *
 * The arguments to this macro are the names of non-root nodes in the
 * tree required to reach the desired node, starting from the root.
 * Non-alphanumeric characters in each name must be converted to
 * underscores to form valid C tokens, and letters must be lowercased.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * You can use `DT_PATH(soc, serial_40001000)` to get a node identifier
 * for the `serial@40001000` node. Node labels like `serial1` cannot be
 * used as DT_PATH() arguments; use DT_NODELABEL() for those instead.
 *
 * Example usage with DT_PROP() to get the `current-speed` property:
 *
 * @code{.c}
 *     DT_PROP(DT_PATH(soc, serial_40001000), current_speed) // 115200
 * @endcode
 *
 * (The `current-speed` property is also in `lowercase-and-underscores`
 * form when used with this API.)
 *
 * When determining arguments to DT_PATH():
 *
 * - the first argument corresponds to a child node of the root (`soc` above)
 * - a second argument corresponds to a child of the first argument
 *   (`serial_40001000` above, from the node name `serial@40001000`
 *   after lowercasing and changing `@` to `_`)
 * - and so on for deeper nodes in the desired node's path
 *
 * @param ... lowercase-and-underscores node names along the node's path,
 *            with each name given as a separate argument
 * @return node identifier for the node with that path
 */
#define DT_PATH(...) DT_PATH_INTERNAL(__VA_ARGS__)

/**
 * @brief Get a node identifier for a node label
 *
 * Convert non-alphanumeric characters in the node label to
 * underscores to form valid C tokens, and lowercase all letters. Note
 * that node labels are not the same thing as label properties.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     serial1: serial@40001000 {
 *             label = "UART_0";
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 * @endcode
 *
 * The only node label in this example is `serial1`.
 *
 * The string `UART_0` is *not* a node label; it's the value of a
 * property named label.
 *
 * You can use `DT_NODELABEL(serial1)` to get a node identifier for the
 * `serial@40001000` node. Example usage with DT_PROP() to get the
 * current-speed property:
 *
 * @code{.c}
 *     DT_PROP(DT_NODELABEL(serial1), current_speed) // 115200
 * @endcode
 *
 * Another example devicetree fragment:
 *
 * @code{.dts}
 *     cpu@0 {
 *            L2_0: l2-cache {
 *                    cache-level = <2>;
 *                    ...
 *            };
 *     };
 * @endcode
 *
 * Example usage to get the cache-level property:
 *
 * @code{.c}
 *     DT_PROP(DT_NODELABEL(l2_0), cache_level) // 2
 * @endcode
 *
 * Notice how `L2_0` in the devicetree is lowercased to `l2_0` in the
 * DT_NODELABEL() argument.
 *
 * @param label lowercase-and-underscores node label name
 * @return node identifier for the node with that label
 */
#define DT_NODELABEL(label) DT_CAT(DT_N_NODELABEL_, label)

/**
 * @brief Get a node identifier from /aliases
 *
 * This macro's argument is a property of the `/aliases` node. It
 * returns a node identifier for the node which is aliased. Convert
 * non-alphanumeric characters in the alias property to underscores to
 * form valid C tokens, and lowercase all letters.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             aliases {
 *                     my-serial = &serial1;
 *             };
 *
 *             soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * You can use DT_ALIAS(my_serial) to get a node identifier for the
 * `serial@40001000` node. Notice how `my-serial` in the devicetree
 * becomes `my_serial` in the DT_ALIAS() argument. Example usage with
 * DT_PROP() to get the current-speed property:
 *
 * @code{.c}
 *     DT_PROP(DT_ALIAS(my_serial), current_speed) // 115200
 * @endcode
 *
 * @param alias lowercase-and-underscores alias name.
 * @return node identifier for the node with that alias
 */
#define DT_ALIAS(alias) DT_CAT(DT_N_ALIAS_, alias)

/**
 * @brief Test if the devicetree has a given alias
 * @param alias_name lowercase-and-underscores devicetree alias name
 * @return 1 if the alias exists and refers to a node, 0 otherwise
 */
#define DT_HAS_ALIAS(alias_name) DT_NODE_EXISTS(DT_ALIAS(alias_name))

/**
 * @brief Get the hash associated with a DT node
 *
 * Get the hash for the specified node_id. The hash is calculated on the
 * full devicetree path of the node.
 * @param node_id node identifier
 * @return hash value as a preprocessor token
 */
#define DT_NODE_HASH(node_id) DT_CAT(node_id, _HASH)

/**
 * @brief Get a node identifier for an instance of a compatible
 *
 * All nodes with a particular compatible property value are assigned
 * instance numbers, which are zero-based indexes specific to that
 * compatible. You can get a node identifier for these nodes by
 * passing DT_INST() an instance number, @p inst, along with the

/* excerpt lines 271-369 */
 *   instance numbers starting from 0, and disabled nodes have instance
 *   numbers which are greater than those of any enabled node
 *
 * No other guarantees are made. In particular:
 *
 * - instance numbers **in no way reflect** any numbering scheme that
 *   might exist in SoC documentation, node labels or unit addresses,
 *   or properties of the /aliases node (use DT_NODELABEL() or DT_ALIAS()
 *   for those)
 * - there **is no general guarantee** that the same node will have
 *   the same instance number between builds, even if you are building
 *   the same application again in the same build directory
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     serial1: serial@40001000 {
 *             compatible = "vnd,soc-serial";
 *             status = "disabled";
 *             current-speed = <9600>;
 *             ...
 *     };
 *
 *     serial2: serial@40002000 {
 *             compatible = "vnd,soc-serial";
 *             status = "okay";
 *             current-speed = <57600>;
 *             ...
 *     };
 *
 *     serial3: serial@40003000 {
 *             compatible = "vnd,soc-serial";
 *             current-speed = <115200>;
 *             ...
 *     };
 * @endcode
 *
 * Assuming no other nodes in the devicetree have compatible
 * `"vnd,soc-serial"`, that compatible has nodes with instance numbers
 * 0, 1, and 2.
 *
 * The nodes `serial@40002000` and `serial@40003000` are both enabled, so
 * their instance numbers are 0 and 1, but no guarantees are made
 * regarding which node has which instance number.
 *
 * Since `serial@40001000` is the only disabled node, it has instance
 * number 2, since disabled nodes are assigned the largest instance
 * numbers. Therefore:
 *
 * @code{.c}
 *     // Could be 57600 or 115200. There is no way to be sure:
 *     // either serial@40002000 or serial@40003000 could
 *     // have instance number 0, so this could be the current-speed
 *     // property of either of those nodes.
 *     DT_PROP(DT_INST(0, vnd_soc_serial), current_speed)
 *
 *     // Could be 57600 or 115200, for the same reason.
 *     // If the above expression expands to 57600, then
 *     // this expands to 115200, and vice-versa.
 *     DT_PROP(DT_INST(1, vnd_soc_serial), current_speed)
 *
 *     // 9600, because there is only one disabled node, and
 *     // disabled nodes are "at the end" of the instance
 *     // number "list".
 *     DT_PROP(DT_INST(2, vnd_soc_serial), current_speed)
 * @endcode
 *
 * Notice how `"vnd,soc-serial"` in the devicetree becomes `vnd_soc_serial`
 * (without quotes) in the DT_INST() arguments. (As usual, `current-speed`
 * in the devicetree becomes `current_speed` as well.)
 *
 * Nodes whose `compatible` property has multiple values are assigned
 * independent instance numbers for each compatible.
 *
 * @param inst instance number for compatible @p compat
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return node identifier for the node with that instance number and
 *         compatible
 */
#define DT_INST(inst, compat) UTIL_CAT(DT_N_INST, DT_DASH(inst, compat))

/**
 * @brief Get a node identifier for a parent node
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     parent: parent-node {
 *             child: child-node {
 *                     ...
 *             };
 *     };
 * @endcode
 *
 * The following are equivalent ways to get the same node identifier:
 *
 * @code{.c}
 *     DT_NODELABEL(parent)
 *     DT_PARENT(DT_NODELABEL(child))

/* excerpt lines 394-490 */
 *     DT_PARENT(DT_PARENT(DT_NODELABEL(child))
 * @endcode
 *
 * @param node_id node identifier
 * @return a node identifier for the node's parent's parent
 */
#define DT_GPARENT(node_id) DT_PARENT(DT_PARENT(node_id))

/**
 * @brief Get a node identifier for a child node
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc-label: soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage with DT_PROP() to get the status of the
 * `serial@40001000` node:
 *
 * @code{.c}
 *     #define SOC_NODE DT_NODELABEL(soc_label)
 *     DT_PROP(DT_CHILD(SOC_NODE, serial_40001000), status) // "okay"
 * @endcode
 *
 * Node labels like `serial1` cannot be used as the @p child argument
 * to this macro. Use DT_NODELABEL() for that instead.
 *
 * You can also use DT_FOREACH_CHILD() to iterate over node
 * identifiers for all of a node's children.
 *
 * @param node_id node identifier
 * @param child lowercase-and-underscores child node name
 * @return node identifier for the node with the name referred to by 'child'
 */
#define DT_CHILD(node_id, child) UTIL_CAT(node_id, DT_S_PREFIX(child))

/**
 * @brief Get a node identifier for a child node with a matching unit address
 *
 * @note Only works for children with unique integer unit addresses.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc-label: soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage with DT_PROP() to get the status of the
 * `serial@40001000` node:
 *
 * @code{.c}
 *     #define SOC_NODE DT_NODELABEL(soc_label)
 *     DT_PROP(DT_CHILD_BY_UNIT_ADDR_INT(SOC_NODE, 1073745920), status) // "okay"
 * @endcode
 *
 * @param node_id node identifier
 * @param addr Integer unit address for the child node.
 *
 * @return node identifier for the child node with the specified unit address
 */
#define DT_CHILD_BY_UNIT_ADDR_INT(node_id, addr) \
	DT_CAT3(node_id, _CHILD_UNIT_ADDR_INT_, addr)

/**
 * @brief Get a node identifier for a status `okay` node with a compatible
 *
 * Use this if you want to get an arbitrary enabled node with a given
 * compatible, and you do not care which one you get. If any enabled
 * nodes with the given compatible exist, a node identifier for one
 * of them is returned. Otherwise, @ref DT_INVALID_NODE is returned.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	node-a {
 *		compatible = "vnd,device";
 *		status = "okay";
 *	};
 *
 *	node-b {

/* excerpt lines 2687-2750 */
 */
#define DT_IRQ_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT4(node_id, _IRQ_NAME_, name, _VAL_irq_EXISTS))

/**
 * @brief Get a value within an interrupt specifier at an index
 *
 * It might help to read the argument order as being similar to
 * "node->interrupts[index].cell".
 *
 * This can be used to get information about an individual interrupt
 * when a device generates more than one.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     my-serial: serial@abcd1234 {
 *             interrupts = < 33 0 >, < 34 1 >;
 *     };
 * @endcode
 *
 * Assuming the node's interrupt domain has "#interrupt-cells = <2>;" and
 * the individual cells in each interrupt specifier are named "irq" and
 * "priority" by the node's binding, here are some examples:
 *
 *     #define SERIAL DT_NODELABEL(my_serial)
 *
 *     Example usage                       Value
 *     -------------                       -----
 *     DT_IRQ_BY_IDX(SERIAL, 0, irq)          33
 *     DT_IRQ_BY_IDX(SERIAL, 0, priority)      0
 *     DT_IRQ_BY_IDX(SERIAL, 1, irq,          34
 *     DT_IRQ_BY_IDX(SERIAL, 1, priority)      1
 *
 * @param node_id node identifier
 * @param idx logical index into the interrupt specifier array
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_IRQ_BY_IDX(node_id, idx, cell)   \
	DT_CAT5(node_id, _IRQ_IDX_, idx, _VAL_, cell)

/**
 * @brief Get a value within an interrupt specifier by name
 *
 * It might help to read the argument order as being similar to
 * `node->interrupts.name.cell`.
 *
 * This can be used to get information about an individual interrupt
 * when a device generates more than one, if the bindings give each
 * interrupt specifier a name.
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores interrupt specifier name
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_IRQ_BY_NAME(node_id, name, cell) \
	DT_CAT5(node_id, _IRQ_NAME_, name, _VAL_, cell)

/**
 * @brief Get an interrupt specifier's value
 * Equivalent to DT_IRQ_BY_IDX(node_id, 0, cell).
 * @param node_id node identifier

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

## Source: `data/rtos/zephyr/doc/hardware/peripherals/uart.rst`

```
.. _uart_api:

Universal Asynchronous Receiver-Transmitter (UART)
##################################################

Overview
********

Zephyr provides three different ways to access the UART peripheral. Depending
on the method, different API functions are used according to below sections:

1. :ref:`uart_polling_api`
2. :ref:`uart_interrupt_api`
3. :ref:`uart_async_api` using :ref:`dma_api`

Polling is the most basic method to access the UART peripheral. The reading
function, :c:func:`uart_poll_in`, is a non-blocking function and returns a character
or ``-1`` when no valid data is available. The writing function,
:c:func:`uart_poll_out`, is a blocking function and the thread waits until the given
character is sent.

With the Interrupt-driven API, possibly slow communication can happen in the
background while the thread continues with other tasks. The Kernel's
:ref:`kernel_data_passing_api` features can be used to communicate between
the thread and the UART driver.

The Asynchronous API allows to read and write data in the background using DMA
without interrupting the MCU at all. However, the setup is more complex
than the other methods.

.. warning::

   Interrupt-driven API and the Asynchronous API should NOT be used at
   the same time for the same hardware peripheral, since both APIs require
   hardware interrupts to function properly. Using the callbacks for both
   APIs would result in interference between each other.
   :kconfig:option:`CONFIG_UART_EXCLUSIVE_API_CALLBACKS` is enabled by default
   so that only the callbacks associated with one API is active at a time.


Configuration Options
*********************

Most importantly, the Kconfig options define whether the polling API (default),
the interrupt-driven API or the asynchronous API can be used. Only enable the
features you need in order to minimize memory footprint.

Related configuration options:

* :kconfig:option:`CONFIG_SERIAL`
* :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`
* :kconfig:option:`CONFIG_UART_ASYNC_API`
* :kconfig:option:`CONFIG_UART_WIDE_DATA`
* :kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE`
* :kconfig:option:`CONFIG_UART_LINE_CTRL`
* :kconfig:option:`CONFIG_UART_DRV_CMD`


API Reference
*************

.. doxygengroup:: uart_interface


.. _uart_polling_api:

Polling API
===========

.. doxygengroup:: uart_polling


.. _uart_interrupt_api:

Interrupt-driven API
====================

.. doxygengroup:: uart_interrupt


.. _uart_async_api:

Asynchronous API
================

.. doxygengroup:: uart_async
```
