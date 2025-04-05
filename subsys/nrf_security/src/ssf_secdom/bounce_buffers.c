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

static K_HEAP_DEFINE(out_buffer_heap,
		     ROUND_UP(CONFIG_PSA_SSF_CRYPTO_CLIENT_OUT_HEAP_SIZE, CACHE_DATA_UNIT_SIZE));

/**
 * @brief Prepare an out buffer in case the original buffer is not aligned
 *
 * If the original buffer is not aligned, a new buffer is allocated and the data is copied to it.
 * This is needed to achieve DCache DataUnit alignment.
 *
 * @param original_buffer Original buffer
 * @param size Size of the buffer
 * @return void* NULL if the buffer could not be allocated, original_buffer if it was aligned, else
 * a new buffer from the heap
 *
 */
void *bounce_buffers_prepare(void *original_buffer, size_t size)
{
	void *out_buffer;

	if ((IS_ALIGNED(original_buffer, CACHE_DATA_UNIT_SIZE)) &&
	    (IS_ALIGNED(size, CACHE_DATA_UNIT_SIZE))) {
		out_buffer = original_buffer;
	} else {
		out_buffer = k_heap_alloc(&out_buffer_heap, size, K_NO_WAIT);
		if (out_buffer != NULL) {
			memcpy(out_buffer, original_buffer, size);
		}
	}

	return out_buffer;
}

/**
 * @brief Release an out buffer if it was allocated
 *
 * If the out buffer was allocated, the data is copied back to the original buffer and the out
 * buffer is first zeroed and then freed.
 *
 * @param original_buffer The original buffer
 * @param out_buffer The buffer to release
 * @param size Size of the buffer
 */
void bounce_buffers_release(void *original_buffer, void *out_buffer, size_t size)
{
	if (out_buffer == NULL) {
		return;
	}

	if (original_buffer == out_buffer) {
		return;
	}

	memcpy(original_buffer, out_buffer, size);
	/* Clear buffer before returning it to not leak sensitive data */
	memset(out_buffer, 0, size);
	sys_cache_data_flush_range(out_buffer, size);
	k_heap_free(&out_buffer_heap, out_buffer);
}
