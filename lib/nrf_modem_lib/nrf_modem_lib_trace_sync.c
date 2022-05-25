/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <modem/nrf_modem_lib_trace.h>
#include <nrf_modem_at.h>
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
#include <nrfx_uarte.h>
#endif
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
#include <SEGGER_RTT.h>
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
#define UART1_NL DT_NODELABEL(uart1)
PINCTRL_DT_DEFINE(UART1_NL);
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
static int trace_rtt_channel;
static char rtt_buffer[CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE];
#endif

static bool is_transport_initialized;
static bool is_stopped;

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
static bool uart_init(void)
{
	int ret;
	const nrfx_uarte_config_t config = {
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,

		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,

		.interrupt_priority = NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL), PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(ret == 0);

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

int nrf_modem_lib_trace_init(struct k_heap *trace_heap)
{
	ARG_UNUSED(trace_heap);
	is_stopped = false;

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

int nrf_modem_lib_trace_start(enum nrf_modem_lib_trace_mode trace_mode)
{
	if (!is_transport_initialized) {
		return -ENXIO;
	}

	if (nrf_modem_at_printf("AT%%XMODEMTRACE=1,%hu", trace_mode) != 0) {
		return -EOPNOTSUPP;
	}

	is_stopped = false;

	return 0;
}

int nrf_modem_lib_trace_process(const uint8_t *data, uint32_t len)
{
	if (!is_transport_initialized) {
		return -ENXIO;
	}

	if (is_stopped) {
		return 0;
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
	__ASSERT(!k_is_in_isr(), "%s cannot be called from interrupt context", __func__);

	/* Don't use the AT%%XMODEMTRACE=0 command to disable traces because the
	 * modem won't respond if the modem has crashed and is outputting the modem
	 * core dump.
	 */

	is_stopped = true;

	return 0;
}

void nrf_modem_lib_trace_deinit(void)
{
	is_transport_initialized = false;

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
	nrfx_uarte_uninit(&uarte_inst);
#endif
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
	/* Flush writes, uninitialize peripheral. */
#endif
}
