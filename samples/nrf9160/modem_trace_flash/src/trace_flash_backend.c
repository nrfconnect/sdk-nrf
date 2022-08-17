/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <sys/errno.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>

#include "trace_storage.h"

LOG_MODULE_REGISTER(modem_trace_flash_backend, CONFIG_MODEM_TRACE_FLASH_BACKEND_LOG_LEVEL);

static trace_backend_processed_cb trace_processed_callback;

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	return trace_storage_init();
}

int trace_backend_deinit(void)
{
	LOG_DBG("Flash trace backend deinitialized\n");
	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	int write_ret = trace_storage_write(data, len);

	if (write_ret < 0) {
		LOG_ERR("trace_storage_write failed: %d", write_ret);
	}

	int cb_ret = trace_processed_callback(len);

	if (cb_ret < 0) {
		LOG_ERR("trace_processed_callback failed: %d", cb_ret);
		return cb_ret;
	}

	if (write_ret < 0) {
		return write_ret;
	}

	return (int)len;
}
