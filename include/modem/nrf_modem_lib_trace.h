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
