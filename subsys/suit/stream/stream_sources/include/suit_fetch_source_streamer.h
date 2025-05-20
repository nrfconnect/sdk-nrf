/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FETCH_SOURCE_STREAMER_H__
#define FETCH_SOURCE_STREAMER_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Streams an image from source to sink
 *
 * @param[in]   resource_id		Resource identifier, typically in form of URI.
 *
 * @param[in]   resource_id_length	Length of resource_id, in bytes
 *
 * @param[in]   sink			Function pointers to pass image chunk to next chunk
 *processing element. Non-'null' write_ptr is always required. Non-'null' seek_ptr may be required
 *by selected fetch sources
 *
 * @return SUIT_PLAT_SUCCESS on success, error code otherwise
 */
suit_plat_err_t suit_fetch_source_stream(const uint8_t *resource_id, size_t resource_id_length,
					 struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* FETCH_SOURCE_STREAMER_H__ */
