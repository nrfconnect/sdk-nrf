/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/zbus/zbus.h>
#include "memfault/components.h"
#include <memfault/ports/zephyr/http.h>
#include <memfault/metrics/metrics.h>
#include <memfault/core/data_packetizer.h>
#include <memfault/core/trace_event.h>
#include "memfault/panics/coredump.h"
#include <modem/nrf_modem_lib_trace.h>
#include <modem/nrf_modem_lib.h>

LOG_MODULE_DECLARE(memfault_ncs, CONFIG_MEMFAULT_NCS_LOG_LEVEL);

/* Forward declarations */
static bool has_cdr_cb(sMemfaultCdrMetadata *metadata);
static void mark_cdr_read_cb(void);
static bool read_data_cb(uint32_t offset, void *buf, size_t buf_len);

static const char * const mimetypes[] = {
	MEMFAULT_CDR_BINARY
};

static sMemfaultCdrMetadata trace_recording_metadata = {
	.start_time.type = kMemfaultCurrentTimeType_Unknown,
	.mimetypes = (char const **)mimetypes,
	.num_mimetypes = 1,
	.data_size_bytes = 0,
	.collection_reason = "modem traces",
};

static sMemfaultCdrSourceImpl recording = {
	.has_cdr_cb = has_cdr_cb,
	.read_data_cb = read_data_cb,
	.mark_cdr_read_cb = mark_cdr_read_cb,
};

static int modem_trace_enable(void)
{
	int err;

	err = nrf_modem_lib_trace_level_set(NRF_MODEM_LIB_TRACE_LEVEL_LTE_AND_IP);
	if (err) {
		LOG_ERR("nrf_modem_lib_trace_level_set, error: %d", err);
		return err;
	}

	return 0;
}

/* Enable modem traces as soon as the modem is initialized if there is no valid coredump.
 * Modem traces are disabled by default via CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_OFF.
 */
NRF_MODEM_LIB_ON_INIT(memfault_init_hook, on_modem_lib_init, NULL)

static void on_modem_lib_init(int ret, void *ctx)
{
	int err;

	if (memfault_coredump_has_valid_coredump(NULL)) {
		return;
	}

	LOG_DBG("No coredump valid, clearing modem trace buffer");

	err = nrf_modem_lib_trace_clear();
	if (err) {
		LOG_ERR("Failed to clear modem trace data: %d", err);
		return;
	}

	err = modem_trace_enable();
	if (err) {
		LOG_ERR("Failed to enable modem traces: %d", err);
		return;
	}
}

static bool has_cdr_cb(sMemfaultCdrMetadata *metadata)
{
	if (trace_recording_metadata.data_size_bytes == 0) {
		return false;
	}

	*metadata = trace_recording_metadata;
	return true;
}

static void mark_cdr_read_cb(void)
{
	int err;

	LOG_DBG("Modem trace data has been processed / uploaded");

	/* It's not guaranteed that the trace backend clears the stored traces after readout,
	 * so we clear them before enabling traces just in case.
	 */
	err = nrf_modem_lib_trace_clear();
	if (err) {
		LOG_ERR("Failed to clear modem trace data: %d", err);

		/* Continue */
	}

	err = modem_trace_enable();
	if (err) {
		LOG_ERR("Failed to enable modem traces after upload: %d", err);

		/* Continue */
	}

	/* Set CDR data length to 0 once the modem trace has been processed (uploaded) */
	trace_recording_metadata.data_size_bytes = 0;
}

static bool read_data_cb(uint32_t offset, void *buf, size_t buf_len)
{
	ARG_UNUSED(offset);

	int err = nrf_modem_lib_trace_read(buf, buf_len);

	if (err == -ENODATA) {
		LOG_WRN("No more modem trace data to read");
		return false;
	} else if (err < 0) {
		LOG_ERR("Failed to read modem trace data: %d", err);
		return false;
	}

	return true;
}

int memfault_lte_coredump_modem_trace_init(void)
{
	static bool initialized;

	if (initialized) {
		LOG_ERR("Already initialized");
		return -EALREADY;
	}

	if (!memfault_cdr_register_source(&recording)) {
		LOG_ERR("Failed to register modem trace CDR source, storage full");
		return -EIO;
	}

	initialized = true;

	LOG_DBG("Modem trace CDR source initialized");

	return 0;
}

int memfault_lte_coredump_modem_trace_prepare_for_upload(void)
{
	size_t trace_size = 0;

	trace_size = nrf_modem_lib_trace_data_size();

	if (trace_size == -ENOTSUP) {
		LOG_ERR("The current modem trace backend is not supported");
		return -ENOTSUP;
	} else if (trace_size < 0) {
		LOG_ERR("Failed to get modem trace size: %d", trace_size);
		return trace_size;
	} else if (trace_size == 0) {
		LOG_DBG("No modem traces to send");
		return -ENODATA;
	}

	LOG_DBG("Preparing modem trace data upload of: %d bytes", trace_size);

	trace_recording_metadata.duration_ms = 0;
	trace_recording_metadata.data_size_bytes = trace_size;

	return 0;
}
