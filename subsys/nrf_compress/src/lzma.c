/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <LzmaDec.h>
#include <Lzma2Dec.h>
#include <nrf_compress/implementation.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_compress_lzma, CONFIG_NRF_COMPRESS_LOG_LEVEL);

/* Header size for lzma2 */
#define LZMA2_HEADER_SIZE 2

/* Assume lp and lc parameters in the compressed images do not sum to a value greater than the
 * following constant to limit the memory used by LZMA probability array.
 */
#define MAX_LZMA_LC_PLUS_LP 4
#define MAX_LZMA_PROB_SIZE  (1984 + (0x300 << MAX_LZMA_LC_PLUS_LP))

/* Assume the maximum LZMA dictionary size to limit the RAM buffer size for the decompressed
 * stream.
 */
#define MAX_LZMA_DICT_SIZE  (128 * 1024)

#if !defined(CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA1) && \
	!defined(CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2)
#error "Missing selection of lzma algorithm selection, please select " \
	"CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA1 or CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2"
#endif

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_STATIC)
static uint16_t lzma_probs[MAX_LZMA_PROB_SIZE];
#endif

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC) && defined(CONFIG_NRF_COMPRESS_CLEANUP)
static size_t malloc_probs_size = 0;
#endif

static void *lzma_probs_alloc(ISzAllocPtr p, size_t size)
{
#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_STATIC)
	if (size > sizeof(lzma_probs)) {
		LOG_ERR("Compress library tried to allocate too large a buffer (0x%x)", size);
		return NULL;
	}

	return lzma_probs;
#else
	void *buffer = malloc(size);

	if (buffer == NULL) {
		LOG_ERR("Failed to allocate nRF compression library buffer (0x%x)", size);
	}

#ifdef CONFIG_NRF_COMPRESS_CLEANUP
	malloc_probs_size = size;
#endif

	return buffer;
#endif
}

static void lzma_probs_free(ISzAllocPtr p, void *address)
{
#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
#ifdef CONFIG_NRF_COMPRESS_CLEANUP
	memset(address, 0x00, malloc_probs_size);

	malloc_probs_size = 0;
#endif
	free(address);
#else
#ifdef CONFIG_NRF_COMPRESS_CLEANUP
	memset(lzma_probs, 0x00, sizeof(lzma_probs));
#endif
#endif
}

const static ISzAlloc lzma_probs_allocator = {
	.Alloc = lzma_probs_alloc,
	.Free = lzma_probs_free,
};

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_STATIC)
#if CONFIG_NRF_COMPRESS_MEMORY_ALIGNMENT > 1
static uint8_t __aligned(CONFIG_NRF_COMPRESS_MEMORY_ALIGNMENT) lzma_dict[MAX_LZMA_DICT_SIZE];
#else
static uint8_t lzma_dict[MAX_LZMA_DICT_SIZE];
#endif
#else
static uint8_t *lzma_dict = NULL;
#endif

static bool allocated_probs = false;
#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
static CLzma2Dec lzma_decoder;
#else
static CLzmaDec lzma_decoder;
#endif

static int lzma_reset(void *inst);

static int lzma_init(void *inst)
{
	int rc = 0;

	ARG_UNUSED(inst);

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
	if (lzma_dict != NULL) {
		/* Already allocated */
		lzma_reset(inst);

		return rc;
	}

#if CONFIG_NRF_COMPRESS_MEMORY_ALIGNMENT > 1
	lzma_dict = (uint8_t *)aligned_alloc(CONFIG_NRF_COMPRESS_MEMORY_ALIGNMENT,
					     MAX_LZMA_DICT_SIZE);
#else
	lzma_dict = (uint8_t *)malloc(MAX_LZMA_DICT_SIZE);
#endif

	if (lzma_dict == NULL) {
		rc = -ENOMEM;
	}
#endif

	return rc;
}

static int lzma_deinit(void *inst)
{
	ARG_UNUSED(inst);

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
	if (lzma_dict != NULL) {
#ifdef CONFIG_NRF_COMPRESS_CLEANUP
		memset(lzma_dict, 0x00, MAX_LZMA_DICT_SIZE);
#endif

		free(lzma_dict);
		lzma_dict = NULL;
	}
#endif

	lzma_reset(inst);

	return 0;
}

static int lzma_reset(void *inst)
{
	ARG_UNUSED(inst);

	if (allocated_probs) {
		allocated_probs = false;

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
		Lzma2Dec_FreeProbs(&lzma_decoder, &lzma_probs_allocator);
#else
		LzmaDec_FreeProbs(&lzma_decoder, &lzma_probs_allocator);
#endif

		lzma_decoder.decoder.dicPos = 0;
	}

	return 0;
}

