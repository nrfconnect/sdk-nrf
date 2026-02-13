/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
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
#define AES_BLOCK_LAST_BYTE_INDEX  (SX_BLKCIPHER_AES_BLK_SZ - 1)
#define AES_CTR_COUNTER_START_BYTE 0

static void increment_counter(uint8_t *ctr)
{
	for (int i = AES_BLOCK_LAST_BYTE_INDEX; i >= AES_CTR_COUNTER_START_BYTE; i--) {
		if (++ctr[i] != 0) {
			break;
		}
	}
}

psa_status_t cracen_sw_aes_ctr_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size)
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

	/* Initialize the operation */
	operation->alg = PSA_ALG_CTR;
	operation->dir = CRACEN_ENCRYPT; /* CTR mode is identical for encrypt/decrypt */
	operation->blk_size = SX_BLKCIPHER_AES_BLK_SZ;
	operation->unprocessed_input_bytes = 0;
	/* We don't consider the initalization finalized until IV is set */
	operation->initialized = false;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ctr_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length)
{
	if (operation->alg != PSA_ALG_CTR) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (iv_length != SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	operation->unprocessed_input_bytes = 0;
	/* Copy the IV (which includes the initial counter) */
	memcpy(operation->iv, iv, iv_length);
	operation->initialized = true;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ctr_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
{
	psa_status_t status = PSA_SUCCESS;
	size_t bytes_written = 0;
	size_t remaining_input;
	const uint8_t *current_input;
	uint8_t *current_output;
	uint8_t *keystream_block;
	size_t keystream_used;
	size_t remaining_keystream_bytes;
	size_t bytes_to_process;

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

	remaining_input = input_length;
	current_input = input;
	current_output = output;

	keystream_block = operation->unprocessed_input;
	keystream_used = operation->unprocessed_input_bytes;

	while (remaining_input > 0) {
		/* Generate a keystream block when starting a new one */
		if (keystream_used == 0) {
			uint8_t current_ctr[SX_BLKCIPHER_AES_BLK_SZ];

			memcpy(current_ctr, operation->iv, SX_BLKCIPHER_AES_BLK_SZ);

			status = cracen_sw_aes_primitive(&operation->cipher, &operation->keyref,
							 current_ctr, keystream_block);
			if (status != PSA_SUCCESS) {
				return status;
			}
		}

		remaining_keystream_bytes = SX_BLKCIPHER_AES_BLK_SZ - keystream_used;
		bytes_to_process = MIN(remaining_input, remaining_keystream_bytes);

		/* XOR keystream slice with input into output */
		for (size_t j = 0; j < bytes_to_process; j++) {
			current_output[j] = current_input[j] ^ keystream_block[keystream_used + j];
		}


		current_input += bytes_to_process;
		current_output += bytes_to_process;
		remaining_input -= bytes_to_process;
		bytes_written += bytes_to_process;
		keystream_used += bytes_to_process;

		/* If the keystream block was fully consumed, bump counter for next block */
		if (keystream_used == SX_BLKCIPHER_AES_BLK_SZ) {
			keystream_used = 0;
			increment_counter(operation->iv);
		}
	}

	operation->unprocessed_input_bytes = keystream_used;

	*output_length = bytes_written;
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
				      const uint8_t *iv, size_t iv_length, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
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

	status = cracen_sw_aes_ctr_setup(&operation, attributes, key_buffer, key_buffer_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_sw_aes_ctr_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_sw_aes_ctr_update(&operation, input, input_length, output, output_size,
					  output_length);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = cracen_sw_aes_ctr_finish(&operation, &finish_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	*output_length += finish_length;
	return PSA_SUCCESS;
}
