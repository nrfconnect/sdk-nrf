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
#include <cracen_sw_aes_cbc.h>

psa_status_t cracen_sw_aes_cbc_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, enum cipher_operation dir)
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
	operation->alg = alg;
	operation->dir = dir;
	operation->blk_size = SX_BLKCIPHER_AES_BLK_SZ;
	operation->unprocessed_input_bytes = 0;
	/* We don't consider the initialization finalized until IV is set */
	operation->initialized = false;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_cbc_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length)
{
	if (iv_length != SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	operation->unprocessed_input_bytes = 0;
	memcpy(operation->iv, iv, iv_length);
	operation->initialized = true;

	return PSA_SUCCESS;
}

static psa_status_t cbc_encrypt_block(cracen_cipher_operation_t *operation, const uint8_t *input,
				      uint8_t *output)
{
	psa_status_t status;
	uint8_t block_to_encrypt[SX_BLKCIPHER_AES_BLK_SZ];
	size_t output_len;

	/* The plaintext is xored with the previously encrypted block.
	 * for the first block the IV is used.
	 */
	memcpy(block_to_encrypt, input, SX_BLKCIPHER_AES_BLK_SZ);
	cracen_xorbytes((char *)block_to_encrypt, (const char *)operation->iv,
			SX_BLKCIPHER_AES_BLK_SZ);

	status = cracen_sw_aes_ecb_encrypt(&operation->cipher, &operation->keyref, block_to_encrypt,
					   SX_BLKCIPHER_AES_BLK_SZ, output, SX_BLKCIPHER_AES_BLK_SZ,
					   &output_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	memcpy(operation->iv, output, SX_BLKCIPHER_AES_BLK_SZ);

exit:
	/* Clear sensitive data from memory */
	safe_memzero(block_to_encrypt, sizeof(block_to_encrypt));

	return status;
}

static psa_status_t cbc_decrypt_block(cracen_cipher_operation_t *operation, const uint8_t *input,
				      uint8_t *output)
{
	psa_status_t status;
	size_t output_len;
	uint8_t decrypted_block[SX_BLKCIPHER_AES_BLK_SZ];
	uint8_t next_iv[SX_BLKCIPHER_AES_BLK_SZ];

	/* CBC decryption is just the reverse of encryption.
	 * So the block is decrypted first and then XORed with the previous ciphertext block
	 */
	memcpy(next_iv, input, SX_BLKCIPHER_AES_BLK_SZ);
	status = cracen_sw_aes_ecb_decrypt(&operation->cipher, &operation->keyref, input,
					   SX_BLKCIPHER_AES_BLK_SZ, decrypted_block,
					   SX_BLKCIPHER_AES_BLK_SZ, &output_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* XOR with the previous ciphertext block (current IV) */
	memcpy(output, decrypted_block, SX_BLKCIPHER_AES_BLK_SZ);
	cracen_xorbytes((char *)output, (const char *)operation->iv, SX_BLKCIPHER_AES_BLK_SZ);

	memcpy(operation->iv, next_iv, SX_BLKCIPHER_AES_BLK_SZ);

exit:
	/* Clear sensitive data from memory */
	safe_memzero(decrypted_block, sizeof(decrypted_block));
	safe_memzero(next_iv, sizeof(next_iv));

	return status;
}

static psa_status_t process_block(cracen_cipher_operation_t *operation, const uint8_t *input,
				  uint8_t *output)
{
	return (operation->dir == CRACEN_ENCRYPT) ? cbc_encrypt_block(operation, input, output)
						  : cbc_decrypt_block(operation, input, output);
}

static psa_status_t complete_partial_block(cracen_cipher_operation_t *operation,
					   const uint8_t **current_input, uint8_t **current_output,
					   size_t *remaining_input, size_t *bytes_written)
{
	psa_status_t status;
	size_t bytes_to_copy;

	bytes_to_copy = SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes;
	if (bytes_to_copy > *remaining_input) {
		/* Not enough data to complete a block, just buffer it */
		memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes,
		       *current_input, *remaining_input);
		operation->unprocessed_input_bytes += *remaining_input;
		return PSA_SUCCESS;
	}

	/* Complete the partial block */
	memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes, *current_input,
	       bytes_to_copy);

	/* Process the completed block */
	status = process_block(operation, operation->unprocessed_input, *current_output);
	if (status != PSA_SUCCESS) {
		return status;
	}

	*current_input += bytes_to_copy;
	*current_output += SX_BLKCIPHER_AES_BLK_SZ;
	*remaining_input -= bytes_to_copy;
	*bytes_written += SX_BLKCIPHER_AES_BLK_SZ;
	operation->unprocessed_input_bytes = 0;

	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_cbc_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
{
	psa_status_t status = PSA_SUCCESS;
	size_t bytes_written = 0;
	size_t remaining_input;
	const uint8_t *current_input;
	uint8_t *current_output;

	*output_length = 0;

	if (!operation->initialized) {
		return PSA_ERROR_BAD_STATE;
	}

	/* Valid operation, we just don't do anything */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (output == NULL || output_size < input_length + operation->unprocessed_input_bytes) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	remaining_input = input_length;
	current_input = input;
	current_output = output;

	/* Process any leftover data from previous update */
	if (operation->unprocessed_input_bytes > 0) {
		status = complete_partial_block(operation, &current_input, &current_output,
						&remaining_input, &bytes_written);
		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	/* Process complete blocks */
	while (remaining_input >= SX_BLKCIPHER_AES_BLK_SZ) {
		/* For CBC_PKCS7 decryption, we need to keep the last block for finish() */
		if (operation->dir == CRACEN_DECRYPT && operation->alg == PSA_ALG_CBC_PKCS7) {
			if (remaining_input == SX_BLKCIPHER_AES_BLK_SZ) {
				memcpy(operation->unprocessed_input, current_input,
				       SX_BLKCIPHER_AES_BLK_SZ);
				operation->unprocessed_input_bytes = SX_BLKCIPHER_AES_BLK_SZ;
				break;
			}
		}

		status = process_block(operation, current_input, current_output);
		if (status != PSA_SUCCESS) {
			return status;
		}

		current_input += SX_BLKCIPHER_AES_BLK_SZ;
		current_output += SX_BLKCIPHER_AES_BLK_SZ;
		remaining_input -= SX_BLKCIPHER_AES_BLK_SZ;
		bytes_written += SX_BLKCIPHER_AES_BLK_SZ;
	}

	/* Buffer any remaining incomplete block */
	if (remaining_input > 0) {
		memcpy(operation->unprocessed_input, current_input, remaining_input);
		operation->unprocessed_input_bytes = remaining_input;
	}

	*output_length = bytes_written;
	return PSA_SUCCESS;
}

static psa_status_t verify_and_remove_pkcs7_padding(const uint8_t *block, uint8_t *output,
						    size_t output_size, size_t *output_length)
{
	uint8_t padding = block[SX_BLKCIPHER_AES_BLK_SZ - 1];
	size_t padding_index = SX_BLKCIPHER_AES_BLK_SZ - padding;
	uint32_t failure = 0;

	failure |= (padding > SX_BLKCIPHER_AES_BLK_SZ);
	failure |= (padding == 0);

	for (size_t i = 0; i < SX_BLKCIPHER_AES_BLK_SZ; i++) {
		failure |= (block[i] ^ padding) * (i >= padding_index);
	}

	*output_length = SX_BLKCIPHER_AES_BLK_SZ - padding;

	if (failure != 0) {
		*output_length = 0;
		return PSA_ERROR_INVALID_PADDING;
	}

	if (*output_length > output_size) {
		*output_length = 0;
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memcpy(output, block, *output_length);

	return PSA_SUCCESS;
}

static psa_status_t finish_pkcs7_encrypt(cracen_cipher_operation_t *operation, uint8_t *output,
					 size_t output_size, size_t *output_length)
{
	psa_status_t status;
	uint8_t padding = (uint8_t)(SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes);

	/* We always use a temp buffer due to the need to handle inplace encryption */
	uint8_t temp_output[SX_BLKCIPHER_AES_BLK_SZ];

	if (output_size < SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memset(operation->unprocessed_input + operation->unprocessed_input_bytes, padding, padding);

	status = cbc_encrypt_block(operation, operation->unprocessed_input, temp_output);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	memcpy(output, temp_output, SX_BLKCIPHER_AES_BLK_SZ);

	*output_length = SX_BLKCIPHER_AES_BLK_SZ;

exit:
	safe_memzero(temp_output, sizeof(temp_output));
	return status;
}

static psa_status_t finish_pkcs7_decrypt(cracen_cipher_operation_t *operation, uint8_t *output,
					 size_t output_size, size_t *output_length)
{
	psa_status_t status;
	uint8_t decrypted_block[SX_BLKCIPHER_AES_BLK_SZ];

	if (operation->unprocessed_input_bytes != SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = cbc_decrypt_block(operation, operation->unprocessed_input, decrypted_block);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = verify_and_remove_pkcs7_padding(decrypted_block, output, output_size,
						 output_length);

exit:
	safe_memzero(decrypted_block, sizeof(decrypted_block));
	return status;
}

static psa_status_t finish_no_padding(cracen_cipher_operation_t *operation, uint8_t *output,
				      size_t output_size, size_t *output_length)
{
	psa_status_t status;
	/* We always use a temp buffer due to the need to handle inplace encryption */
	uint8_t temp_output[SX_BLKCIPHER_AES_BLK_SZ];

	*output_length = 0;

	if (operation->unprocessed_input_bytes == 0) {
		return PSA_SUCCESS;
	}

	if (operation->unprocessed_input_bytes != SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_size < SX_BLKCIPHER_AES_BLK_SZ) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = process_block(operation, operation->unprocessed_input, temp_output);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	memcpy(output, temp_output, SX_BLKCIPHER_AES_BLK_SZ);

	*output_length = SX_BLKCIPHER_AES_BLK_SZ;

exit:
	safe_memzero(temp_output, sizeof(temp_output));
	return status;
}

psa_status_t cracen_sw_aes_cbc_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				      size_t output_size, size_t *output_length)
{
	psa_status_t status;

	*output_length = 0;

	if (!operation->initialized) {
		return PSA_ERROR_BAD_STATE;
	}

	if (operation->alg == PSA_ALG_CBC_PKCS7) {
		status = (operation->dir == CRACEN_ENCRYPT)
				 ? finish_pkcs7_encrypt(operation, output, output_size,
							output_length)
				 : finish_pkcs7_decrypt(operation, output, output_size,
							output_length);
	} else {
		status = finish_no_padding(operation, output, output_size, output_length);
	}

	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->unprocessed_input_bytes = 0;
	safe_memzero(operation->unprocessed_input, SX_BLKCIPHER_AES_BLK_SZ);
	operation->initialized = false;

	return PSA_SUCCESS;
}

static psa_status_t cbc_crypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *iv,
			      size_t iv_length, const uint8_t *input, size_t input_length,
			      uint8_t *output, size_t output_size, size_t *output_length,
			      enum cipher_operation dir)
{
	psa_status_t status;
	cracen_cipher_operation_t operation = {0};
	size_t update_length;
	size_t finish_length;

	if (input_length > 0 && input == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_size > 0 && output == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;
	operation.alg = alg;

	status = cracen_sw_aes_cbc_setup(&operation, attributes, key_buffer, key_buffer_size, alg,
					 dir);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_sw_aes_cbc_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_sw_aes_cbc_update(&operation, input, input_length, output, output_size,
					  &update_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_sw_aes_cbc_finish(&operation, output + update_length,
					  output_size - update_length, &finish_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	*output_length = update_length + finish_length;

exit:
	safe_memzero(&operation, sizeof(operation));
	return status;
}

psa_status_t cracen_sw_aes_cbc_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	return cbc_crypt(attributes, key_buffer, key_buffer_size, alg, iv, iv_length, input,
			 input_length, output, output_size, output_length, CRACEN_ENCRYPT);
}

psa_status_t cracen_sw_aes_cbc_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	return cbc_crypt(attributes, key_buffer, key_buffer_size, alg, iv, iv_length, input,
			 input_length, output, output_size, output_length, CRACEN_DECRYPT);
}
