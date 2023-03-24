/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);


/* Let's put this in noinit, so it can be read out after a crash/reboot. */
static __noinit uint8_t _ring_buffer_data_ram_trace_buf[
	CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RAM_LENGTH
];

__noinit struct ring_buf ram_trace_buf;
__noinit uint32_t ram_trace_buf_magic;

static trace_backend_processed_cb trace_processed_callback;

static bool is_initialized(void)
{
	return ram_trace_buf_magic == 0xdeadbeed;
}

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	if (!is_initialized()) {
		ram_trace_buf.buffer = _ring_buffer_data_ram_trace_buf;
		ram_trace_buf.size = ARRAY_SIZE(_ring_buffer_data_ram_trace_buf);
		ring_buf_reset(&ram_trace_buf);
		ram_trace_buf_magic = 0xdeadbeed;
	}
	trace_processed_callback = trace_processed_cb;
	return 0;
}

int trace_backend_deinit(void)
{
	/* nothing happens */
	return 0;
}

size_t trace_backend_data_size(void)
{
	if (!is_initialized()) {
		return -EPERM;
	}
	return ring_buf_size_get(&ram_trace_buf);
}

int trace_backend_read(void *buf, size_t len)
{
	if (!is_initialized()) {
		return -EPERM;
	}
	return ring_buf_get(&ram_trace_buf, buf, len);
}

int trace_backend_write(const void *data, size_t len)
{
	if (!is_initialized()) {
		return -EPERM;
	}
	if (len > ring_buf_capacity_get(&ram_trace_buf)) {
		LOG_ERR("trace_backend_write called with more data then buffer can store!");
		len = ring_buf_capacity_get(&ram_trace_buf);
		ring_buf_reset(&ram_trace_buf);
	}

	uint32_t free_space = ring_buf_space_get(&ram_trace_buf);

	if (len > free_space) {
		ring_buf_get(&ram_trace_buf, NULL, len - free_space);
	}

	int result = ring_buf_put(&ram_trace_buf, data, len);

	trace_processed_callback(len);

	return result;
}

int trace_backend_clear(void)
{
	if (!is_initialized()) {
		return -EPERM;
	}
	ring_buf_reset(&ram_trace_buf);
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
