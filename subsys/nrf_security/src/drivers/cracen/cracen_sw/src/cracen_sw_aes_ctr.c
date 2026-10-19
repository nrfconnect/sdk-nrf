/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_security_mem_helpers.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <string.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/util.h>

#include <cracen_psa_primitives.h>
#include <cracen/common.h>
#include <cracen_sw_common.h>
#include <cracen_sw_aes_ctr.h>

/* AES CTR mode counter field is the entire 16-byte block */
#define AES_CTR_COUNTER_START_BYTE 0

psa_status_t cracen_sw_aes_ctr_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg)
{
	psa_status_t status;

	/* Verify key buffer fits in operation structure */
	if (key_buffer_size > sizeof(operation->key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Copy the key into the operation struct */
	memcpy(operation->key_buffer, key_buffer, key_buffer_size);

	/* Load the key reference for ECB operations */
	status = cracen_load_keyref(attributes, operation->key_buffer, key_buffer_size,
				    &operation->keyref);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* CCM* shares this path; only set_iv() differs. */
	operation->alg = alg;
	operation->dir = CRACEN_ENCRYPT; /* CTR mode is identical for encrypt/decrypt */
	operation->blk_size = SX_BLKCIPHER_AES_BLK_SZ;
	operation->unprocessed_input_bytes = 0;
	operation->processed_length = 0;
	/* We don't consider the initalization finalized until IV is set */
	operation->initialized = false;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ctr_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_CCM_STAR_NO_TAG_AES) &&
	    operation->alg == PSA_ALG_CCM_STAR_NO_TAG) {
		if (iv_length != CCM_STAR_NONCE_LENGTH) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		/* Seed with A_1; the counter walk in update() derives A_2, A_3, ... */
		cracen_ccm_star_build_a1(operation->iv, iv);
	} else if (operation->alg == PSA_ALG_CTR) {
		if (iv_length != SX_BLKCIPHER_AES_BLK_SZ) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		/* The IV includes the initial counter */
		memcpy(operation->iv, iv, iv_length);
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	operation->unprocessed_input_bytes = 0;
	operation->initialized = true;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ctr_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
{
	/* unprocessed_input holds the keystream block, unprocessed_input_bytes how much of it
	 * is used (0 when none is left, as after set_iv())
	 */
	size_t keystream_offset = operation->unprocessed_input_bytes != 0
					  ? operation->unprocessed_input_bytes
					  : SX_BLKCIPHER_AES_BLK_SZ;
	psa_status_t status;

	*output_length = 0;

	if (!operation->initialized) {
		return PSA_ERROR_BAD_STATE;
	}

	/* Valid operation, we just don't do anything */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	/* CTR is a stream mode operation so output must be same length as input */
	if (output == NULL || output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = cracen_ccm_star_add_length(operation, input_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* CRACEN stays reserved for all blocks of this call */
	status = silex_statuscodes_to_psa(
		sx_hw_reserve(&operation->cipher.dma, SX_HW_RESERVE_CM_ENABLED));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_sw_ctr_xor(&operation->cipher, &operation->keyref, operation->iv,
				   AES_CTR_COUNTER_START_BYTE, operation->unprocessed_input,
				   &keystream_offset, input, output, input_length);

	sx_hw_release(&operation->cipher.dma);
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->unprocessed_input_bytes = keystream_offset % SX_BLKCIPHER_AES_BLK_SZ;
	*output_length = input_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ctr_finish(cracen_cipher_operation_t *operation,
				      size_t *output_length)
{
	*output_length = 0;

	if (!operation->initialized) {
		return PSA_ERROR_BAD_STATE;
	}

	/* CTR is a stream mode operation all data is produced during update(). */
	operation->unprocessed_input_bytes = 0;

	memset(operation->unprocessed_input, 0, SX_BLKCIPHER_AES_BLK_SZ);

	operation->initialized = false;

	return PSA_SUCCESS;
}

/* Single Shot Crypt which handles both encryption and decryption */
psa_status_t cracen_sw_aes_ctr_crypt(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				     const uint8_t *input, size_t input_length, uint8_t *output,
				     size_t output_size, size_t *output_length)
{
	psa_status_t status;
	cracen_cipher_operation_t operation = {0};
	size_t finish_length;

	if (input_length > 0 && input == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_size > 0 && output == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	status = cracen_sw_aes_ctr_setup(&operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_sw_aes_ctr_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_sw_aes_ctr_update(&operation, input, input_length, output, output_size,
					  output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	status = cracen_sw_aes_ctr_finish(&operation, &finish_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	*output_length += finish_length;

exit:
	safe_memzero(&operation, sizeof(operation));
	return status;
}
