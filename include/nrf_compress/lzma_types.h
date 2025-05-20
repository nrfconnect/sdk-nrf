/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief LZMA API types for compression/decompression subsystem
 */

#ifndef NRF_COMPRESS_LZMA_TYPES_H_
#define NRF_COMPRESS_LZMA_TYPES_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef		lzma_dictionary_open_func_t
 * @brief		Open dictionary interface. It is up to the user
 *			how big is the dictionary buffer, but it must be
 *			at least @a dict_size long.
 *
 * @param[in]		dict_size dictionary size; minimum length of the
 *			buffer that is requested.
 * @param[out]		buff_size size of the dictionary buffer provided.
 *
 * @retval		0 Success.
 * @retval		-ENOMEM if cannot provide requested size. @a buff_size is valid.
 * @retval		-errno Negative errno code on other failure @a buff_size is invalid.
 */
typedef int (*lzma_dictionary_open_func_t)(size_t dict_size, size_t *buff_size);

/**
 * @typedef		lzma_dictionary_close_func_t
 * @brief		Close dictionary interface.
 *
 * @retval		0 Success.
 * @retval		-errno Negative errno code on other failure.
 */
typedef int (*lzma_dictionary_close_func_t)(void);

/**
 * @typedef		lzma_dictionary_write_func_t
 * @brief		Write dictionary interface.
 *
 * @param[in]		pos Position (byte-wise) of the dictionary to start writing to.
 * @param[in]		data Data to write.
 * @param[in]		len Length of @a data buffer.
 *
 * @retval		Number of written bytes to the dictionary (length).
 */
typedef size_t (*lzma_dictionary_write_func_t)(size_t pos, const uint8_t *data, size_t len);

/**
 * @typedef		lzma_dictionary_read_func_t
 * @brief		Read dictionary interface.
 *
 * @param[in]		pos Position of the dictionary to start reading from.
 * @param[in]		data Data buffer to read into.
 * @param[in]		len Length of @a data buffer, number of bytes to read.
 *
 * @retval		Number of bytes read from the dictionary (length).
 */
typedef size_t (*lzma_dictionary_read_func_t)(size_t pos, uint8_t *data, size_t len);

typedef struct lzma_dictionary_interface_t {
	const lzma_dictionary_open_func_t open;
	const lzma_dictionary_close_func_t close;
	const lzma_dictionary_write_func_t write;
	const lzma_dictionary_read_func_t read;
} lzma_dictionary_interface;

/**
 * @brief This is an initialization context struct type. Instantionize and pass it to
 * interface functions like for e.g. nrf_compress_init_func_t, nrf_compress_decompress_func_t.
 */
typedef struct lzma_codec_t {
	const lzma_dictionary_interface dict_if;
} lzma_codec;

#ifdef __cplusplus
}
#endif

#endif /* NRF_COMPRESS_IMPLEMENTATION_H_ */
