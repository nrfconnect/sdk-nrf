/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_DECOMPRESS_FILTER_H__
#define SUIT_DECOMPRESS_FILTER_H__

#include <suit_sink.h>
#include <suit_types.h>
#include <suit_metadata.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interface function type for output sink readback, passed to suit_decompress_filter_get()
 * as a callback. Decompression process needs access to unaltered destination component memory
 * (see WARNING below).
 *
 * @param[in] sink_ctx Context of decompression output sink
 * @param[in] offset Offset of output sink area to start reading from
 * @param[out] buf Buffer to read into
 * @param[in] size Size of @a buf; data read size
 *
 * @retval SUIT_PLAT_SUCCESS on success
 * @retval SUIT_PLAT_ERR_INVAL if invalid argument passed
 * @retval SUIT_PLAT_ERR_IO if I/O access error
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS on read out of bounds
 */
typedef suit_plat_err_t (*out_sink_readback_func)(void *sink_ctx, size_t offset, uint8_t *buf,
						  size_t size);

/**
 * @brief Get decompress filter object.
 * WARNING: Decompression filter must be the last in a filter chain because it needs to have access
 * to unaltered destination component memory during streaming.
 *
 * @param[out] in_sink   Pointer to input stream_sink to pass compressed data
 * @param[in]  compress_info  Pointer to the structure with compression info
 * @param[in]  out_sink  Pointer to output stream_sink to be filled with decompressed data
 * @param[in]  out_sink_readback Output sink readback function
 *
 * @retval SUIT_PLAT_SUCCESS on success
 * @retval SUIT_PLAT_ERR_BUSY if filter is already in use
 * @retval SUIT_PLAT_ERR_INVAL if invalid argument passed
 * @retval SUIT_PLAT_ERR_CRASH on error during decompression initialization
 */
suit_plat_err_t suit_decompress_filter_get(struct stream_sink *in_sink,
					   const struct suit_compression_info *compress_info,
					   const struct stream_sink *out_sink,
					   out_sink_readback_func out_sink_readback);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_DECOMPRESS_FILTER_H__ */
