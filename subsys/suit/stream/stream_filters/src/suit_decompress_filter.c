/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_compress/implementation.h>
#include <suit_decompress_filter.h>
#include <suit_types.h>
#include <suit_plat_decode_util.h>
#include <zephyr/logging/log.h>
#include <LzmaDec.h>

LOG_MODULE_REGISTER(suit_decompress_filter, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_LZMA2_HEADER_LENGTH	2 /* The same as in lzma.c */
#define CHUNK_BUFFER_SIZE	(SUIT_LZMA2_HEADER_LENGTH + LZMA_REQUIRED_INPUT_MAX)

struct decompress_ctx {
	struct stream_sink out_sink;
	out_sink_readback_func out_sink_readback;
	size_t decompressed_image_size;
	size_t curr_image_offset;
	bool in_use;
	const struct nrf_compress_implementation *codec_impl;
	void *codec_ctx;
	uint8_t last_chunk[CHUNK_BUFFER_SIZE];
	uint8_t last_chunk_size;
	bool flushed;
};

static struct decompress_ctx ctx;

/**
 * @brief Interface functions for external lzma dictionary, see include/nrf_compress/lzma_types.h.
 */
static int open_dictionary(size_t dict_size, size_t *buff_size);
static int close_dictionary(void);
static size_t write_dictionary(size_t pos, const uint8_t *data, size_t len);
static size_t read_dictionary(size_t pos, uint8_t *data, size_t len);

static const lzma_dictionary_interface lzma_if = {
	.open = open_dictionary,
	.close = close_dictionary,
	.write = write_dictionary,
	.read = read_dictionary
};

static lzma_codec lzma_inst = {
	.dict_if = lzma_if
};

/*
 * Use pointer to volatile function, as stated in Percival's blog article at:
 *
 * http://www.daemonology.net/blog/2014-09-04-how-to-zero-a-buffer.html
 *
 * Although some compilers may still optimize out the memset, it is safer to use
 * some sort of trick than simply call memset.
 */
static void *(*const volatile memset_func)(void *, int, size_t) = memset;

/**
 * @brief Zeroize a buffer
 *
 * @param[out] buf Buffer to zeroize
 * @param[in] len Length of @a buf
 */
static void zeroize(void *buf, size_t len)
{
	if ((buf == NULL) || (len == 0)) {
		return;
	}

	memset_func(buf, 0, len);
}

static int open_dictionary(size_t dict_size, size_t *buff_size)
{
	/** We always return success in here. If requested dict_size is bigger than our
	 * decompressed image size we fake *buff_size to be enough, because dictionary is never
	 * accessed past the size of the decompressed data.
	 *
	 * If, for any reason, dictionary was accessed outside the bounds of output_sink memory,
	 * write_dictionary and read_dictionary interfaces would return an error and entire
	 * decompression process would exit with error.
	 */
	*buff_size = ctx.decompressed_image_size > dict_size ?
				ctx.decompressed_image_size
				: dict_size;

	return SUIT_PLAT_SUCCESS;
}

static int close_dictionary(void)
{
	return 0;
}

static size_t write_dictionary(size_t pos, const uint8_t *data, size_t len)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;

	if (ctx.curr_image_offset != pos) {
		ctx.curr_image_offset = pos;
		res = ctx.out_sink.seek(ctx.out_sink.ctx, ctx.curr_image_offset);
		if (res != SUIT_PLAT_SUCCESS) {
			return 0;
		}
	}

	res = ctx.out_sink.write(ctx.out_sink.ctx, data, len);
	ctx.curr_image_offset += len;
	return res == SUIT_PLAT_SUCCESS ? len : 0;
}

static size_t read_dictionary(size_t pos, uint8_t *data, size_t len)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;

	res = ctx.out_sink_readback(ctx.out_sink.ctx, pos, data, len);

	return res == SUIT_PLAT_SUCCESS ? len : 0;
}

static suit_plat_err_t used_storage(void *ctx, size_t *size)
{
	struct decompress_ctx *decompress_ctx = (struct decompress_ctx *)ctx;

	if ((ctx == NULL) || (size == NULL)) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (decompress_ctx->out_sink.used_storage != NULL) {
		return decompress_ctx->out_sink.used_storage(decompress_ctx->out_sink.ctx, size);
	}

	return SUIT_PLAT_ERR_UNSUPPORTED;
}

static suit_plat_err_t erase(void *ctx)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;

	if (ctx != NULL) {
		struct decompress_ctx *decompress_ctx = (struct decompress_ctx *)ctx;
		const struct nrf_compress_implementation *codec_impl;
		size_t size;
		suit_plat_err_t size_ret;

		codec_impl = decompress_ctx->codec_impl;
		(void)codec_impl->reset(decompress_ctx->codec_ctx);

		zeroize(decompress_ctx->last_chunk, sizeof(decompress_ctx->last_chunk));
		decompress_ctx->last_chunk_size = 0;

		size_ret = used_storage(decompress_ctx, &size);

		if (size_ret != SUIT_PLAT_SUCCESS) {
			return SUIT_PLAT_ERR_CRASH;
		}

		if (decompress_ctx->out_sink.erase != NULL && size != 0) {
			res = decompress_ctx->out_sink.erase(decompress_ctx->out_sink.ctx);
		}
	} else {
		res = SUIT_PLAT_ERR_INVAL;
	}

	return res;
}

