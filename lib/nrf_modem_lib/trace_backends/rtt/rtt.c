/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>
#include <SEGGER_RTT.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

static int trace_rtt_channel;
static char rtt_buffer[CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE];

static trace_backend_processed_cb trace_processed_callback;

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}
	if (trace_rtt_channel <= 0) {
		return -EBUSY;
	}

	trace_processed_callback = trace_processed_cb;

	LOG_INF("Modem_trace RTT backend channel %d", trace_rtt_channel);

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
	uint8_t failed_write_count = 0;

	while (remaining_bytes) {
		uint16_t transfer_len = MIN(remaining_bytes,
			CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE);
		size_t idx = len - remaining_bytes;

		ret = SEGGER_RTT_WriteNoLock(trace_rtt_channel, &buf[idx], transfer_len);
		if (!ret) {
			if (failed_write_count < 10) {
				failed_write_count++;
				k_msleep(10);
				continue;
			}

			return (remaining_bytes == len ? -ENOSR : (len - remaining_bytes));
		}

		failed_write_count = 0;
		remaining_bytes -= ret;

		err = trace_processed_callback(ret);
		if (err) {
			LOG_ERR("Trace processed callback failed, err %d", err);
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

/* Allocate RTT buffer ASAP so external tooling can attach before `nrf_modem_init` */
IF_DISABLED(CONFIG_UNITY, (static))
int nrf_modem_lib_trace_rtt_channel_alloc(void)
{
	const int segger_rtt_mode = SEGGER_RTT_MODE_NO_BLOCK_TRIM;

	trace_rtt_channel = SEGGER_RTT_AllocUpBuffer("modem_trace", rtt_buffer, sizeof(rtt_buffer),
						     segger_rtt_mode);

	return trace_rtt_channel < 0 ? -EBUSY : 0;
}

/* The RTT core initialises in PRE_KERNEL_1 */
SYS_INIT(nrf_modem_lib_trace_rtt_channel_alloc, POST_KERNEL, 0);
