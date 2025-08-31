/**
 *
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../../../cracenpsa/src/common.h"
#include <cracen/mem_helpers.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <zephyr/sys/util.h>
#include <stdbool.h>
#include <string.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/chachapoly.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/__assert.h>
#include <cracen_sw_aes_ctr.h>
#include <cracen_sw_common.h>

#include "cracen_psa_primitives.h"

static bool is_alg_supported(psa_algorithm_t alg, const psa_key_attributes_t *attributes)
{
	bool is_supported = false;

	switch (alg) {
	case PSA_ALG_CTR:
		IF_ENABLED(PSA_NEED_CRACEN_CTR_AES,
			   (is_supported = psa_get_key_type(attributes) == PSA_KEY_TYPE_AES));
		break;
	default:
		is_supported = false;
		break;
	}

	return is_supported;
}

static psa_status_t encrypt_cbc(const struct sxkeyref *key, const uint8_t *input,
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
	if (remaining_bytes > 0) {
		memcpy(padded_input_block, input + full_blocks_length, remaining_bytes);
	}

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
		return PSA_SUCCESS;
	} else {
		return silex_statuscodes_to_psa(sx_status);
	}
}

static psa_status_t decrypt_cbc(const struct sxkeyref *key, const uint8_t *input,
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

static psa_status_t crypt(cracen_cipher_operation_t *operation,
			  const psa_key_attributes_t *attributes, psa_algorithm_t alg,
			  const uint8_t *input, size_t input_length, uint8_t *output,
			  size_t output_size, size_t *output_length)
{
	size_t update_output_length = 0;
	size_t finish_output_length = 0;
	psa_status_t status = cracen_cipher_update(operation, input, input_length, output,
						   output_size, &update_output_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_finish(operation, output + update_output_length,
				      output_size - update_output_length, &finish_output_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	*output_length = update_output_length + finish_output_length;

	return status;
}

psa_status_t cracen_cipher_encrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				   const uint8_t *input, size_t input_length, uint8_t *output,
				   size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(iv != NULL);
	__ASSERT_NO_MSG(input != NULL || input_length == 0);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(output_length != NULL);

	*output_length = 0;
	psa_status_t status;

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (alg == PSA_ALG_CTR) {
			if (output_size < input_length) {
				return PSA_ERROR_BUFFER_TOO_SMALL;
			}

			/* Handle inplace encryption by moving plaintext to right to free space for
			 * iv
			 */
			if (input_length && output > input && output < input + input_length) {
				memmove(output, input, input_length);
				input = output;
			}

			return cracen_sw_aes_ctr_crypt(attributes, key_buffer, key_buffer_size, iv,
						       iv_length, input, input_length, output,
						       output_size, output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES) && alg == PSA_ALG_ECB_NO_PADDING) {
		struct sxkeyref key;

		status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
		if (status != PSA_SUCCESS) {
			return status;
		}
		return cracen_aes_ecb_encrypt(&key, input, input_length, output, output_size,
					      output_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES) && alg == PSA_ALG_CBC_PKCS7) {
		struct sxkeyref key;

		status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
		if (status != PSA_SUCCESS) {
			return status;
		}
		return encrypt_cbc(&key, input, input_length, output, output_size, output_length,
				   iv);
	}

	status = setup(CRACEN_ENCRYPT, &operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return crypt(&operation, attributes, alg, input, input_length, output, output_size,
		     output_length);
}

psa_status_t cracen_cipher_decrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   uint8_t *output, size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(input != NULL || input_length == 0);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(output_length != NULL);

	const size_t iv_size = SX_BLKCIPHER_IV_SZ;
	psa_status_t status;

	*output_length = 0;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (input_length < iv_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_crypt(attributes, key_buffer, key_buffer_size,
						       input, iv_size, input + iv_size,
						       input_length - iv_size, output, output_size,
						       output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES) && alg == PSA_ALG_ECB_NO_PADDING) {
		struct sxkeyref key;

		status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
		if (status != PSA_SUCCESS) {
			return status;
		}
		return cracen_aes_ecb_decrypt(&key, input, input_length, output, output_size,
					      output_length);
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES) && alg == PSA_ALG_CBC_PKCS7) {
		struct sxkeyref key;

		status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
		if (status != PSA_SUCCESS) {
			return status;
		}
		return decrypt_cbc(&key, input + iv_size, input_length - iv_size, output,
				   output_size, output_length, input);
	}

	status = setup(CRACEN_DECRYPT, &operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return crypt(&operation, attributes, alg, input, input_length, output, output_size,
		     output_length);

}

psa_status_t cracen_cipher_encrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg)
{
	if (!is_alg_supported(alg, attributes)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_setup(operation, attributes, key_buffer,
						       key_buffer_size);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_cipher_decrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg)
{
	if (!is_alg_supported(alg, attributes)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		return cracen_sw_aes_ctr_setup(operation, attributes, key_buffer, key_buffer_size);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_cipher_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				  size_t iv_length)
{
	__ASSERT_NO_MSG(iv != NULL);

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (operation->alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_set_iv(operation, iv, iv_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_cipher_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				  size_t input_length, uint8_t *output, size_t output_size,
				  size_t *output_length)
{
	__ASSERT_NO_MSG(input != NULL || input_length == 0);
	__ASSERT_NO_MSG(output_length != NULL);

	*output_length = 0;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (output == NULL || output_size < input_length + operation->unprocessed_input_bytes) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (operation->alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_update(operation, input, input_length, output,
							output_size, output_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(output_length != NULL);

	*output_length = 0;

	if (operation->unprocessed_input_bytes == 0 && operation->alg != PSA_ALG_CBC_PKCS7) {
		return PSA_SUCCESS;
	}

	__ASSERT_NO_MSG(output != NULL);

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (operation->alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_finish(operation, output_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_cipher_abort(cracen_cipher_operation_t *operation)
{
	int sx_status;

	sx_status = sx_blkcipher_free(&operation->cipher);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	safe_memzero(operation, sizeof(cracen_cipher_operation_t));
	return PSA_SUCCESS;
}