static suit_plat_err_t write(void *ctx, const uint8_t *buf, size_t size)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;
	size_t chunk_size;
	int rc;
	struct decompress_ctx *decompress_ctx = (struct decompress_ctx *)ctx;
	uint8_t *output = NULL;
	size_t output_size = 0;
	uint32_t processed_size = 0;
	const struct nrf_compress_implementation *codec_impl;

	if ((ctx == NULL) || (buf == NULL) || (size == 0)) {
		LOG_ERR("Invalid arguments.");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (!decompress_ctx->in_use) {
		LOG_ERR("Decrypt filter not initialized.");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (decompress_ctx->flushed == true) {
		LOG_INF("Wrong decompression state.");
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	codec_impl = decompress_ctx->codec_impl;

	/* Make sure we buffer CHUNK_BUFFER_SIZE bytes
	 * in ctx.last_chunk for the flush operation.
	 */
	while (size + decompress_ctx->last_chunk_size > ARRAY_SIZE(decompress_ctx->last_chunk)) {
		if (decompress_ctx->last_chunk_size != 0) {
			chunk_size = MIN(decompress_ctx->last_chunk_size,
					 size + decompress_ctx->last_chunk_size
						- CHUNK_BUFFER_SIZE);

			/* Following check is for the header case
			 * - 2 bytes may be requested by the codec.
			 */
			chunk_size = MIN(chunk_size,
					 codec_impl->decompress_bytes_needed(
							decompress_ctx->codec_ctx));

			rc = codec_impl->decompress(decompress_ctx->codec_ctx,
						    decompress_ctx->last_chunk, chunk_size,
						    false, &processed_size, &output, &output_size);

			if (rc != 0 || processed_size != chunk_size) {
				LOG_ERR("Decompression data error, line: %d", __LINE__);
				res = SUIT_PLAT_ERR_CRASH;
				goto cleanup;
			}

			if (output_size != 0) {
				/* It means that we reached the end of dictionary buffer, which
				 * must not happen in our case - we cannot override it as it
				 * occupies image space.
				 */
				LOG_ERR("Too big decompressed image size.");
				res = SUIT_PLAT_ERR_CRASH;
				goto cleanup;
			}

			decompress_ctx->last_chunk_size -= chunk_size;

			if (decompress_ctx->last_chunk_size != 0) {
				/* Shift unprocessed data in last chunk to the buffer beginning. */
				memcpy(decompress_ctx->last_chunk,
				       decompress_ctx->last_chunk + chunk_size,
				       decompress_ctx->last_chunk_size);
			}
			continue;
		}

		chunk_size = MIN(size - CHUNK_BUFFER_SIZE,
				 codec_impl->decompress_bytes_needed(decompress_ctx->codec_ctx));

		rc = codec_impl->decompress(decompress_ctx->codec_ctx,
					    buf, chunk_size, false,
					    &processed_size, &output, &output_size);

		if (rc != 0 || processed_size != chunk_size) {
			LOG_ERR("Decompression data error on line: %d", __LINE__);
			res = SUIT_PLAT_ERR_CRASH;
			goto cleanup;
		}

		if (output_size != 0) {
			/* It means that we reached the end of dictionary buffer, which
			 * must not happen in our case - we cannot override it as it
			 * occupies image space.
			 */
			LOG_ERR("Too big decompressed image size.");
			res = SUIT_PLAT_ERR_CRASH;
			goto cleanup;
		}

		size -= processed_size;
		buf += processed_size;
	}

	memcpy(decompress_ctx->last_chunk + decompress_ctx->last_chunk_size, buf, size);
	decompress_ctx->last_chunk_size += size;
	return res;

cleanup:
	zeroize(decompress_ctx->last_chunk, sizeof(decompress_ctx->last_chunk));
	decompress_ctx->last_chunk_size = 0;

	return res;
}

static suit_plat_err_t flush(void *ctx)
{
	suit_plat_err_t res = SUIT_PLAT_SUCCESS;
	size_t chunk_size;
	int rc;
	uint8_t *output = NULL;
	size_t output_size = 0;
	uint32_t processed_size = 0;
	const struct nrf_compress_implementation *codec_impl;

	struct decompress_ctx *decompress_ctx = (struct decompress_ctx *)ctx;

	codec_impl = decompress_ctx->codec_impl;

	if (ctx == NULL) {
		LOG_ERR("Invalid arguments - decompress ctx is NULL");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (!decompress_ctx->in_use) {
		LOG_ERR("Decompress filter not initialized.");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (decompress_ctx->flushed == true) {
		LOG_INF("Decompress filter already flushed.");
		return SUIT_PLAT_SUCCESS;
	}

	if (decompress_ctx->last_chunk_size < CHUNK_BUFFER_SIZE) {
		LOG_WRN("Wrong decompression state.");
		res = SUIT_PLAT_ERR_INCORRECT_STATE;
		goto cleanup;
	}

	while (decompress_ctx->last_chunk_size > 0 && res == SUIT_PLAT_SUCCESS) {
		chunk_size = MIN(decompress_ctx->last_chunk_size,
				 codec_impl->decompress_bytes_needed(
							decompress_ctx->codec_ctx));

		rc = codec_impl->decompress(decompress_ctx->codec_ctx,
					    decompress_ctx->last_chunk, chunk_size, true,
					    &processed_size, &output, &output_size);

		if (rc != 0 || processed_size != chunk_size) {
			LOG_ERR("Decompression data error");
			res = SUIT_PLAT_ERR_CRASH;
			break;
		}

		decompress_ctx->last_chunk_size -= chunk_size;

		if (decompress_ctx->last_chunk_size != 0) {
			/* Shift unprocessed data in last chunk to the buffer beginning. */
			memcpy(decompress_ctx->last_chunk,
			       decompress_ctx->last_chunk + chunk_size,
			       decompress_ctx->last_chunk_size);
		}
	}

	if (output_size != decompress_ctx->decompressed_image_size) {
		res = SUIT_PLAT_ERR_CRASH;
	}

	(void)codec_impl->reset(decompress_ctx->codec_ctx);

	if (res == SUIT_PLAT_SUCCESS) {
		LOG_INF("Firmware decompression successful");
		if (decompress_ctx->out_sink.flush != NULL) {
			decompress_ctx->out_sink.flush(decompress_ctx->out_sink.ctx);
		}
	} else {
		goto cleanup;
	}

	zeroize(decompress_ctx->last_chunk, sizeof(decompress_ctx->last_chunk));
	decompress_ctx->flushed = true;

	return res;

cleanup:

	decompress_ctx->flushed = true;
	(void)erase(decompress_ctx);

	return res;
}

static suit_plat_err_t release(void *ctx)
{
	struct decompress_ctx *decompress_ctx = (struct decompress_ctx *)ctx;
	const struct nrf_compress_implementation *codec_impl;

	if (ctx == NULL) {
		LOG_ERR("Invalid arguments - decompress ctx is NULL");
		return SUIT_PLAT_ERR_INVAL;
	}

	codec_impl = decompress_ctx->codec_impl;

	suit_plat_err_t res = flush(ctx);

	if (decompress_ctx->out_sink.release != NULL) {
		suit_plat_err_t release_ret =
			decompress_ctx->out_sink.release(decompress_ctx->out_sink.ctx);

		if (res == SUIT_SUCCESS) {
			res = release_ret;
		}
	}

	codec_impl->deinit(decompress_ctx->codec_ctx);
	zeroize(decompress_ctx, sizeof(struct decompress_ctx));

	return res;
}

static suit_plat_err_t validate_decompression(const struct suit_compression_info *compress_info,
					      enum nrf_compress_types *compress_type)
{
	switch (compress_info->compression_alg_id) {
	case suit_lzma2:
		*compress_type = NRF_COMPRESS_TYPE_LZMA;
		ctx.codec_ctx = &lzma_inst;
		break;
	default:
		LOG_ERR("Unsupported decompression algorithm: %d",
			compress_info->compression_alg_id);
		return SUIT_PLAT_ERR_INVAL;
	}
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_decompress_filter_get(struct stream_sink *in_sink,
					   const struct suit_compression_info *compress_info,
					   const struct stream_sink *out_sink,
					   out_sink_readback_func out_sink_readback)
{
	int rc;
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	enum nrf_compress_types compress_type = NRF_COMPRESS_TYPE_COUNT;

	if (ctx.in_use) {
		LOG_ERR("The decompression filter is busy");
		return SUIT_PLAT_ERR_BUSY;
	}

	if (compress_info == NULL || out_sink == NULL || in_sink == NULL ||
	    out_sink_readback == NULL  || out_sink->write == NULL ||
	    out_sink->used_storage == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	ret = validate_decompression(compress_info, &compress_type);
	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	ctx.codec_impl = nrf_compress_implementation_find(compress_type);
	if (ctx.codec_impl == NULL) {
		LOG_ERR("Could not find codec implementation for selected compression type");
		return SUIT_PLAT_ERR_CRASH;
	}

	rc = ctx.codec_impl->init(ctx.codec_ctx);
	if (rc != 0) {
		LOG_ERR("Failed to initialize lzma codec");
		ctx.codec_impl = NULL;
		ctx.codec_ctx = NULL;
		return SUIT_PLAT_ERR_CRASH;
	}

	ctx.in_use = true;
	memcpy(&ctx.out_sink, out_sink, sizeof(struct stream_sink));
	ctx.out_sink_readback = out_sink_readback;
	ctx.decompressed_image_size = compress_info->decompressed_image_size;
	in_sink->ctx = &ctx;
	in_sink->write = write;
	in_sink->erase = erase;
	in_sink->release = release;
	in_sink->flush = flush;
	in_sink->used_storage = used_storage;

	/* Seeking is not possible on compressed payload. */
	in_sink->seek = NULL;

	return SUIT_PLAT_SUCCESS;
}
