/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file nrf_modem_lib_trace.h
 *
 * @defgroup nrf_modem_lib_trace nRF91 Modem trace module
 * @{
 */
#ifndef NRF_MODEM_LIB_TRACE_H__
#define NRF_MODEM_LIB_TRACE_H__

#include <stdint.h>

/** @brief Initialize the modem trace module.
 *
 * Initializes the module and the trace transport medium.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_init(void);

/**@brief Trace mode
 *
 * The trace mode can be used to filter the traces.
 */
enum nrf_modem_lib_trace_mode {
	NRF_MODEM_LIB_TRACE_COREDUMP_ONLY = 1,  /**< Coredump only. */
	NRF_MODEM_LIB_TRACE_ALL = 2,            /**< LTE, IP, GNSS, and coredump. */
	NRF_MODEM_LIB_TRACE_IP_ONLY = 4,        /**< IP. */
	NRF_MODEM_LIB_TRACE_LTE_IP = 5,         /**< LTE and IP. */
};

/** @brief Start a trace session.
 *
 * This function sends AT command that requests the modem to start sending traces.
 *
 * @param trace_mode Trace mode
 * @param duration   Trace duration in seconds. If set to 0, the trace session will
 *                   continue until @ref nrf_modem_lib_trace_stop is called or until
 *                   the required size of max trace data (specified by the
 *                   @p max_size parameter) is received.
 * @param max_size   Maximum size (in bytes) of trace data that should be received.
 *                   The tracing will be stopped after receiving @p max_size
 *                   bytes. If set to 0, the trace session will continue until
 *                   @ref nrf_modem_lib_trace_stop is called or until the duration
 *                   set via the @p duration parameter is reached.
 *                   To ensure the integrity of the trace output, the
 *                   modem_trace module will never skip a trace message. For
 *                   this purpose, if it detects that a received trace won't fit
 *                   in the maximum allowed size, it will stop the trace session
 *                   without sending out that trace to the transport medium.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_start(enum nrf_modem_lib_trace_mode trace_mode, uint16_t duration,
			      uint32_t max_size);

/** @brief Process modem trace data
 *
 * This function should only be called to process a trace received from the modem by the
 * nrf_modem_os_trace_put() function. This function forwards the trace to the selected
 * (during compile time) trace transport medium. When the maximum number of trace bytes
 * (configured via the @ref nrf_modem_lib_trace_start) is received, this function stops the trace
 * session.
 *
 * @param data Memory buffer containing the modem trace data.
 * @param len  Memory buffer length.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_process(const uint8_t *data, uint32_t len);

/** @brief Stop an ongoing trace session
 *
 * This function stops an ongoing trace session.
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_stop(void);

#endif /* NRF_MODEM_LIB_TRACE_H__ */
/**@} */
