/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>
#include <SEGGER_RTT.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

static int trace_rtt_channel;
static char rtt_buffer[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE];

static trace_backend_processed_cb trace_processed_callback;

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	const int segger_rtt_mode = SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL;

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	trace_rtt_channel = SEGGER_RTT_AllocUpBuffer("modem_trace", rtt_buffer, sizeof(rtt_buffer),
						     segger_rtt_mode);

	if (trace_rtt_channel <= 0) {
		return -EBUSY;
	}

	return 0;
}

int trace_backend_deinit(void)
{
	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	int err;
	int ret;

	uint8_t *buf = (uint8_t *)data;
	size_t remaining_bytes = len;

	while (remaining_bytes) {
		uint16_t transfer_len = MIN(remaining_bytes,
			CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE);
		size_t idx = len - remaining_bytes;

		ret = SEGGER_RTT_WriteNoLock(trace_rtt_channel, &buf[idx], transfer_len);

		remaining_bytes -= ret;

		err = trace_processed_callback(ret);
		if (err) {
			return err;
		}
	}

	return (int)len;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
};
