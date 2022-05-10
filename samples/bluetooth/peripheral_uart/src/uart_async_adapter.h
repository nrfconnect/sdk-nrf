/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief UART asynchronous API adapter
 */

/**
 * @brief UART asynchronous API universal adapter
 * @defgroup uart_async_adapter UART ASYNC adapter
 * @{
 *
 * This module acts as an adapter between UART interrupt and async interface.
 *
 * @note The UART ASYNC API adapter implementation is experimental.
 *       It means it is not guaranteed to work in any corner situation.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>


/**
 * @brief UART asynch adapter data structure
 *
 * The data used by asynch adapter.
 */
struct uart_async_adapter_data {
	/** Target device pointer */
	const struct device *target;
	/** User callback function for synchronous interface */
	uart_callback_t user_callback;
	/** Pointer to the user data */
	void *user_data;

	/* The interface realization */
	/** Lock for the data that requires it */
	struct k_spinlock lock;

	/** Data used for output transmission */
	struct {
		/** The original buffer pointer set when data transfer was requested */
		const uint8_t *buf;
		/** Current buffer position */
		const uint8_t *curr_buf;
		/** Number of data left in the current buffer */
		volatile size_t size_left;
		/** Timer used for timeout */
		struct k_timer timeout_timer;
		/** Tx state */
		bool enabled;
	} tx;

	/** Data used for input transmission */
	struct {
		/** Base buffer pointer used now for data reception */
		uint8_t *buf;
		/** Current position to write data into */
		uint8_t *curr_buf;
		/** Last position of the notified buffer */
		uint8_t *last_notify_buf;
		/** Number of bytes left in the current buffer */
		size_t size_left;
		/** Buffer prepared for the next transfer */
		uint8_t *next_buf;
		/** The size of the buffer for the next transfer */
		size_t next_buf_len;
		/** Timeout set by the user */
		int32_t timeout;
		/** Timer used for timeout */
		struct k_timer timeout_timer;
		/** RX state */
		bool enabled;
	} rx;
};

/**
 * @brief Driver API for ASYNC adapter
 *
 * The API of the UART async adapter uses standard UART API structure.
 */
extern const struct uart_driver_api uart_async_adapter_driver_api;

/**
 * @brief The name of the data instance connected with created device instance
 *
 * @name _dev_name The name of the created device instance
 */
#define UART_ASYNC_ADAPTER_INST_DATA_NAME(_dev_name) _CONCAT(uart_async_adapter_data_, _dev_name)

#define UART_ASYNC_ADAPTER_INST_STATE_NAME(_dev_name) _CONCAT(uart_async_adapter_state_, _dev_name)

#define UART_ASYNC_ADAPTER_INST_NAME(_dev_name) _CONCAT(_dev_name, _inst)

/**
 * @brief The macro that creates and instance of the UART async adapter
 *
 * @name _dev The name of the created device instance
 */
#define UART_ASYNC_ADAPTER_INST_DEFINE(_dev) \
	static struct uart_async_adapter_data UART_ASYNC_ADAPTER_INST_DATA_NAME(_dev); \
	static struct device_state UART_ASYNC_ADAPTER_INST_STATE_NAME(_dev); \
	static const struct device UART_ASYNC_ADAPTER_INST_NAME(_dev) = { \
		.name = STRINGIFY(_dev), \
		.api = &uart_async_adapter_driver_api, \
		.state = &UART_ASYNC_ADAPTER_INST_STATE_NAME(_dev), \
		.data = &UART_ASYNC_ADAPTER_INST_DATA_NAME(_dev), \
	}; \
	static const struct device *const _dev = &UART_ASYNC_ADAPTER_INST_NAME(_dev)

/**
 * @brief Initialize adapter
 *
 * Call this function before uart adapter is used.
 * The function configures connected UART device to work with the adapter.
 *
 * @param dev    The adapter interface
 * @param target Target UART device with only interrupt interface
 */
void uart_async_adapter_init(const struct device *dev, const struct device *target);

/** @} */
