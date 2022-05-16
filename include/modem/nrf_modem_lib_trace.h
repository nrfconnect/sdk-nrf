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

#include <zephyr/kernel.h>
#include <stdint.h>

/** @brief Initialize the modem trace module.
 *
 * Initializes the module and the trace transport medium.
 *
 * @param trace_heap Heap to use for modem traces
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_init(struct k_heap *trace_heap);

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
 *
 * @return Zero on success, non-zero otherwise.
 */
int nrf_modem_lib_trace_start(enum nrf_modem_lib_trace_mode trace_mode);

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

/**
 * @brief Deinitialize trace module.
 */
void nrf_modem_lib_trace_deinit(void);

#endif /* NRF_MODEM_LIB_TRACE_H__ */
/**@} */
