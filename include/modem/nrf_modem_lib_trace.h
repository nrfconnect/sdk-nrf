/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_MODEM_LIB_TRACE_H__
#define NRF_MODEM_LIB_TRACE_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file nrf_modem_lib_trace.h
 *
 * @defgroup nrf_modem_lib_trace nRF91 Modem trace module
 * @{
 */

/** @brief Trace level
 *
 * The trace level can be used to filter the traces.
 */
enum nrf_modem_lib_trace_level {
	NRF_MODEM_LIB_TRACE_LEVEL_OFF = 0,		/**< Disable output. */
	NRF_MODEM_LIB_TRACE_LEVEL_COREDUMP_ONLY = 1,	/**< Coredump only. */
	NRF_MODEM_LIB_TRACE_LEVEL_FULL = 2,		/**< LTE, IP, GNSS, and coredump. */
	NRF_MODEM_LIB_TRACE_LEVEL_IP_ONLY = 4,		/**< IP. */
	NRF_MODEM_LIB_TRACE_LEVEL_LTE_AND_IP = 5,	/**< LTE and IP. */
};

/** @brief Trace event */
enum nrf_modem_lib_trace_event {
	NRF_MODEM_LIB_TRACE_EVT_FULL = -ENOSPC,        /**< Trace storage is full. */
};

/** @brief Trace callback that is called by the trace library when an event occour.
 *
 * @note This callback must be defined by the application with some trace backends.
 *
 * @param evt Occurring event
 *
 * @return Zero on success, non-zero otherwise.
 */
extern void nrf_modem_lib_trace_callback(enum nrf_modem_lib_trace_event evt);

/** @brief Wait for trace to have finished processing after coredump or shutdown.
 *
 * This function blocks until the trace module has finished processing data after
 * a modem fault (coredump) or modem shutdown.
 *
 * @param timeout Time to wait for trace processing to be done.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_processing_done_wait(k_timeout_t timeout);

/** @brief Set trace level.
 *
 * @param trace_level Trace level
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_level_set(enum nrf_modem_lib_trace_level trace_level);

/**
 * @brief Get the number of bytes stored in the compile-time selected trace backend.
 *
 * @note To ensure the retrieved number of bytes is correct, this function should only be called
 *       when the backend is done processing traces. This operation is only supported with some
 *       trace backends. If not supported, the function returns -ENOTSUP.
 *
 * @returns Number of bytes stored in the trace backend.
 */
size_t nrf_modem_lib_trace_data_size(void);

/**
 * @brief Read trace data
 *
 * Read out the trace data. After read, the backend can choose to free the
 * space and prepare it for further write operations.
 *
 * @note This operation is only supported with some trace backends. If not supported, the function
 *       returns -ENOTSUP.
 *
 * @param buf Output buffer
 * @param len Size of output buffer
 *
 * @return read number of bytes, negative errno on failure.
 */
int nrf_modem_lib_trace_read(uint8_t *buf, size_t len);

/**
 * @brief Clear captured trace data
 *
 * Clear all captured trace data and prepare for capturing new traces.
 *
 * @note This operation is only supported with some trace backends. If not supported, the function
 *       returns -ENOTSUP.
 *
 * @return 0 on success, negative errno on failure.
 */
int nrf_modem_lib_trace_clear(void);

#if defined(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE) || defined(__DOXYGEN__)
/** @brief Get the last measured rolling average bitrate of the trace backend.
 *
 * This function returns the last measured rolling average bitrate of the trace backend
 * calculated over the last @kconfig{CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE_PERIOD_MS} period.
 *
 * @return Rolling average bitrate of the trace backend
 */
uint32_t nrf_modem_lib_trace_backend_bitrate_get(void);
#endif /* defined(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_BITRATE) || defined(__DOXYGEN__) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_MODEM_LIB_TRACE_H__ */
