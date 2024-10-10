/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Public API for compression/decompression subsystem
 */

#ifndef NRF_COMPRESS_IMPLEMENTATION_H_
#define NRF_COMPRESS_IMPLEMENTATION_H_

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief Compression/decompression subsystem
 * @defgroup compression_decompression_subsystem Compression/decompression subsystem
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef		nrf_compress_init_func_t
 * @brief		Initialize compression implementation.
 *
 * @param[in] inst	Reserved for future use, must be NULL.
 *
 * @retval		0 Success.
 * @retval		-errno Negative errno code on other failure.
 */
typedef int (*nrf_compress_init_func_t)(void *inst);

/**
 * @typedef		nrf_compress_deinit_func_t
 * @brief		De-initialize compression implementation.
 *
 * @param[in] inst	Reserved for future use, must be NULL.
 *
 * @retval		0 Success.
 * @retval		-errno Negative errno code on other failure.
 */
typedef int (*nrf_compress_deinit_func_t)(void *inst);

/**
 * @typedef		nrf_compress_reset_func_t
 * @brief		Reset compression state function. Used to abort current compression or
 *			decompression task before starting a new one.
 *
 * @param[in] inst	Reserved for future use, must be NULL.
 *
 * @retval		0 Success.
 * @retval		-errno Negative errno code on other failure.
 */
typedef int (*nrf_compress_reset_func_t)(void *inst);

/**
 * @typedef		nrf_compress_compress_func_t
 * @brief		Placeholder function for future use, do not use.
 *
 * @param[in] inst	Reserved for future use, must be NULL.
 *
 * @retval		0 Success.
 * @retval		-errno Negative errno code on other failure.
 */
typedef int (*nrf_compress_compress_func_t)(void *inst);

/**
 * @brief		Return chunk size of data to provide to next call of
 *			#nrf_compress_decompress_func_t function. This is the ideal amount of data
 *			that should be provided to the next function call. Less data than this may
 *			be provided if more is not available (for example, end of data or data is
 *			is being streamed).
 *
 * @param[in] inst	Reserved for future use, must be NULL.
 *
 * @retval		Positive value Success indicating chunk size.
 * @retval		-errno Negative errno code on other failure.
 */
typedef size_t (*nrf_compress_decompress_bytes_needed_t)(void *inst);

/**
 * @brief			Decompress portion of compressed data. This function will need to
 *				be called one or more times with compressed data to decompress it
 *				into its natural form.
 *
 * @param[in] inst		Reserved for future use, must be NULL.
 * @param[in] input		Input data buffer, containing the compressed data.
 * @param[in] input_size	Size of the input data buffer.
 * @param[in] last_part		Last part of compressed data. This should be set to true if this is
 *				the final part of the input data.
 * @param[out] offset		Input data offset pointer. This will be updated with the amount of
 *				bytes used from the input buffer. If this is not the last
 *				decompression call, then the next call to this function should be
 *				offset the input data buffer by this amount of bytes.
 * @param[out] output		Output data buffer pointer to pointer. This will be set to the
 *				compression's output buffer when decompressed data is available to
 *				be used or copied.
 * @param[out] output_size	Size of data in output data buffer pointer. Data should only be
 *				read when the value in this pointer is greater than 0.
 *
 * @retval			0 Success.
 * @retval			-errno Negative errno code on other failure.
 */
typedef int (*nrf_compress_decompress_func_t)(void *inst, const uint8_t *input, size_t input_size,
					      bool last_part, uint32_t *offset, uint8_t **output,
					      size_t *output_size);

/** @brief Supported compression types */
enum nrf_compress_types {
	/** lzma1 or lzma2 */
	NRF_COMPRESS_TYPE_LZMA,

	/** ARM thumb filter */
	NRF_COMPRESS_TYPE_ARM_THUMB,

	/** Marks end/count of nRF supported filters */
	NRF_COMPRESS_TYPE_COUNT,

	/** Start of freely usable IDs with custom out-of-tree implementations */
	NRF_COMPRESS_TYPE_USER_CUSTOM_START = 32768
};

struct nrf_compress_implementation {
	/** @brief ID of implementation #nrf_compress_types. */
	const uint16_t id;

	const nrf_compress_init_func_t init;
	const nrf_compress_deinit_func_t deinit;
	const nrf_compress_reset_func_t reset;

#if defined(CONFIG_NRF_COMPRESS_COMPRESSION) || defined(__DOXYGEN__)
	const nrf_compress_compress_func_t compress;
#endif

#if defined(CONFIG_NRF_COMPRESS_DECOMPRESSION) || defined(__DOXYGEN__)
	const nrf_compress_decompress_bytes_needed_t decompress_bytes_needed;
	const nrf_compress_decompress_func_t decompress;
#endif
};

/**
 * @brief				Define a compression implementation.
 *					This adds a new entry to the iterable section linker list
 *					of compression implementations.
 *
 * @param name				Name of the compression type.
 * @param _id				ID of the compression type #nrf_compress_types.
 * @param _init				Initialization function #nrf_compress_init_func_t.
 * @param _deinit			Deinitialization function
 *					#nrf_compress_deinit_func_t.
 * @param _reset			Reset function #nrf_compress_reset_func_t.
 * @param _compress			Compress function or NULL if no compression support
 *					#nrf_compress_compress_func_t.
 * @param _decompress_bytes_needed	Decompression bytes needed function or NULL if no
 *					decompression support
 *					#nrf_compress_decompress_bytes_needed_t.
 * @param _decompress			Decompression function or NULL if no decompression support
 *					#nrf_compress_decompress_func_t.
 */
#define NRF_COMPRESS_IMPLEMENTATION_DEFINE(name, _id, _init, _deinit, _reset, _compress,	\
					   _decompress_bytes_needed, _decompress)		\
	STRUCT_SECTION_ITERABLE(nrf_compress_implementation, name) = {				\
		.id = _id,									\
		.init = _init,									\
		.deinit = _deinit,								\
		.reset = _reset,								\
		COND_CODE_1(CONFIG_NRF_COMPRESS_COMPRESSION, (					\
				.compress = _compress,						\
			    ), ())								\
		COND_CODE_1(CONFIG_NRF_COMPRESS_DECOMPRESSION, (				\
				.decompress_bytes_needed = _decompress_bytes_needed,		\
				.decompress = _decompress,					\
			    ), ())								\
	}

/**
 * @brief		Find a compression implementation.
 *
 * @param[in] id	Type of compression #nrf_compress_types.
 *
 * @retval		non-NULL Success.
 * @retval		NULL Compression type not found/supported.
 */
struct nrf_compress_implementation *nrf_compress_implementation_find(uint16_t id);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NRF_COMPRESS_IMPLEMENTATION_H_ */
