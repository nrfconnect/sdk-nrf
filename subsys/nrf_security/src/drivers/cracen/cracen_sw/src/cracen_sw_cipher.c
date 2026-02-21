/**
 *
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/common.h>
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
#include <cracen_sw_aes_cbc.h>
#include <cracen_sw_common.h>

#include <cracen_psa_primitives.h>

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
		IF_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES,
			   (is_supported = psa_get_key_type(attributes) == PSA_KEY_TYPE_AES));
		break;
	case PSA_ALG_CBC_PKCS7:
		IF_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES,
			   (is_supported = psa_get_key_type(attributes) == PSA_KEY_TYPE_AES));
		break;
	case PSA_ALG_CTR:
		IF_ENABLED(PSA_NEED_CRACEN_CTR_AES,
			   (is_supported = psa_get_key_type(attributes) == PSA_KEY_TYPE_AES));
		break;
	case PSA_ALG_ECB_NO_PADDING:
		IF_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES,
			   (is_supported = psa_get_key_type(attributes) == PSA_KEY_TYPE_AES));
		break;
	default:
		is_supported = false;
		break;
	}

	return is_supported;
}

static psa_status_t setup(enum cipher_operation dir, cracen_cipher_operation_t *operation,
			  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			  size_t key_buffer_size, psa_algorithm_t alg)
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
	if (status != PSA_SUCCESS) {
		return status;
	}

	operation->alg = alg;
	operation->dir = dir;
	operation->blk_size =
		(alg == PSA_ALG_STREAM_CIPHER) ? SX_BLKCIPHER_MAX_BLK_SZ : SX_BLKCIPHER_AES_BLK_SZ;

	return PSA_SUCCESS;
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

	psa_status_t status;
	*output_length = 0;
	cracen_cipher_operation_t operation = {0};

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (alg == PSA_ALG_CTR) {
			if (output_size < input_length) {
				return PSA_ERROR_BUFFER_TOO_SMALL;
			}

			/* Handle inplace encryption by moving plaintext to the right by iv_length
			 * bytes. This is done because in inplace encryption the input and output
			 * should point to the same data so that the output can overwrite its own
			 * input. If they are not in sync the output will overwrite the input of
			 * another operation which is of course wrong.
			 *
			 */
			if (output == input + iv_length) {
				memmove(output, input, input_length);
				input = output;
			}

			return cracen_sw_aes_ctr_crypt(attributes, key_buffer, key_buffer_size, iv,
						       iv_length, input, input_length, output,
						       output_size, output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (alg == PSA_ALG_ECB_NO_PADDING) {
			status = cracen_load_keyref(attributes, key_buffer, key_buffer_size,
						    &operation.keyref);
			if (status != PSA_SUCCESS) {
				return status;
			}
			return cracen_sw_aes_ecb_encrypt(&operation.cipher, &operation.keyref,
							 input, input_length, output, output_size,
							 output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (alg == PSA_ALG_CBC_PKCS7) {

			/* Handle inplace encryption by moving plaintext to the right by iv_length
			 * bytes. This is done because in inplace encryption the input and output
			 * should point to the same data so that the output can overwrite its own
			 * input. If they are not in sync the output will overwrite the input of
			 * another operation which is of course wrong.
			 *
			 */
			if (output == input + iv_length) {
				memmove(output, input, input_length);
				input = output;
			}
			return cracen_sw_aes_cbc_encrypt(attributes, key_buffer, key_buffer_size,
							 alg, iv, iv_length, input, input_length,
							 output, output_size, output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)) {
		if (alg == PSA_ALG_CBC_NO_PADDING) {

			/* Handle inplace encryption by moving plaintext to the right by iv_length
			 * bytes. This is done because in inplace encryption the input and output
			 * should point to the same data so that the output can overwrite its own
			 * input. If they are not in sync the output will overwrite the input of
			 * another operation which is of course wrong.
			 *
			 */
			if (output == input + iv_length) {
				memmove(output, input, input_length);
				input = output;
			}
			return cracen_sw_aes_cbc_encrypt(attributes, key_buffer, key_buffer_size,
							 alg, iv, iv_length, input, input_length,
							 output, output_size, output_length);
		}
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

	cracen_cipher_operation_t operation = {0};
	psa_status_t status;
	/* ChaCha20 only supports 12 bytes IV in the single part decryption function */
	const size_t iv_size = (alg == PSA_ALG_STREAM_CIPHER) ? 12 : SX_BLKCIPHER_IV_SZ;
	*output_length = 0;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_AES)) {
		if (alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_crypt(attributes, key_buffer, key_buffer_size,
						       input, iv_size, input + iv_size,
						       input_length - iv_size, output, output_size,
						       output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (alg == PSA_ALG_ECB_NO_PADDING) {
			status = cracen_load_keyref(attributes, key_buffer, key_buffer_size,
						    &operation.keyref);
			if (status != PSA_SUCCESS) {
				return status;
			}
			return cracen_sw_aes_ecb_decrypt(&operation.cipher, &operation.keyref,
							 input, input_length, output, output_size,
							 output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_decrypt(attributes, key_buffer, key_buffer_size,
							 alg, input, iv_size, input + iv_size,
							 input_length - iv_size, output,
							 output_size, output_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)) {
		if (alg == PSA_ALG_CBC_NO_PADDING) {
			return cracen_sw_aes_cbc_decrypt(attributes, key_buffer, key_buffer_size,
							 alg, input, iv_size, input + iv_size,
							 input_length - iv_size, output,
							 output_size, output_length);
		}
	}

	if (input_length < iv_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = setup(CRACEN_DECRYPT, &operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_cipher_set_iv(&operation, input, iv_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return crypt(&operation, attributes, alg, input + iv_size, input_length - iv_size, output,
		     output_size, output_length);
}

static psa_status_t initialize_cipher(cracen_cipher_operation_t *operation)
{
	int sx_status = SX_ERR_UNINITIALIZED_OBJ;

	switch (operation->alg) {
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

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)) {
		if (alg == PSA_ALG_CBC_NO_PADDING) {
			return cracen_sw_aes_cbc_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg, CRACEN_ENCRYPT);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg, CRACEN_ENCRYPT);
		}
	}

	return setup(CRACEN_ENCRYPT, operation, attributes, key_buffer, key_buffer_size, alg);
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
		if (alg == PSA_ALG_CTR) {
			return cracen_sw_aes_ctr_setup(operation, attributes, key_buffer,
						       key_buffer_size);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES)) {
		if (alg == PSA_ALG_CBC_NO_PADDING) {
			return cracen_sw_aes_cbc_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg, CRACEN_DECRYPT);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg, CRACEN_DECRYPT);
		}
	}

	return setup(CRACEN_DECRYPT, operation, attributes, key_buffer, key_buffer_size, alg);
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

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES) ||
	    IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (operation->alg == PSA_ALG_CBC_NO_PADDING ||
		    operation->alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_set_iv(operation, iv, iv_length);
		}
	}

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
	__ASSERT_NO_MSG(input != NULL || input_length == 0);
	__ASSERT_NO_MSG(output_length != NULL);

	int sx_status = SX_ERR_UNINITIALIZED_OBJ;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;
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

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES) ||
	    IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (operation->alg == PSA_ALG_CBC_NO_PADDING ||
		    operation->alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_update(operation, input, input_length, output,
							output_size, output_length);
		}
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
					if (operation->dir == CRACEN_ENCRYPT) {
						psa_status = cracen_sw_aes_ecb_encrypt(
							&operation->cipher, &operation->keyref,
							operation->unprocessed_input,
							operation->unprocessed_input_bytes, output,
							output_size, output_length);
					} else {
						psa_status = cracen_sw_aes_ecb_decrypt(
							&operation->cipher, &operation->keyref,
							operation->unprocessed_input,
							operation->unprocessed_input_bytes, output,
							output_size, output_length);
					}
					if (psa_status != PSA_SUCCESS) {
						return psa_status;
					}
					output += (uint32_t)operation->unprocessed_input_bytes;
					operation->unprocessed_input_bytes = 0;
				}

				if (block_bytes) {
					if (operation->dir == CRACEN_ENCRYPT) {
						psa_status = cracen_sw_aes_ecb_encrypt(
							&operation->cipher, &operation->keyref,
							input, block_bytes, output, output_size,
							output_length);
					} else {
						psa_status = cracen_sw_aes_ecb_decrypt(
							&operation->cipher, &operation->keyref,
							input, block_bytes, output, output_size,
							output_length);
					}
					if (psa_status != PSA_SUCCESS) {
						return psa_status;
					}
				}
			}

		} else {
			if (operation->initialized) {
				sx_status = sx_blkcipher_resume_state(&operation->cipher);
				if (sx_status != SX_OK) {
					return silex_statuscodes_to_psa(sx_status);
				}
			} else {
				psa_status = initialize_cipher(operation);

				if (psa_status != PSA_SUCCESS) {
					return psa_status;
				}
			}

			if (operation->unprocessed_input_bytes) {
				__ASSERT_NO_MSG(operation->unprocessed_input_bytes ==
						operation->blk_size);
				sx_status = sx_blkcipher_crypt(
					&operation->cipher, operation->unprocessed_input,
					operation->unprocessed_input_bytes, output);
				if (sx_status != SX_OK) {
					return silex_statuscodes_to_psa(sx_status);
				}

				operation->unprocessed_input_bytes = 0;
				/* Adjust output pointer. */
				output += (uint32_t)operation->blk_size;
			}

			if (block_bytes) {
				sx_status = sx_blkcipher_crypt(&operation->cipher, input,
							       block_bytes, output);
				if (sx_status != SX_OK) {
					return silex_statuscodes_to_psa(sx_status);
				}
			}

			sx_status = sx_blkcipher_save_state(&operation->cipher);
			if (sx_status != SX_OK) {
				return silex_statuscodes_to_psa(sx_status);
			}

			sx_status = sx_blkcipher_wait(&operation->cipher);
			if (sx_status != SX_OK) {
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

	(void)psa_status;
	return PSA_SUCCESS;
}

psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length)
{
	__ASSERT_NO_MSG(output_length != NULL);

	int sx_status;
	psa_status_t psa_status;

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

	if (IS_ENABLED(PSA_NEED_CRACEN_CBC_NO_PADDING_AES) ||
	    IS_ENABLED(PSA_NEED_CRACEN_CBC_PKCS7_AES)) {
		if (operation->alg == PSA_ALG_CBC_NO_PADDING ||
		    operation->alg == PSA_ALG_CBC_PKCS7) {
			return cracen_sw_aes_cbc_finish(operation, output, output_size,
							output_length);
		}
	}

	if (operation->unprocessed_input_bytes == 0) {
		return PSA_SUCCESS;
	}

	sx_status = SX_ERR_UNINITIALIZED_OBJ;

	/* sxsymcrypt doesn't support context saving for ECB, as each encrypted block is
	 * independent from the previous one we just encrypt the so far available full
	 * blocks.
	 */
	if (IS_ENABLED(PSA_NEED_CRACEN_ECB_NO_PADDING_AES)) {
		if (operation->alg == PSA_ALG_ECB_NO_PADDING) {
			if (operation->dir == CRACEN_ENCRYPT) {
				return cracen_sw_aes_ecb_encrypt(&operation->cipher,
								 &operation->keyref,
								 operation->unprocessed_input,
								 operation->unprocessed_input_bytes,
								 output, output_size,
								 output_length);
			} else {
				return cracen_sw_aes_ecb_decrypt(&operation->cipher,
								 &operation->keyref,
								 operation->unprocessed_input,
								 operation->unprocessed_input_bytes,
								 output, output_size,
								 output_length);
			}
		}
	}
	if (operation->initialized) {
		sx_status = sx_blkcipher_resume_state(&operation->cipher);
		if (sx_status != SX_OK) {
			return silex_statuscodes_to_psa(sx_status);
		}
	} else {
		psa_status = initialize_cipher(operation);

		if (psa_status != PSA_SUCCESS) {
			return psa_status;
		}
	}

	*output_length = operation->unprocessed_input_bytes;
	if (*output_length > output_size) {
		*output_length = 0;
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	sx_status = sx_blkcipher_crypt(&operation->cipher, operation->unprocessed_input,
				       operation->unprocessed_input_bytes, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(&operation->cipher);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_wait(&operation->cipher);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return PSA_SUCCESS;
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
