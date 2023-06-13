/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <memfault/metrics/metrics.h>
#include <memfault/ports/zephyr/http.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>
#include <memfault/core/custom_data_recording.h>
#include <modem/nrf_modem_lib_trace.h>

#include "modem_trace_memfault.h"
#include "mosh_print.h"

static const char *mimetypes[] = { MEMFAULT_CDR_BINARY };

static sMemfaultCdrMetadata trace_recording_metadata = {
	.start_time.type = kMemfaultCurrentTimeType_Unknown,
	.mimetypes = mimetypes,
	.num_mimetypes = 1,
	.data_size_bytes = 0,
	.collection_reason = "modem traces",
};

static bool has_modem_traces;

static bool has_cdr_cb(sMemfaultCdrMetadata *metadata)
{
	if (!has_modem_traces) {
		return false;
	}

	*metadata = trace_recording_metadata;

	return true;
}

static bool read_data_cb(uint32_t offset, void *data, size_t data_len)
{
	int err = nrf_modem_lib_trace_read(data, data_len);

	if (err < 0) {
		mosh_error("Error reading modem traces: %d", err);
		return false;
	}

	return true;
}

static void mark_cdr_read_cb(void)
{
	has_modem_traces = false;
}

static sMemfaultCdrSourceImpl s_my_custom_data_recording_source = {
	.has_cdr_cb = has_cdr_cb,
	.read_data_cb = read_data_cb,
	.mark_cdr_read_cb = mark_cdr_read_cb,
};

void modem_trace_memfault_send(uint32_t trace_duration_ms)
{
	static bool memfault_cdr_source_registered;
	size_t size = nrf_modem_lib_trace_data_size();

	if (!memfault_cdr_source_registered) {
		memfault_cdr_register_source(&s_my_custom_data_recording_source);
		memfault_cdr_source_registered = true;
	}

	trace_recording_metadata.duration_ms = 0;
	trace_recording_metadata.data_size_bytes = size;
	has_modem_traces = true;

	mosh_print("Prepared to send %d bytes of trace data to memfault during next transfer.",
		size);
	mosh_print("Trigger a transfer now with \"mflt post_chunks\".");
}
