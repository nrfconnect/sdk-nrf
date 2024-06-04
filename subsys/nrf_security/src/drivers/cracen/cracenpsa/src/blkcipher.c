/**
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <cracen/mem_helpers.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <string.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/chachapoly.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/__assert.h>

#include "cracen_psa_primitives.h"

static psa_status_t cracen_cipher_crypt(cracen_cipher_operation_t *operation,
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

/* The AES ECB does not support multipart operations in Cracen. This means that keeping
 * the state between calls is not supported. This function is using the single part
 * APIs of Cracen to perform the AES ECB operations.
 */
psa_status_t cracen_cipher_crypt_ecb(const struct sxkeyref *key, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length, enum cipher_operation dir,
				     bool aes_countermeasures)
{
	int sx_status;
	struct sxblkcipher blkciph;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	if (dir == CRACEN_ENCRYPT) {
		sx_status = sx_blkcipher_create_aesecb_enc(&blkciph, key, aes_countermeasures);
	} else {
		sx_status = sx_blkcipher_create_aesecb_dec(&blkciph, key);
	}

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_crypt(&blkciph, input, input_length, output);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&blkciph);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&blkciph);
	if (sx_status == SX_OK) {
		*output_length = input_length;
	}

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_cipher_encrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				   const uint8_t *input, size_t input_length, uint8_t *output,
				   size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(iv != NULL);
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(output_length != NULL);

	psa_status_t status;
	cracen_cipher_operation_t operation = {0};
	*output_length = 0;

	/* If ECB is not enabled in the configuration the encrypt setup will return an not supported
	 * error and thus we don't need to write an else here.
	 */
	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (alg == PSA_ALG_ECB_NO_PADDING) {
			struct sxkeyref key;

			status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
			if (status != PSA_SUCCESS) {
				return status;
			}
			return cracen_cipher_crypt_ecb(&key, input, input_length, output,
						       output_size, output_length, CRACEN_ENCRYPT,
						       BA411_AES_COUNTERMEASURES_ENABLE);
		}
	}

	status = cracen_cipher_encrypt_setup(&operation, attributes, key_buffer, key_buffer_size,
					     alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_set_iv(&operation, iv, iv_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_cipher_crypt(&operation, attributes, alg, input, input_length, output,
				   output_size, output_length);
}

psa_status_t cracen_cipher_decrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   uint8_t *output, size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(output_length != NULL);

	cracen_cipher_operation_t operation = {0};
	psa_status_t status;
	/* ChaCha20 only supports 12 bytes IV in the single part decryption function */
	const size_t iv_size = (alg == PSA_ALG_STREAM_CIPHER) ? 12 : SX_BLKCIPHER_IV_SZ;
	struct sxkeyref key;
	*output_length = 0;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	/* If ECB is not enabled in the configuration the decrypt setup will return an not supported
	 * error and thus we don't need to write an else here.
	 */
	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (alg == PSA_ALG_ECB_NO_PADDING) {
			status = cracen_load_keyref(attributes, key_buffer, key_buffer_size, &key);
			if (status != PSA_SUCCESS) {
				return status;
			}
			return cracen_cipher_crypt_ecb(&key, input, input_length, output,
						       output_size, output_length, CRACEN_DECRYPT,
						       BA411_AES_COUNTERMEASURES_ENABLE);
		}
	}

	if (input_length < iv_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = cracen_cipher_decrypt_setup(&operation, attributes, key_buffer, key_buffer_size,
					     alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_set_iv(&operation, input, iv_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return cracen_cipher_crypt(&operation, attributes, alg, input + iv_size,
				   input_length - iv_size, output, output_size, output_length);
}

static bool is_alg_supported(psa_algorithm_t alg, const psa_key_attributes_t *attributes)
{

	bool is_supported = false;

	switch (alg) {
	case PSA_ALG_STREAM_CIPHER:
		/* This is needed because in the PSA APIs the PSA_ALG_STREAM_CIPHER
		 *  relies on the key type to identify which algorithm to use. Here we
		 *  make sure that the key type is supported before we continue.
		 */
		if (IS_ENABLED(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)) {
			is_supported = (psa_get_key_type(attributes) == PSA_KEY_TYPE_CHACHA20)
					       ? true
					       : false;
		}
		break;
	case PSA_ALG_CBC_NO_PADDING:
		IF_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES, (is_supported = true));
		break;
	case PSA_ALG_CBC_PKCS7:
		IF_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES, (is_supported = true));
		break;
	case PSA_ALG_CTR:
		IF_ENABLED(PSA_NEED_CRACEN_CTR_AES, (is_supported = true));
		break;
	case PSA_ALG_ECB_NO_PADDING:
		IF_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES, (is_supported = true));
		break;
	case PSA_ALG_OFB:
		IF_ENABLED(PSA_NEED_CRACEN_OFB_AES, (is_supported = true));
		break;
	default:
		is_supported = false;
		break;
	}

	return is_supported;
}

