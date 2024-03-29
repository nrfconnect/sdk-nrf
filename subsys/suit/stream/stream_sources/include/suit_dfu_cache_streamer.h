/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CACHE_FETCH_H__
#define CACHE_FETCH_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stream payload from cache to sink
 *
 * @param cache Pointer to SUIT cache
 * @param uri URI to be found in cache - source
 * @param uri_size Uri size
 * @param sink Pointer to sink that will write payload - target
 * @return SUIT_PLAT_SUCCESS if success otherwise error code
 */
suit_plat_err_t suit_dfu_cache_streamer_stream(const uint8_t *uri, size_t uri_size,
					       struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* CACHE_FETCH_H__ */
