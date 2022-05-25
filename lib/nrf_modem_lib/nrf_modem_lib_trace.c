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
#include <nrf_errno.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
#include <nrfx_uarte.h>
#endif
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
#include <SEGGER_RTT.h>
#endif

LOG_MODULE_REGISTER(nrf_modem_lib_trace, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

static struct k_heap *t_heap;

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
/* Maximum time to wait for a UART transfer to complete before giving up.*/
#define UART_TX_WAIT_TIME_MS 100
#define UNUSED_FLAGS 0

#define UART1_NL DT_NODELABEL(uart1)
PINCTRL_DT_DEFINE(UART1_NL);
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);

/* Semaphore used to check if the last UART transfer was complete. */
static K_SEM_DEFINE(tx_sem, 1, 1);

static void wait_for_tx_done(void);
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
static int trace_rtt_channel;
static char rtt_buffer[CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE];
#endif

static bool is_transport_initialized;
static bool is_stopped;

struct trace_data_t {
	void *fifo_reserved; /* 1st word reserved for use by fifo */
	const uint8_t *data;
	uint32_t len;
};

K_FIFO_DEFINE(trace_fifo);

static void trace_processed_callback(const uint8_t *data, uint32_t len)
{
	int err;

	err = nrf_modem_trace_processed_callback(data, len);
	(void) err;

	__ASSERT(err == 0,
		 "nrf_modem_trace_processed_callback returns error %d for "
		 "data = %p, len = %d",
		 err, data, len);
}

#define TRACE_THREAD_STACK_SIZE 512
#define TRACE_THREAD_PRIORITY CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PRIO

void trace_handler_thread(void)
{
	while (1) {
		struct trace_data_t *trace_data = k_fifo_get(&trace_fifo, K_FOREVER);
		const uint8_t * const data = trace_data->data;
		const uint32_t len = trace_data->len;

		if (!is_stopped) {
#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
			/* Split buffer into smaller chunks to be transferred using DMA. */
			const uint32_t MAX_BUF_LEN = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
			uint32_t remaining_bytes = len;
			nrfx_err_t err;

			while (remaining_bytes) {
				size_t transfer_len = MIN(remaining_bytes, MAX_BUF_LEN);
				uint32_t idx = len - remaining_bytes;

				if (k_sem_take(&tx_sem, K_MSEC(UART_TX_WAIT_TIME_MS)) != 0) {
					LOG_WRN("UARTE TX not available");
					break;
				}
				err = nrfx_uarte_tx(&uarte_inst, &data[idx], transfer_len);
				if (err != NRFX_SUCCESS) {
					LOG_ERR("nrfx_uarte_tx error: %d", err);
					k_sem_give(&tx_sem);
					break;
				}
				remaining_bytes -= transfer_len;
			}
			wait_for_tx_done();
#endif

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT
			uint32_t remaining_bytes = len;

			while (remaining_bytes) {
				uint16_t transfer_len = MIN(remaining_bytes,
						CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_RTT_BUF_SIZE);
				uint32_t idx = len - remaining_bytes;

				SEGGER_RTT_WriteSkipNoLock(trace_rtt_channel, &data[idx],
					transfer_len);
				remaining_bytes -= transfer_len;
			}
#endif
		}

		trace_processed_callback(data, len);
		k_heap_free(t_heap, trace_data);
	}
}

#ifdef CONFIG_NRF_MODEM_LIB_TRACE_MEDIUM_UART
static void uarte_callback(nrfx_uarte_event_t const *event, void *context)
{
	__ASSERT(k_sem_count_get(&tx_sem) == 0,
			"UART TX semaphore not in use");

	if (event->type == NRFX_UARTE_EVT_ERROR) {
		LOG_ERR("uarte error 0x%04x", event->data.error.error_mask);

		k_sem_give(&tx_sem);
	}

	if (event->type == NRFX_UARTE_EVT_TX_DONE) {
		k_sem_give(&tx_sem);
	}
}

static bool uart_init(void)
{
	int ret;
	const uint8_t irq_priority = DT_IRQ(UART1_NL, priority);

	const nrfx_uarte_config_t config = {
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,

		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,

		.interrupt_priority = irq_priority,
		.p_context = NULL,
	};

	ret = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL),
		PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(ret == 0);

	IRQ_CONNECT(DT_IRQN(UART1_NL),
		irq_priority,
		nrfx_isr,
		&nrfx_uarte_1_irq_handler,
		UNUSED_FLAGS);
	return (nrfx_uarte_init(&uarte_inst, &config, &uarte_callback) ==
		NRFX_SUCCESS);
}

static void wait_for_tx_done(void)
{
	/* Attempt to take the TX semaphore to stop the thread execution until
	 * UART is done sending. Immediately give the semaphore when it becomes
	 * available.
	 */
	if (k_sem_take(&tx_sem, K_MSEC(UART_TX_WAIT_TIME_MS)) != 0) {
		LOG_WRN("UARTE TX not available");
	}
	k_sem_give(&tx_sem);
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
	t_heap = trace_heap;
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
	int err = 0;

	if (is_transport_initialized) {
		struct trace_data_t *trace_data;
		size_t bytes = sizeof(struct trace_data_t);

		trace_data = k_heap_alloc(t_heap, bytes, K_NO_WAIT);

		if (trace_data != NULL) {
			trace_data->data = data;
			trace_data->len = len;

			k_fifo_put(&trace_fifo, trace_data);
		} else {
			LOG_ERR("trace_alloc failed.");

			err = -ENOMEM;
		}
	} else {
		LOG_ERR("Modem trace received without initializing transport");

		err = -ENXIO;
	}

	if (err) {
		/* Unable to handle the trace, but the
		 * nrf_modem_trace_processed_callback should always be called as
		 * required by the modem library.
		 */
		trace_processed_callback(data, len);
	}

	return err;
}

int nrf_modem_lib_trace_stop(void)
{
	__ASSERT(!k_is_in_isr(),
		"nrf_modem_lib_trace_stop cannot be called from interrupt context");

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

K_THREAD_DEFINE(trace_thread_id, TRACE_THREAD_STACK_SIZE, trace_handler_thread,
	NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);
