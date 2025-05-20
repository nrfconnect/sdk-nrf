/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SINK_H__
#define SINK_H__

#include <stdio.h>
#include <stdint.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef suit_plat_err_t (*erase_ptr)(void *ctx);
typedef suit_plat_err_t (*write_ptr)(void *ctx, const uint8_t *buf, size_t size);
typedef suit_plat_err_t (*seek_ptr)(void *ctx, size_t offset);
typedef suit_plat_err_t (*flush_ptr)(void *ctx);
typedef suit_plat_err_t (*used_storage_ptr)(void *ctx, size_t *size);
typedef suit_plat_err_t (*release_ptr)(void *ctx);

/**
 * @brief Structure represents node that is a target for data.
 *    The idea is to be able to create chain of such links/filters to process and write data.
 *    Structure can represent for example:
 *    - decrypting or decompressing entity and being an intermediate block.
 *    - a writer, presumably a final block.
 */
struct stream_sink {
	erase_ptr erase;
	write_ptr write;
	seek_ptr seek;
	flush_ptr flush;
	used_storage_ptr used_storage;
	release_ptr release;

	void *ctx; /* context used by specific sink implementation */
};

/**
 * @brief Helper function for releasing sink
 *
 * @param sink Sink to be released
 * @return suit_plat_err_t
 */
static inline suit_plat_err_t release_sink(struct stream_sink *sink)
{
	if (sink != NULL) {
		if (sink->release != NULL) {
			return sink->release(sink->ctx);
		}

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}

#ifdef __cplusplus
}
#endif

#endif /* SINK_H__ */
