/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib_trace.h>
#include <modem/trace_backend.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>

LOG_MODULE_REGISTER(nrf_modem_lib_trace, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

static bool backend_initialized;
static bool trace_running;

int nrf_modem_lib_trace_init(struct k_heap *trace_heap)
{
	int err;

	ARG_UNUSED(trace_heap);
	trace_running = true;

	err = trace_backend_init();
	if (err) {
		backend_initialized = false;
		return -EBUSY;
	}

	backend_initialized = true;

	return 0;
}

int nrf_modem_lib_trace_start(enum nrf_modem_lib_trace_mode trace_mode)
{
	if (!backend_initialized) {
		return -EPERM;
	}

	if (nrf_modem_at_printf("AT%%XMODEMTRACE=1,%hu", trace_mode) != 0) {
		return -EOPNOTSUPP;
	}

	trace_running = true;

	return 0;
}

int nrf_modem_lib_trace_process(const uint8_t *data, uint32_t len)
{
	int ret;

	if (trace_running && backend_initialized) {
		ret = trace_backend_write(data, len);
		if (ret < 0) {
			LOG_ERR("trace_backend_write failed, err %d", ret);
		}
	}

	ret = nrf_modem_trace_processed_callback(data, len);
	if (ret) {
		LOG_ERR("nrf_modem_trace_processed_callback failed, err %d", ret);
	}

	return 0;
}

int nrf_modem_lib_trace_stop(void)
{
	__ASSERT(!k_is_in_isr(), "%s cannot be called from interrupt context", __func__);

	/* Don't use the AT%%XMODEMTRACE=0 command to disable traces because the
	 * modem won't respond if the modem has crashed and is outputting the modem
	 * core dump.
	 */

	trace_running = false;

	return 0;
}

void nrf_modem_lib_trace_deinit(void)
{
	trace_backend_deinit();
	backend_initialized = false;
}
