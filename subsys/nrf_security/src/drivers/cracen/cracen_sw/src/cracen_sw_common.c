/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/keyref.h>
#include <sxsymcrypt/aes.h>
#include <cracen/statuscodes.h>
#include <zephyr/logging/log.h>
#include <cracen/common.h>
#include <cracen_sw_common.h>

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);

psa_status_t cracen_sw_aes_ecb_crypt(struct sxblkcipher *blkciph,
				     const uint8_t *input, size_t input_length, uint8_t *output,
				     size_t output_size, size_t *output_length)
{
	int sx_status;

	sx_status = sx_blkcipher_crypt(blkciph, input, input_length, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(blkciph);
	if (sx_status == SX_OK) {
		*output_length = input_length;
	}
	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_sw_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	int sx_status;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return cracen_sw_aes_ecb_crypt(blkciph, input, input_length, output, output_size,
				       output_length);
}

psa_status_t cracen_sw_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	int sx_status;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	sx_status = sx_blkcipher_create_aesecb_dec(blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return cracen_sw_aes_ecb_crypt(blkciph, input, input_length, output, output_size,
				       output_length);
}

psa_status_t cracen_sw_aes_primitive(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				     const uint8_t *input, uint8_t *output)
{
	int sx_status;
	size_t output_size;

	sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return cracen_sw_aes_ecb_crypt(blkciph, input, SX_BLKCIPHER_AES_BLK_SZ, output,
				       SX_BLKCIPHER_AES_BLK_SZ, &output_size);
}

psa_status_t cracen_sw_increment_counter_be(uint8_t *ctr_buf, size_t ctr_buf_size, size_t start_pos)
{
	for (size_t i = ctr_buf_size; i > start_pos; i--) {
		if (++ctr_buf[i - 1] != 0) {
			return PSA_SUCCESS;
		}
	}

	/* All counter bytes wrapped to zero which means it overflowed */
	return PSA_ERROR_INVALID_ARGUMENT;
}

void cracen_sw_encode_value_be(uint8_t *buffer, size_t buffer_size, size_t value,
			       size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[buffer_size - 1 - i] = value >> (i * 8);
	}
}
