/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/aes/cracen_aes_ecb.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>
#include <cracen/statuscodes.h>
#include <cracen_psa.h>
#include <cracen/common.h>
#include <cracen_psa_primitives.h>

/* The AES ECB does not support multipart operations in Cracen. This means that keeping
 * the state between calls is not supported. This function is using the single part
 * APIs of Cracen to perform the AES ECB operations.
 */
static psa_status_t cracen_aes_ecb_crypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
			      const uint8_t *input, size_t input_length, uint8_t *output,
			      size_t output_size, size_t *output_length, bool encrypt)
{
	int sx_status;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	if (encrypt) {
		sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	} else {
		sx_status = sx_blkcipher_create_aesecb_dec(blkciph, key);
	}

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

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

psa_status_t cracen_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length)
{
	return cracen_aes_ecb_crypt(blkciph, key, input, input_length,
				    output, output_size, output_length,
				    true);
}

psa_status_t cracen_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length)
{
	return cracen_aes_ecb_crypt(blkciph, key, input, input_length,
				    output, output_size, output_length,
				    false);
}
