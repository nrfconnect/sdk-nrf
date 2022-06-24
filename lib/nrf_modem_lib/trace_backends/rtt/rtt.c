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

int trace_backend_init(void)
{
#if defined(CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING)
	const int segger_rtt_mode = SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL;
#else
	const int segger_rtt_mode = SEGGER_RTT_MODE_NO_BLOCK_SKIP;
#endif

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
	uint8_t *buf = (uint8_t *)data;
	size_t remaining_bytes = len;

	while (remaining_bytes) {
		uint16_t transfer_len = MIN(remaining_bytes,
			CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT_BUF_SIZE);
		size_t idx = len - remaining_bytes;

#if defined(CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING)
		SEGGER_RTT_Write(trace_rtt_channel, &buf[idx], transfer_len);
#else
		SEGGER_RTT_WriteNoLock(trace_rtt_channel, &buf[idx], transfer_len);
#endif
		remaining_bytes -= transfer_len;
	}

	return (int)len;
}
