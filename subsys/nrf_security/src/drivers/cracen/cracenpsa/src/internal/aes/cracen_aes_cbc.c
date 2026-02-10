/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/aes/cracen_aes_cbc.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

psa_status_t cracen_aes_cbc_encrypt(const struct sxkeyref *key, const uint8_t *input,
				    size_t input_length, uint8_t *output, size_t output_size,
				    size_t *output_length, const uint8_t *iv)
{
	int sx_status;
	struct sxblkcipher cipher_ctx;
	uint8_t padded_input_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t remaining_bytes = input_length % SX_BLKCIPHER_AES_BLK_SZ;
	uint8_t padding = SX_BLKCIPHER_AES_BLK_SZ - remaining_bytes;
	size_t padded_input_length = input_length + padding;
	size_t full_blocks_length = input_length - remaining_bytes;

	if (output_size < padded_input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_status = sx_blkcipher_create_aescbc_enc(&cipher_ctx, key, iv);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	if (full_blocks_length > 0) {
		sx_status = sx_blkcipher_crypt(&cipher_ctx, input, full_blocks_length, output);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}
	}

	memset(padded_input_block, padding, sizeof(padded_input_block));
	memcpy(padded_input_block, input + full_blocks_length, remaining_bytes);

	sx_status = sx_blkcipher_crypt(&cipher_ctx, padded_input_block, sizeof(padded_input_block),
				       output + full_blocks_length);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&cipher_ctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&cipher_ctx);
	if (sx_status == SX_OK) {
		*output_length = padded_input_length;
	}

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_aes_cbc_decrypt(const struct sxkeyref *key, const uint8_t *input,
				    size_t input_length, uint8_t *output, size_t output_size,
				    size_t *output_length, const uint8_t *iv)
{
	int sx_status;
	struct sxblkcipher cipher_ctx;

	if (input_length % SX_BLKCIPHER_AES_BLK_SZ != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_status = sx_blkcipher_create_aescbc_dec(&cipher_ctx, key, iv);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_crypt(&cipher_ctx, input, input_length, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&cipher_ctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&cipher_ctx);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	size_t padding_length = output[input_length - 1];
	size_t padding_index = input_length - padding_length;
	uint32_t failure = 0;

	failure |= (padding_length > SX_BLKCIPHER_AES_BLK_SZ);
	failure |= (padding_length == 0);

	for (size_t i = 0; i < input_length; i++) {
		failure |= (output[i] ^ padding_length) * (i >= padding_index);
	}

	*output_length = padding_index;

	return (failure == 0) ? PSA_SUCCESS : PSA_ERROR_INVALID_PADDING;
}
