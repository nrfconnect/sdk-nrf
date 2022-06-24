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

int trace_backend_init(void)
{
	int err;
	const nrfx_uarte_config_t config = {
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
		.hal_cfg.hwfc = NRF_UARTE_HWFC_DISABLED,
		.hal_cfg.parity = NRF_UARTE_PARITY_EXCLUDED,
		.baudrate = NRF_UARTE_BAUDRATE_1000000,
		.interrupt_priority = NRFX_UARTE_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL), PINCTRL_STATE_DEFAULT);
	__ASSERT_NO_MSG(err == 0);

	err = nrfx_uarte_init(&uarte_inst, &config, NULL);
	if (err != NRFX_SUCCESS && err != NRFX_ERROR_INVALID_STATE) {
		return -EBUSY;
	}

	return 0;
}

int trace_backend_deinit(void)
{
	int err;

	nrfx_uarte_uninit(&uarte_inst);

	err = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(UART1_NL), PINCTRL_STATE_SLEEP);
	__ASSERT_NO_MSG(err == 0);

	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	/* Split RAM buffer into smaller chunks to be transferred using DMA. */
	uint8_t *buf = (uint8_t *)data;
	size_t remaining_bytes = len;
	const size_t MAX_BUF_LEN = (1 << UARTE1_EASYDMA_MAXCNT_SIZE) - 1;

	while (remaining_bytes) {
		size_t transfer_len = MIN(remaining_bytes, MAX_BUF_LEN);
		size_t idx = len - remaining_bytes;

		nrfx_uarte_tx(&uarte_inst, &buf[idx], transfer_len);
		remaining_bytes -= transfer_len;
	}

	return len;
}
