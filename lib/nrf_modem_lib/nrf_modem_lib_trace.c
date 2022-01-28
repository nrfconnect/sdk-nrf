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

static struct k_heap *t_heap;
static bool backend_initialized;
static bool trace_running;

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
	int ret;

	while (true) {
		struct trace_data_t *trace_data = k_fifo_get(&trace_fifo, K_FOREVER);
		const uint8_t * const data = trace_data->data;
		const uint32_t len = trace_data->len;

		if (trace_running && backend_initialized) {
			ret = trace_backend_write(data, len);
			if (ret < 0) {
				LOG_ERR("trace_backend_write failed, err %d", ret);
			}
		}

		trace_processed_callback(data, len);

		k_heap_free(t_heap, trace_data);
	}
}

int nrf_modem_lib_trace_init(struct k_heap *trace_heap)
{
	int err;
	t_heap = trace_heap;
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
	int err = 0;

	if (backend_initialized) {
		struct trace_data_t *trace_data;
		size_t bytes = sizeof(struct trace_data_t);

		trace_data = k_heap_alloc(t_heap, bytes, K_NO_WAIT);

		if (trace_data != NULL) {
			trace_data->data = data;
			trace_data->len = len;

			k_fifo_put(&trace_fifo, trace_data);
		} else {
			LOG_ERR("trace_alloc failed");

			err = -ENOMEM;
		}
	} else {
		LOG_ERR("Modem trace received without initializing transport");

		err = -EPERM;
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

	trace_running = false;

	return 0;
}

void nrf_modem_lib_trace_deinit(void)
{
	int err;

	backend_initialized = false;

	err = trace_backend_deinit();

	if (err) {
		/* Cannot change function signature, so we just log the error instead */
		LOG_ERR("Modem trace backend deinit failed, err %d", err);
	}
}

K_THREAD_DEFINE(trace_thread_id, TRACE_THREAD_STACK_SIZE, trace_handler_thread,
	NULL, NULL, NULL, TRACE_THREAD_PRIORITY, 0, 0);
