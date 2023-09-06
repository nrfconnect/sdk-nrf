/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef TRACE_BACKEND_H__
#define TRACE_BACKEND_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file trace_backend.h
 *
 * @defgroup trace_backend nRF91 Modem trace backend interface.
 * @{
 */

/** @brief callback to signal the trace module that
 * some amount of trace data has been processed.
 */
typedef int (*trace_backend_processed_cb)(size_t len);

/**
 * @brief The trace backend interface, implemented by the trace backend.
 */
struct nrf_modem_lib_trace_backend {
	/**
	 * @brief Initialize the compile-time selected trace backend.
	 *
	 * @param trace_processed_cb Function callback for signaling how much of the trace data has
	 *                           been processed by the backend.
	 *
	 * @return 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*init)(trace_backend_processed_cb trace_processed_cb);

	/**
	 * @brief Deinitialize the compile-time selected trace backend.
	 *
	 * @return 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*deinit)(void);

	/**
	 * @brief Write trace data to the compile-time selected trace backend.
	 *
	 * @param data Memory buffer containing modem trace data.
	 * @param len  Memory buffer length.
	 *
	 * @returns Number of bytes written if the operation was successful.
	 *          Otherwise, a (negative) error code is returned.
	 *          Especially,
	 *          -ENOSPC if no space is available and the backend has to be cleared before
	 *                  tracing can continue. For some trace backends, space is also cleared
	 *                  when performing the read operation.
	 *          -EAGAIN if no data were written due to e.g. flow control and the operation
	 *                  should be retried.
	 */
	int (*write)(const void *data, size_t len);

	/**
	 * @brief Get the number of bytes stored in the compile-time selected trace backend.
	 *
	 * @note To ensure the retrieved number of bytes is correct, this function should only be
	 *       called when the backend is done processing traces.
	 *       Set to @c NULL if this operation is not supported by the trace backend.
	 *
	 * @returns Number of bytes stored in the trace backend.
	 */
	size_t (*data_size)(void);

	/**
	 * @brief Read trace data from the compile-time selected trace backend.
	 *
	 * Read out the trace data from the trace backend. After read, the backend can free the
	 * space and prepare it for further write operations.
	 *
	 * @note Set to @c NULL if this operation is not supported by the trace backend.
	 *
	 * @param buf Output buffer
	 * @param len Size of output buffer
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*read)(void *buf, size_t len);

	/**
	 * @brief Erase all captured trace data in the compile-time selected trace backend.
	 *
	 * Erase all captured trace data and prepare for capturing new traces.
	 *
	 * @note Set to @c NULL if this operation is not supported by the trace backend.
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*clear)(void);

	/**
	 * @brief Suspend trace backend.
	 *
	 * This brings the backend to a power saving state. @c resume must be called before tracing
	 * can continue.
	 *
	 * @note Set to @c NULL if this operation is not supported by the trace backend.
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*suspend)(void);

	/**
	 * @brief Resume trace backend.
	 *
	 * This wakes the backend from a power saving state.
	 *
	 * @note Set to @c NULL if this operation is not supported by the trace backend.
	 *
	 * @return 0 on success, negative errno on failure.
	 */
	int (*resume)(void);
};

/**@} */ /* defgroup trace_backend */

#ifdef __cplusplus
}
#endif

#endif /* TRACE_BACKEND_H__ */