static size_t lzma_bytes_needed(void *inst)
{
	ARG_UNUSED(inst);

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
	if (lzma_dict == NULL) {
		return 0;
	}
#endif

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
	return (allocated_probs ? CONFIG_NRF_COMPRESS_CHUNK_SIZE : LZMA2_HEADER_SIZE);
#else
	return (allocated_probs ? CONFIG_NRF_COMPRESS_CHUNK_SIZE : LZMA_PROPS_SIZE);
#endif
}

static int lzma_decompress(void *inst, const uint8_t *input, size_t input_size, bool last_part,
			   uint32_t *offset, uint8_t **output, size_t *output_size)
{
	int rc;
	ELzmaStatus status;
	size_t chunk_size = input_size;

	ARG_UNUSED(inst);

#if defined(CONFIG_NRF_COMPRESS_MEMORY_TYPE_MALLOC)
	if (lzma_dict == NULL) {
		return -ESRCH;
	}
#endif

	if (input == NULL || input_size == 0 || offset == NULL || output == NULL ||
	    output_size == NULL) {
		return -EINVAL;
	}

	*output = NULL;
	*output_size = 0;

	if (!allocated_probs) {
#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
		rc = Lzma2Dec_AllocateProbs(&lzma_decoder, input[0], &lzma_probs_allocator);
#else
		rc = LzmaDec_AllocateProbs(&lzma_decoder, input, LZMA_PROPS_SIZE,
					   &lzma_probs_allocator);
#endif

		if (rc) {
			rc = -EINVAL;
			goto done;
		}

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
		if (lzma_decoder.decoder.prop.dicSize > MAX_LZMA_DICT_SIZE) {
#else
		if (lzma_decoder.prop.dicSize > MAX_LZMA_DICT_SIZE) {
#endif
			rc = -EINVAL;
#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
			Lzma2Dec_FreeProbs(&lzma_decoder, &lzma_probs_allocator);
#else
			LzmaDec_FreeProbs(&lzma_decoder, &lzma_probs_allocator);
#endif
			goto done;
		}

		allocated_probs = true;
#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
		*offset = LZMA2_HEADER_SIZE;
#else
		/* Header and account for uncompressed size */
		*offset = LZMA_PROPS_SIZE + sizeof(uint64_t);
#endif

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
		lzma_decoder.decoder.dic = lzma_dict;
		lzma_decoder.decoder.dicBufSize = MAX_LZMA_DICT_SIZE;
		Lzma2Dec_Init(&lzma_decoder);
#else
		lzma_decoder.dic = lzma_dict;
		lzma_decoder.dicBufSize = MAX_LZMA_DICT_SIZE;
		LzmaDec_Init(&lzma_decoder);
#endif

		return 0;
	}

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
	rc = Lzma2Dec_DecodeToDic(&lzma_decoder, MAX_LZMA_DICT_SIZE, input, &chunk_size,
					(last_part ? LZMA_FINISH_END : LZMA_FINISH_ANY), &status);
#else
	rc = LzmaDec_DecodeToDic(&lzma_decoder, MAX_LZMA_DICT_SIZE, input, &chunk_size,
					(last_part ? LZMA_FINISH_END : LZMA_FINISH_ANY), &status);
#endif

	if (rc) {
		rc = -EINVAL;
		goto done;
	}

	*offset = chunk_size;

	if (last_part && (status == LZMA_STATUS_FINISHED_WITH_MARK ||
			  status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) &&
	    *offset < input_size) {
		/* If last block, ensure offset matches complete file size */
		*offset = input_size;
	}

#ifdef CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2
	if (lzma_decoder.decoder.dicPos >= lzma_decoder.decoder.dicBufSize ||
	    (last_part && input_size == chunk_size)) {
		*output = lzma_decoder.decoder.dic;
		*output_size = lzma_decoder.decoder.dicPos;
		lzma_decoder.decoder.dicPos = 0;
	}
#else
	if (lzma_decoder.dicPos >= lzma_decoder.dicBufSize ||
	    (last_part && input_size == chunk_size)) {
		*output = lzma_decoder.dic;
		*output_size = lzma_decoder.dicPos;
		lzma_decoder.dicPos = 0;
	}
#endif

done:
	return rc;
}

NRF_COMPRESS_IMPLEMENTATION_DEFINE(lzma, NRF_COMPRESS_TYPE_LZMA, lzma_init, lzma_deinit,
				   lzma_reset, NULL, lzma_bytes_needed, lzma_decompress);