static psa_status_t initialize_cipher(cracen_cipher_operation_t *operation)
{
	int sx_status = SX_ERR_UNINITIALIZED_OBJ;

	switch (operation->alg) {
	case PSA_ALG_CBC_NO_PADDING:
		if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_blkcipher_create_aescbc_dec(&operation->cipher,
									     &operation->keyref,
									     operation->iv)
					    : sx_blkcipher_create_aescbc_enc(&operation->cipher,
									     &operation->keyref,
									     operation->iv);
		}
		break;
	case PSA_ALG_CBC_PKCS7:
		if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_blkcipher_create_aescbc_dec(&operation->cipher,
									     &operation->keyref,
									     operation->iv)
					    : sx_blkcipher_create_aescbc_enc(&operation->cipher,
									     &operation->keyref,
									     operation->iv);
		}
		break;
	case PSA_ALG_OFB:
		if (IS_ENABLED(PSA_NEED_CRACEN_OFB_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_blkcipher_create_aesofb_dec(&operation->cipher,
									     &operation->keyref,
									     operation->iv)
					    : sx_blkcipher_create_aesofb_enc(&operation->cipher,
									     &operation->keyref,
									     operation->iv);
		}
		break;
	case PSA_ALG_CTR:
		if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_blkcipher_create_aesctr_dec(&operation->cipher,
									     &operation->keyref,
									     operation->iv)
					    : sx_blkcipher_create_aesctr_enc(&operation->cipher,
									     &operation->keyref,
									     operation->iv);
		}
		break;
	case PSA_ALG_STREAM_CIPHER:
		if (IS_ENABLED(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_blkcipher_create_chacha20_dec(
						      &operation->cipher, &operation->keyref,
						      &operation->iv[0], &operation->iv[4])
					    : sx_blkcipher_create_chacha20_enc(
						      &operation->cipher, &operation->keyref,
						      &operation->iv[0], &operation->iv[4]);
		}
		break;
	default:
		sx_status = SX_ERR_INCOMPATIBLE_HW;
	}

	operation->initialized = true;

	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t operation_setup(enum cipher_operation dir, cracen_cipher_operation_t *operation,
				    const psa_key_attributes_t *attributes,
				    const uint8_t *key_buffer, size_t key_buffer_size,
				    psa_algorithm_t alg)
{
	if (!is_alg_supported(alg, attributes)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/*
	 * Copy the key into the operation struct as it is not guaranteed
	 * to be valid longer than the function call.
	 */

	if (key_buffer_size > sizeof(operation->key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->key_buffer, key_buffer, key_buffer_size);

	psa_status_t status = cracen_load_keyref(attributes, operation->key_buffer, key_buffer_size,
						 &operation->keyref);

	if (status) {
		return status;
	}

	operation->alg = alg;
	operation->dir = dir;
	operation->blk_size =
		(alg == PSA_ALG_STREAM_CIPHER) ? SX_BLKCIPHER_MAX_BLK_SZ : SX_BLKCIPHER_AES_BLK_SZ;

	return PSA_SUCCESS;
}

psa_status_t cracen_cipher_encrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg)
{
	return operation_setup(CRACEN_ENCRYPT, operation, attributes, key_buffer, key_buffer_size,
			       alg);
}

psa_status_t cracen_cipher_decrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg)
{
	return operation_setup(CRACEN_DECRYPT, operation, attributes, key_buffer, key_buffer_size,
			       alg);
}

psa_status_t cracen_cipher_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				  size_t iv_length)
{
	__ASSERT_NO_MSG(iv != NULL);

	/* Set IV is called after the encrypt/decrypt setup functions thus we
	 * know that we have CHACHA20 as the stream cipher here. Chacha20
	 * supports IV length of 12 bytes which uses a zero counter.
	 * The internal operation->iv is always 16 bytes where the first
	 * 4 bytes contain the counter. Since the operation is always
	 * initialized with 0s we can just place the IV in the correct offset.
	 */

	if (IS_ENABLED(PSA_NEED_CRACEN_STREAM_CIPHER_CHACHA20)) {
		if (operation->alg == PSA_ALG_STREAM_CIPHER) {
			if (iv_length == 12) {
				memcpy(&operation->iv[4], iv, iv_length);
				return PSA_SUCCESS;
			} else {
				return (iv_length == 8 || iv_length == 16)
					       ? PSA_ERROR_NOT_SUPPORTED
					       : PSA_ERROR_INVALID_ARGUMENT;
			}
		}
	}

	if (iv_length != SX_BLKCIPHER_IV_SZ) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->iv, iv, iv_length);
	return PSA_SUCCESS;
}

