/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>
#include <nrfx_uarte.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

#define UART1_NL DT_NODELABEL(uart1)
PINCTRL_DT_DEFINE(UART1_NL);
static const nrfx_uarte_t uarte_inst = NRFX_UARTE_INSTANCE(1);

/* Maximum time to wait for a UART transfer to complete before giving up. */
#define UART_TX_WAIT_TIME_MS 100
#define UNUSED_FLAGS 0

/* Semaphore used to check if the last UART transfer was completed. */
static K_SEM_DEFINE(tx_sem, 1, 1);

static trace_backend_processed_cb trace_processed_callback;

static void uarte_callback(nrfx_uarte_event_t const *event, void *p_context)
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

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;
	const nrfx_uarte_config_t config = {
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,
		.interrupt_priority = DT_IRQ(UART1_NL, priority),
		.p_context = NULL,
	};

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL), PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(err == 0);

	IRQ_CONNECT(DT_IRQN(UART1_NL), DT_IRQ(UART1_NL, priority), nrfx_isr,
		    &nrfx_uarte_1_irq_handler, UNUSED_FLAGS);

	err = nrfx_uarte_init(&uarte_inst, &config, &uarte_callback);
	if (err != NRFX_SUCCESS && err != NRFX_ERROR_INVALID_STATE) {
		return -EBUSY;
	}

	return 0;
}

int trace_backend_deinit(void)
{
	nrfx_uarte_uninit(&uarte_inst);

#if CONFIG_PM_DEVICE
	int err;

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL), PINCTRL_STATE_SLEEP);
	__ASSERT_NO_MSG(err == 0);
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	nrfx_err_t err;

	/* Split RAM buffer into smaller chunks to be transferred using DMA. */
	uint8_t *buf = (uint8_t *)data;
	const size_t MAX_BUF_LEN = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;
	size_t remaining_bytes = len;

	while (remaining_bytes) {
		size_t transfer_len = MIN(remaining_bytes, MAX_BUF_LEN);
		size_t idx = len - remaining_bytes;

		if (k_sem_take(&tx_sem, K_MSEC(UART_TX_WAIT_TIME_MS)) != 0) {
			LOG_WRN("UARTE TX not available!");
			break;
		}

		err = nrfx_uarte_tx(&uarte_inst, &buf[idx], transfer_len);
		if (err != NRFX_SUCCESS) {
			LOG_ERR("nrfx_uarte_tx error: %d", err);
			k_sem_give(&tx_sem);
			break;
		}

		remaining_bytes -= transfer_len;
	}

	wait_for_tx_done();

	err = trace_processed_callback(len);
	if (err) {
		return err;
	}

	return len;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
};
