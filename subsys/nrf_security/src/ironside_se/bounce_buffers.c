/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include "bounce_buffers.h"

/* k_heap_alloc allocated memory is aligned on a multiple of pointer sizes. The HW's DataUnit size
 * must match this Zephyr behaviour.
 */
BUILD_ASSERT(CACHE_DATA_UNIT_SIZE == sizeof(uintptr_t));

static K_HEAP_DEFINE(out_buffer_heap, ROUND_UP(CONFIG_NRF_IRONSIDE_SE_PSA_SERVICES_OUT_HEAP_SIZE,
					       CACHE_DATA_UNIT_SIZE));

void *bounce_buffers_prepare(void *original_buffer, size_t size)
{
	void *out_buffer = NULL;

	if (((IS_ALIGNED(original_buffer, CACHE_DATA_UNIT_SIZE)) &&
	     (IS_ALIGNED(size, CACHE_DATA_UNIT_SIZE))) ||
	    (size == 0)) {
		out_buffer = original_buffer;
	} else {
		out_buffer = k_heap_alloc(&out_buffer_heap, size, K_NO_WAIT);
		if (out_buffer != NULL) {
			memcpy(out_buffer, original_buffer, size);
		}
	}

	return out_buffer;
}

void bounce_buffers_release(void *original_buffer, void *out_buffer, size_t size)
{
	if (out_buffer == NULL || out_buffer == original_buffer) {
		return;
	}

	memcpy(original_buffer, out_buffer, size);
	/* Clear buffer before returning it to not leak sensitive data */
	memset(out_buffer, 0, size);
	sys_cache_data_flush_range(out_buffer, size);
	k_heap_free(&out_buffer_heap, out_buffer);
}