psa_status_t cracen_cipher_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				  size_t input_length, uint8_t *output, size_t output_size,
				  size_t *output_length)
{
	__ASSERT_NO_MSG(input != NULL);
	__ASSERT_NO_MSG(output != NULL);
	__ASSERT_NO_MSG(output_length != NULL);

	int sx_status = SX_ERR_UNINITIALIZED_OBJ;
	*output_length = 0;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (output_size < input_length + operation->unprocessed_input_bytes) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->unprocessed_input_bytes > 0) {
		size_t fill_block_bytes = operation->blk_size - operation->unprocessed_input_bytes;

		if (input_length < fill_block_bytes) {
			memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes,
			       input, input_length);
			operation->unprocessed_input_bytes += input_length;
			return PSA_SUCCESS;
		}

		memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes, input,
		       fill_block_bytes);
		operation->unprocessed_input_bytes += fill_block_bytes;

		/* Adjust input pointers */
		input += fill_block_bytes;
		input_length -= fill_block_bytes;
	}

	/* Clamp processed data to multiple of block size */
	size_t block_bytes = input_length & ~((uint32_t)operation->blk_size - 1);

	if (operation->dir == CRACEN_DECRYPT && operation->alg == PSA_ALG_CBC_PKCS7) {
		/* The last block contains padding. The block containing padding
		 * must be handled in finish operation. If input data is block
		 * aligned we must postpone processing of the last block.
		 */
		if (block_bytes == input_length) {
			block_bytes -= (uint32_t)operation->blk_size;
		}
	}

	if (block_bytes || operation->unprocessed_input_bytes) {
		size_t total_output = block_bytes + operation->unprocessed_input_bytes;

		/* sxsymcrypt doesn't support context saving for ECB, as each encrypted block is
		 * independent from the previous one we just encrypt the so far available full
		 * blocks.
		 */
		if (operation->alg == PSA_ALG_ECB_NO_PADDING) {
			if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {

				if (operation->unprocessed_input_bytes) {
					__ASSERT_NO_MSG(operation->unprocessed_input_bytes ==
							operation->blk_size);
					status = cracen_cipher_crypt_ecb(
						&operation->keyref, operation->unprocessed_input,
						operation->unprocessed_input_bytes, output,
						output_size, output_length, operation->dir,
						BA411_AES_COUNTERMEASURES_ENABLE);
					if (status != PSA_SUCCESS) {
						return status;
					}
					output += (uint32_t)operation->unprocessed_input_bytes;
					operation->unprocessed_input_bytes = 0;
				}

				if (block_bytes) {
					status = cracen_cipher_crypt_ecb(
						&operation->keyref, input, block_bytes, output,
						output_size, output_length, operation->dir,
						BA411_AES_COUNTERMEASURES_ENABLE);
					if (status != PSA_SUCCESS) {
						return status;
					}
				}
			}

		} else {
			if (operation->initialized) {
				sx_status = sx_blkcipher_resume_state(&operation->cipher);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}
			} else {
				psa_status_t r = initialize_cipher(operation);

				if (r != PSA_SUCCESS) {
					return r;
				}
			}

			if (operation->unprocessed_input_bytes) {
				__ASSERT_NO_MSG(operation->unprocessed_input_bytes ==
						operation->blk_size);
				sx_status = sx_blkcipher_crypt(
					&operation->cipher, operation->unprocessed_input,
					operation->unprocessed_input_bytes, output);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}

				operation->unprocessed_input_bytes = 0;
				/* Adjust output pointer. */
				output += (uint32_t)operation->blk_size;
			}

			if (block_bytes) {
				sx_status = sx_blkcipher_crypt(&operation->cipher, input,
							       block_bytes, output);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}
			}

			sx_status = sx_blkcipher_save_state(&operation->cipher);
			if (sx_status) {
				return silex_statuscodes_to_psa(sx_status);
			}

			sx_status = sx_blkcipher_wait(&operation->cipher);
			if (sx_status) {
				return silex_statuscodes_to_psa(sx_status);
			}

			*output_length += total_output;
		}
	}

	/* Store unprocessed bytes until next update or finalization of crypto
	 * operation.
	 */
	if (block_bytes < input_length) {
		__ASSERT_NO_MSG(operation->unprocessed_input_bytes == 0);
		memcpy(operation->unprocessed_input, input + block_bytes,
		       input_length - block_bytes);
		operation->unprocessed_input_bytes = input_length - block_bytes;
	}

	(void)status;
	return PSA_SUCCESS;
}

psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(output_length != NULL);

	int sx_status;

	*output_length = 0;

	if (operation->unprocessed_input_bytes == 0 && operation->alg != PSA_ALG_CBC_PKCS7) {
		return PSA_SUCCESS;
	}

	__ASSERT_NO_MSG(output != NULL);

	sx_status = SX_ERR_UNINITIALIZED_OBJ;

	/* sxsymcrypt doesn't support context saving for ECB, as each encrypted block is
	 * independent from the previous one we just encrypt the so far available full
	 * blocks.
	 */
	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (operation->alg == PSA_ALG_ECB_NO_PADDING) {
			return cracen_cipher_crypt_ecb(
				&operation->keyref, operation->unprocessed_input,
				operation->unprocessed_input_bytes, output, output_size,
				output_length, operation->dir, BA411_AES_COUNTERMEASURES_ENABLE);
		}
	}

	if (operation->initialized) {
		sx_status = sx_blkcipher_resume_state(&operation->cipher);
		if (sx_status) {
			return silex_statuscodes_to_psa(sx_status);
		}
	} else {
		psa_status_t r = initialize_cipher(operation);

		if (r != PSA_SUCCESS) {
			return r;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (operation->alg == PSA_ALG_CBC_PKCS7) {
			if (operation->dir == CRACEN_ENCRYPT) {
				uint8_t padding = (uint32_t)operation->blk_size -
						  operation->unprocessed_input_bytes;

				/* The value to pad which equals the number of
				 * padded bytes as described in PKCS7 (rfc2315).
				 */
				memset(&operation->unprocessed_input
						[operation->unprocessed_input_bytes],
				       padding, padding);
				operation->unprocessed_input_bytes = SX_BLKCIPHER_AES_BLK_SZ;
			} else {
				__ASSERT_NO_MSG(operation->blk_size == SX_BLKCIPHER_AES_BLK_SZ);

				if (operation->unprocessed_input_bytes != SX_BLKCIPHER_AES_BLK_SZ) {
					return PSA_ERROR_INVALID_ARGUMENT;
				}

				uint8_t out_with_padding[SX_BLKCIPHER_AES_BLK_SZ];

				sx_status = sx_blkcipher_crypt(
					&operation->cipher, operation->unprocessed_input,
					operation->unprocessed_input_bytes, out_with_padding);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}

				sx_status = sx_blkcipher_run(&operation->cipher);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}

				sx_status = sx_blkcipher_wait(&operation->cipher);
				if (sx_status) {
					return silex_statuscodes_to_psa(sx_status);
				}

				uint8_t padding = out_with_padding[SX_BLKCIPHER_AES_BLK_SZ - 1];
				/* Verify that padding is in the valid
				 * range.
				 */
				if (padding > SX_BLKCIPHER_AES_BLK_SZ || padding == 0) {
					return PSA_ERROR_INVALID_PADDING;
				}

				/* Verify all padding bytes. */
				for (unsigned int i = SX_BLKCIPHER_AES_BLK_SZ;
				     i > (SX_BLKCIPHER_AES_BLK_SZ - padding); i--) {
					if (out_with_padding[i - 1] != padding) {
						return PSA_ERROR_INVALID_PADDING;
					}
				}

				/* Verify output buffer. */
				*output_length = SX_BLKCIPHER_AES_BLK_SZ - padding;
				if (*output_length > output_size) {
					*output_length = 0;
					return PSA_ERROR_BUFFER_TOO_SMALL;
				}

				/* Copy plaintext without padding. */
				memcpy(output, out_with_padding, *output_length);

				return PSA_SUCCESS;
			}
		}
	}

	*output_length = operation->unprocessed_input_bytes;
	if (*output_length > output_size) {
		*output_length = 0;
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->alg == PSA_ALG_CBC_NO_PADDING &&
	    (operation->unprocessed_input_bytes % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	sx_status = sx_blkcipher_crypt(&operation->cipher, operation->unprocessed_input,
				       operation->unprocessed_input_bytes, output);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&operation->cipher);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&operation->cipher);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_cipher_abort(cracen_cipher_operation_t *operation)
{
	sx_blkcipher_free(&operation->cipher);
	safe_memzero(operation, sizeof(cracen_cipher_operation_t));
	return PSA_SUCCESS;
}
