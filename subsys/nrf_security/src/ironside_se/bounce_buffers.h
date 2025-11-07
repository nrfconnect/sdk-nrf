/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __BOUNCE_BUFFERS_H__
#define __BOUNCE_BUFFERS_H__

#include <nrfx.h>

#define CACHE_DATA_UNIT_SIZE (DCACHEDATA_DATAWIDTH * 4)

#ifdef CONFIG_NRF_IRONSIDE_SE_PSA_SERVICES_OUT_BOUNCE_BUFFERS

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
void *bounce_buffers_prepare(void *original_buffer, size_t size);

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
void bounce_buffers_release(void *original_buffer, void *out_buffer, size_t size);

#else /* CONFIG_NRF_IRONSIDE_SE_PSA_SERVICES_OUT_BOUNCE_BUFFERS */

NRF_STATIC_INLINE
void *bounce_buffers_prepare(void *original_buffer, size_t size)
{
	ARG_UNUSED(size);

	return original_buffer;
}

NRF_STATIC_INLINE
void bounce_buffers_release(void *original_buffer, void *out_buffer, size_t size)
{
	ARG_UNUSED(original_buffer);
	ARG_UNUSED(out_buffer);
	ARG_UNUSED(size);
}

#endif  /* CONFIG_NRF_IRONSIDE_SE_PSA_SERVICES_OUT_BOUNCE_BUFFERS */

#endif /* __BOUNCE_BUFFERS_H__ */
