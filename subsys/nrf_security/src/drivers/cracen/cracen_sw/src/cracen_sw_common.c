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
#include "../../../cracenpsa/src/common.h"
#include <cracen_sw_common.h>

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);

psa_status_t cracen_aes_ecb_crypt(struct sxblkcipher *blkciph,
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
	return sx_status;
}

psa_status_t cracen_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
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

	return cracen_aes_ecb_crypt(blkciph, input, input_length, output, output_size,
				    output_length);
}

psa_status_t cracen_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
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

	return cracen_aes_ecb_crypt(blkciph, input, input_length, output, output_size,
				    output_length);
}

psa_status_t cracen_aes_primitive(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				  const uint8_t *input, uint8_t *output)
{
	int sx_status;
	size_t output_size;

	sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return cracen_aes_ecb_crypt(blkciph, input, SX_BLKCIPHER_AES_BLK_SZ, output,
				    SX_BLKCIPHER_AES_BLK_SZ, &output_size);
}
