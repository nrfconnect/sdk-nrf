/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include <modem/trace_backend.h>

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	return 0;
}

int trace_backend_deinit(void)
{
	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	return 0;
}

size_t trace_backend_data_size(void)
{
	return 0;
}

int trace_backend_read(void *buf, size_t len)
{
	return 0;
}

int trace_backend_clear(void)
{
	return 0;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.data_size = trace_backend_data_size,
	.read = trace_backend_read,
	.clear = trace_backend_clear,
};
