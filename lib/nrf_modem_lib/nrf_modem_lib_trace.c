/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <kernel.h>
#include <zephyr.h>
#include <modem/nrf_modem_lib_trace.h>
#include <nrf_modem_at.h>
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
#include <nrfx_uarte.h>
#endif
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
#include <SEGGER_RTT.h>
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
static int trace_rtt_channel;
static char rtt_buffer[CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE];
#endif

static uint32_t max_trace_size_bytes;
static uint32_t total_trace_size_rcvd;
static bool is_transport_initialized;

static void trace_stop_timer_handler(struct k_timer *timer);

K_TIMER_DEFINE(trace_stop_timer, trace_stop_timer_handler, NULL);

static void trace_stop_timer_handler(struct k_timer *timer)
{
	nrf_modem_lib_trace_stop();
}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
static bool uart_init(void)
{
	const nrfx_uarte_config_t config = {
		.pseltxd = DT_PROP(DT_NODELABEL(uart1), tx_pin),
		.pselrxd = DT_PROP(DT_NODELABEL(uart1), rx_pin),
		.pselcts = NRF_UARTE_PSEL_DISCONNECTED,
		.pselrts = NRF_UARTE_PSEL_DISCONNECTED,

		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,

		.interrupt_priority = NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	return (nrfx_uarte_init(&uarte_inst, &config, NULL) == NRFX_SUCCESS);
}
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
static bool rtt_init(void)
{
	trace_rtt_channel = SEGGER_RTT_AllocUpBuffer("modem_trace", rtt_buffer, sizeof(rtt_buffer),
						     SEGGER_RTT_MODE_NO_BLOCK_SKIP);

	return (trace_rtt_channel > 0);
}
#endif

int nrf_modem_lib_trace_init(void)
{
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
	is_transport_initialized = uart_init();
#endif
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
	is_transport_initialized = rtt_init();
#endif

	if (!is_transport_initialized) {
		return -EBUSY;
	}
	return 0;
}

int nrf_modem_lib_trace_start(enum nrf_modem_lib_trace_mode trace_mode, uint16_t duration,
			      uint32_t max_size)
{
	if (!is_transport_initialized) {
		return -ENXIO;
	}

	total_trace_size_rcvd = 0;

	max_trace_size_bytes = max_size;

	if (nrf_modem_at_printf("AT%%XMODEMTRACE=1,%hu", trace_mode) != 0) {
		return -EOPNOTSUPP;
	}

	if (duration != 0) {
		k_timer_start(&trace_stop_timer, K_SECONDS(duration), K_SECONDS(0));
	}

	return 0;
}

int nrf_modem_lib_trace_process(const uint8_t *data, uint32_t len)
{
	if (!is_transport_initialized) {
		return -ENXIO;
	}

	if (max_trace_size_bytes != 0) {
		total_trace_size_rcvd += len;

		if (total_trace_size_rcvd > max_trace_size_bytes) {
			/* Skip sending  to transport medium as the current trace won't fit.
			 * Disable traces (see API doc for reasoning).
			 */
			nrf_modem_lib_trace_stop();

			return 0;
		}
		if (total_trace_size_rcvd == max_trace_size_bytes) {
			nrf_modem_lib_trace_stop();
		}
	}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
	/* Split RAM buffer into smaller chunks to be transferred using DMA. */
	uint32_t remaining_bytes = len;
	const uint32_t MAX_BUF_LEN = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;

	while (remaining_bytes) {
		size_t transfer_len = MIN(remaining_bytes, MAX_BUF_LEN);
		uint32_t idx = len - remaining_bytes;

		nrfx_uarte_tx(&uarte_inst, &data[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
	uint32_t remaining_bytes = len;

	while (remaining_bytes) {
		uint8_t transfer_len = MIN(remaining_bytes,
					CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);
		uint32_t idx = len - remaining_bytes;

		SEGGER_RTT_WriteSkipNoLock(trace_rtt_channel, &data[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}
#endif

	return 0;
}

int nrf_modem_lib_trace_stop(void)
{
	k_timer_stop(&trace_stop_timer);

	if (nrf_modem_at_printf("AT%%XMODEMTRACE=0") != 0) {
		return -EOPNOTSUPP;
	}

	return 0;
}
