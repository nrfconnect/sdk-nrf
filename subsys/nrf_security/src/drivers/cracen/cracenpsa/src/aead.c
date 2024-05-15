/**
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sxsymcrypt/aead.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/chachapoly.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/__assert.h>

#include "common.h"
#include <cracen/mem_helpers.h>

/* CCM, GCM and ChaCha20-Poly1305 have 16 byte tags by default */
#define DEFAULT_TAG_SIZE 16

/*
 * This function assumes it is given a valid algorithm for aead.
 */
static psa_key_type_t alg_to_key_type(psa_algorithm_t alg)
{

	switch (alg) {
	case PSA_ALG_GCM:
		IF_ENABLED(PSA_NEED_CRACEN_GCM_AES, (return PSA_KEY_TYPE_AES));
		break;
	case PSA_ALG_CCM:
		IF_ENABLED(PSA_NEED_CRACEN_CCM_AES, (return PSA_KEY_TYPE_AES));
		break;
	case PSA_ALG_CHACHA20_POLY1305:
		IF_ENABLED(PSA_NEED_CRACEN_CHACHA20_POLY1305, (return PSA_KEY_TYPE_CHACHA20));
		break;
	}

	return PSA_KEY_TYPE_NONE;
}

static bool is_key_type_supported(psa_algorithm_t alg, const psa_key_attributes_t *attributes)
{
	return psa_get_key_type(attributes) == alg_to_key_type(alg);
}

static bool is_nonce_length_supported(psa_algorithm_t alg, size_t nonce_length)
{
	_Static_assert((12 == SX_GCM_IV_SZ) && (12 == SX_CHACHAPOLY_IV_SZ));

	switch (alg) {
	case PSA_ALG_GCM:
	case PSA_ALG_CHACHA20_POLY1305:
		return nonce_length == 12u;
	case PSA_ALG_CCM:
		return sx_aead_aesccm_nonce_size_is_valid(nonce_length);
	}

	return false;
}

static uint8_t get_tag_size(psa_algorithm_t alg, size_t key_buffer_size)
{
	return PSA_AEAD_TAG_LENGTH(alg_to_key_type(PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg)),
				   PSA_BYTES_TO_BITS(key_buffer_size), alg);
}

/*
 * There are three supported algorithms GCM, CCM, and CHACHA20_POLY1305.
 *
 * GCM and CCM have a block size of 16, AKA
 * PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)
 *
 * ChaCha20 operates internally with 64 byte blocks, so does the sxsymcrypt
 * driver
 */
static uint8_t get_block_size(psa_algorithm_t alg)
{
	switch (alg) {
	case PSA_ALG_GCM:
	case PSA_ALG_CCM:
		return 16;
	case PSA_ALG_CHACHA20_POLY1305:
		return 64;
	}

	__ASSERT_NO_MSG(false);
	return 0;
}

static psa_status_t process_on_hw(cracen_aead_operation_t *operation)
{
	int sx_status = sx_aead_save_state(&operation->ctx);

	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_aead_wait(&operation->ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}
	operation->context_state = CRACEN_CONTEXT_INITIALIZED;
	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t initialize_ctx(cracen_aead_operation_t *operation)
{
	int sx_status = SX_ERR_INCOMPATIBLE_HW;

	/* If the nonce_length is wrong then we must be in a bad state */
	if (!is_nonce_length_supported(operation->alg, operation->nonce_length)) {
		return PSA_ERROR_BAD_STATE;
	}

	switch (operation->alg) {
	case PSA_ALG_GCM:
		if (IS_ENABLED(PSA_NEED_CRACEN_GCM_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_aead_create_aesgcm_dec(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->tag_size)
					    : sx_aead_create_aesgcm_enc(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->tag_size);
		}
		break;
	case PSA_ALG_CCM:
		if (IS_ENABLED(PSA_NEED_CRACEN_CCM_AES)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_aead_create_aesccm_dec(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->nonce_length,
						      operation->tag_size, operation->ad_length,
						      operation->plaintext_length)
					    : sx_aead_create_aesccm_enc(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->nonce_length,
						      operation->tag_size, operation->ad_length,
						      operation->plaintext_length);
		}
		break;
	case PSA_ALG_CHACHA20_POLY1305:
		if (IS_ENABLED(PSA_NEED_CRACEN_CHACHA20_POLY1305)) {
			sx_status = operation->dir == CRACEN_DECRYPT
					    ? sx_aead_create_chacha20poly1305_dec(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->tag_size)
					    : sx_aead_create_chacha20poly1305_enc(
						      &operation->ctx, &operation->keyref,
						      operation->nonce, operation->tag_size);
		}
		break;
	default:
		sx_status = SX_ERR_INCOMPATIBLE_HW;
		break;
	}

	return silex_statuscodes_to_psa(sx_status);
}

static psa_status_t initialize_or_resume_context(cracen_aead_operation_t *operation)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	switch (operation->context_state) {
	case CRACEN_NOT_INITIALIZED:
		status = initialize_ctx(operation);

		operation->context_state = CRACEN_HW_RESERVED;
		break;
	case CRACEN_CONTEXT_INITIALIZED:
		status = silex_statuscodes_to_psa(sx_aead_resume_state(&operation->ctx));

		operation->context_state = CRACEN_HW_RESERVED;
		break;
	case CRACEN_HW_RESERVED:
		status = PSA_SUCCESS;
	}

	return status;
}

static void cracen_writebe(uint8_t *out, uint64_t data, uint16_t targetsz)
{
	uint_fast16_t i;

	for (i = 0; i < targetsz; i++) {
		out[(targetsz - 1) - i] = (data >> (i * 8)) & 0xFF;
	}
}

static psa_status_t create_aead_ccmheader(cracen_aead_operation_t *operation)
{
	uint8_t flags;
	size_t m, l;
	uint8_t header[26] = {0};
	size_t header_size = 16;

	/* RFC3610 paragraph 2.2 defines the formatting of the first block.
	 * M, CCM TAG size is one of {4,6,8,10,12,14,16}, CCM* not supported
	 * (MAC size 0)
	 * L must be between 2 and 8.
	 * Nonce size should be between 7 and 13 bytes.
	 * The first block contains:
	 *  byte  [0]           the flags byte (see below)
	 *  bytes [1,1+nonce.len]   nonce
	 *  bytes [2+nonce.len, 16] message length
	 *
	 *  The flags byte has the following bit fields:
	 *    [7]   = 0 Reserved
	 *    [6]   = 1 if ad data provided, else 0
	 *    [3:5] = authentication tag size, encoded as (tagsz-2)/2
	 *              only multiples of 2 between 2 and 16 are allowed.
	 *    [0:2] = length field size minus 1.
	 **/
	l = 15 - operation->nonce_length;

	flags = (operation->ad_length > 0) ? (1 << 6) : 0;
	m = (operation->tag_size - 2) / 2;

	flags |= (m & 0x7) << 3;
	flags |= ((l - 1) & 0x7);
	header[0] = flags;

	memcpy(&header[1], (void *)operation->nonce, operation->nonce_length);

	cracen_writebe(&(header[1 + operation->nonce_length]), operation->plaintext_length, l);

	/*
	 * If there is additional authentication data, encode the size into
	 * bytes [16, 17/21/25] depending on the length
	 */
	if (operation->ad_length > 0) {
		if (operation->ad_length < 0xFF00) {
			cracen_writebe(&header[16], operation->ad_length, 2);
			header_size += 2;
		} else if (operation->ad_length <= 0xFFFFFFFF) {
			header[16] = 0xFF;
			header[17] = 0xFE;
			cracen_writebe(&header[18], operation->ad_length, 4);
			header_size += 6;
		} else {
			header[16] = 0xFF;
			header[17] = 0xFF;
			cracen_writebe(&header[18], operation->ad_length, 8);
			header_size += 10;
		}
	}

	return cracen_aead_update_ad(operation, header, header_size);
}

static psa_status_t setup(cracen_aead_operation_t *operation, enum cipher_operation dir,
			  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			  size_t key_buffer_size, psa_algorithm_t alg)
{
	/*
	 * All algorithms in PSA Crypto 1.1.0 (PSA_ALG_CCM, PSA_ALG_GCM,
	 * and PSA_ALG_CHACHA20_POLY1305) are supported by this driver and
	 * the Oberon PSA Core does input validation that the algorithm is
	 * an AEAD algorithm so we omit input-validation of alg here.
	 *
	 * The operation must be inactive, but this is validated by the
	 * PSA Core so we do not need to validate it here.
	 */

	/*
	 * It is not clear if the Oberon PSA Core does input validation of
	 * the key attributes or not and this driver will later assume the
	 * key type is appropriate so we check it here to be safe.
	 */
	if (!is_key_type_supported(PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg), attributes)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	/*
	 * Copy the key into the operation struct as it is not guaranteed to be
	 * valid longer than the function call
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

	operation->alg = PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg);
	operation->dir = dir;
	operation->tag_size = get_tag_size(alg, key_buffer_size);

	/*
	 * At this point the nonce is not known, which is required to
	 * create a sxsymcrypt context, therefore all values are stored
	 * in the psa operation context.
	 */
	return status;
}

static psa_status_t cracen_feed_data_to_hw(cracen_aead_operation_t *operation, const uint8_t *input,
					   size_t input_length, uint8_t *output, bool is_ad_update)
{
	int sx_status;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;

	psa_status = initialize_or_resume_context(operation);
	if (psa_status) {
		return psa_status;
	}
	if (is_ad_update) {
		sx_status = sx_aead_feed_aad(&operation->ctx, input, input_length);
	} else {
		sx_status = sx_aead_crypt(&operation->ctx, input, input_length, output);
	}

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg)
{
	return setup(operation, CRACEN_ENCRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg)
{
	return setup(operation, CRACEN_DECRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length)
{
	if (!is_nonce_length_supported(operation->alg, nonce_length)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	memcpy(operation->nonce, nonce, nonce_length);
	operation->nonce_length = nonce_length;

	/* Create the CCM header */
	if (operation->alg == PSA_ALG_CCM) {
		return create_aead_ccmheader(operation);
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length)
{
	operation->ad_length = ad_length;
	operation->plaintext_length = plaintext_length;

	return PSA_SUCCESS;
}

static psa_status_t cracen_aead_update_internal(cracen_aead_operation_t *operation,
						const uint8_t *input, size_t input_length,
						uint8_t *output, size_t output_size,
						size_t *output_length, bool is_ad_update)
{
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blk_bytes = 0;
	size_t out_bytes = 0;

	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	if (operation->unprocessed_input_bytes || input_length < get_block_size(operation->alg)) {
		uint8_t remaining_bytes =
			get_block_size(operation->alg) - operation->unprocessed_input_bytes;
		if (input_length <= remaining_bytes) {
			memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes,
			       input, input_length);
			operation->unprocessed_input_bytes += input_length;
			/* The output_length can be NULL when we process the additional data because
			 * the value is not needed by any of the supported algorithms.
			 */
			if (output_length != NULL) {
				*output_length = 0;
			}
			return PSA_SUCCESS;
		}

		memcpy(operation->unprocessed_input + operation->unprocessed_input_bytes, input,
		       remaining_bytes);
		input += remaining_bytes;
		input_length -= remaining_bytes;
		operation->unprocessed_input_bytes += remaining_bytes;
	}

	if (operation->unprocessed_input_bytes == get_block_size(operation->alg)) {
		if (!is_ad_update && output_size < operation->unprocessed_input_bytes) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		psa_status = cracen_feed_data_to_hw(operation, operation->unprocessed_input,
						    operation->unprocessed_input_bytes, output,
						    is_ad_update);
		if (psa_status) {
			return psa_status;
		}
		if (!is_ad_update) {
			output = output + operation->unprocessed_input_bytes;
		}

		out_bytes = operation->unprocessed_input_bytes;
		operation->unprocessed_input_bytes = 0;
	}

	/* Clamp input length to a multiple of the block size. */
	blk_bytes = input_length & ~(get_block_size(operation->alg) - 1);

	/* For CCM, sxsymcrypt driver needs a chunk of input data to produce a tag
	 * therefore we buffer the last block until finish will be called.
	 * blk_bytes tracks the amount of block-sized input that will be
	 * processed immediately. So to buffer the input and prevent processing
	 * we subtract one block from blk_bytes.
	 */
	if (operation->alg == PSA_ALG_CCM && input_length != 0 && blk_bytes == input_length) {
		blk_bytes -= get_block_size(operation->alg);
	}

	if (blk_bytes > 0) {
		if (!is_ad_update && (output_size < (blk_bytes + out_bytes))) {
			return PSA_ERROR_BUFFER_TOO_SMALL;
		}

		/* Feed the full blocks to the device. */
		psa_status =
			cracen_feed_data_to_hw(operation, input, blk_bytes, output, is_ad_update);
		if (psa_status) {
			return psa_status;
		}

		input += blk_bytes;
		input_length -= blk_bytes;
		out_bytes += blk_bytes;
	}

	/* Only process on HW if data has been fed */
	if (out_bytes != 0) {
		psa_status = process_on_hw(operation);
		if (psa_status) {
			return psa_status;
		}
		if (output_length) {
			*output_length = out_bytes;
		}
	}

	/* Copy remaining bytes to be encrypted later. */
	size_t remaining_bytes = input_length;

	if (remaining_bytes) {
		memcpy(operation->unprocessed_input, input, remaining_bytes);
		operation->unprocessed_input_bytes = remaining_bytes;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length)
{
	return cracen_aead_update_internal(operation, input, input_length, NULL, 0, NULL, true);
}

psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	/*
	 * Even if no plain/ciphertext is provided we still wanna have one block
	 * of AD buffered before creating/verifying the tag
	 */
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

	/* If there is unprocessed "additional data", then feed it before
	 * feeding the plaintext.
	 */
	if (operation->unprocessed_input_bytes && !operation->ad_finished) {
		psa_status_t status =
			cracen_feed_data_to_hw(operation, operation->unprocessed_input,
					       operation->unprocessed_input_bytes, NULL, true);
		if (status) {
			return status;
		}

		status = process_on_hw(operation);
		if (status) {
			return status;
		}

		operation->unprocessed_input_bytes = 0;
	}

	operation->ad_finished = true;

	return cracen_aead_update_internal(operation, input, input_length, output, output_size,
					   output_length, false);
}

psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length)
{
	int sx_status = SX_ERR_UNINITIALIZED_OBJ;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if ((tag_size < operation->tag_size) ||
	    (operation->ad_finished && (ciphertext_size < operation->unprocessed_input_bytes))) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->unprocessed_input_bytes) {
		status = cracen_feed_data_to_hw(operation, operation->unprocessed_input,
						operation->unprocessed_input_bytes, ciphertext,
						!operation->ad_finished);
		if (status) {
			return status;
		}
		if (operation->ad_finished) {
			*ciphertext_length = operation->unprocessed_input_bytes;
		}
	} else {
		/* In this case plaintext and AD has length 0 and we don't have a context.
		 * Initialize it here, so we can produce the tag.
		 */
		initialize_or_resume_context(operation);
	}

	sx_status = sx_aead_produce_tag(&operation->ctx, tag);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_aead_wait(&operation->ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	*tag_length = operation->tag_size;

	safe_memzero((void *)operation, sizeof(cracen_aead_operation_t));

	return PSA_SUCCESS;
}

psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length)
{
	int sx_status = SX_ERR_UNINITIALIZED_OBJ;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->ad_finished && (plaintext_size < operation->unprocessed_input_bytes)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (operation->unprocessed_input_bytes) {
		status = cracen_feed_data_to_hw(operation, operation->unprocessed_input,
						operation->unprocessed_input_bytes, plaintext,
						!operation->ad_finished);
		if (status) {
			return status;
		}
		if (operation->ad_finished) {
			*plaintext_length = operation->unprocessed_input_bytes;
		}
	} else {
		/* In this case ciphertext and AD has length 0 and we don't have a context.
		 * Initialize it here, so we can verify the tag.
		 */
		initialize_or_resume_context(operation);
	}

	/* Finish the AEAD decryption and tag validation. */
	sx_status = sx_aead_verify_tag(&operation->ctx, tag);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_aead_wait(&operation->ctx);
	if (sx_status) {
		return silex_statuscodes_to_psa(sx_status);
	}

	safe_memzero((void *)operation, sizeof(cracen_aead_operation_t));

	return silex_statuscodes_to_psa(sx_status);
}

psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation)
{
	safe_memzero((void *)operation, sizeof(cracen_aead_operation_t));
	return PSA_SUCCESS;
}

psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_aead_operation_t operation = {0};
	size_t update_length = 0;
	uint8_t local_tag_buffer[PSA_AEAD_TAG_MAX_SIZE] = {0};
	size_t tag_length = 0;

	status =
		cracen_aead_encrypt_setup(&operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_set_lengths(&operation, additional_data_length, plaintext_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_set_nonce(&operation, nonce, nonce_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_update_ad(&operation, additional_data, additional_data_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_update(&operation, plaintext, plaintext_length, ciphertext,
				    ciphertext_size, ciphertext_length);
	if (status != PSA_SUCCESS) {
		*ciphertext_length = 0;
		goto error_exit;
	}

	status = cracen_aead_finish(&operation, &ciphertext[*ciphertext_length],
				    ciphertext_size - *ciphertext_length, &update_length,
				    local_tag_buffer, sizeof(local_tag_buffer), &tag_length);
	if (status != PSA_SUCCESS) {
		*ciphertext_length = 0;
		goto error_exit;
	}
	*ciphertext_length += update_length;

	/* Copy tag to the end of the ciphertext buffer, if big enough */
	if (*ciphertext_length + tag_length > ciphertext_size) {
		*ciphertext_length = 0;
		status = PSA_ERROR_BUFFER_TOO_SMALL;
		goto error_exit;
	}
	memcpy(&ciphertext[*ciphertext_length], &local_tag_buffer, tag_length);
	*ciphertext_length += tag_length;

	return status;

error_exit:
	cracen_aead_abort(&operation);
	return status;
}

psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_aead_operation_t operation = {0};
	size_t update_length = 0;

	status =
		cracen_aead_decrypt_setup(&operation, attributes, key_buffer, key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_set_lengths(&operation, additional_data_length,
					 ciphertext_length - operation.tag_size);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_set_nonce(&operation, nonce, nonce_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_update_ad(&operation, additional_data, additional_data_length);
	if (status != PSA_SUCCESS) {
		goto error_exit;
	}

	status = cracen_aead_update(&operation, ciphertext, ciphertext_length - operation.tag_size,
				    plaintext, plaintext_size, plaintext_length);
	if (status != PSA_SUCCESS) {
		*plaintext_length = 0;
		goto error_exit;
	}

	status = cracen_aead_verify(&operation, &plaintext[*plaintext_length],
				    plaintext_size - *plaintext_length, &update_length,
				    &ciphertext[ciphertext_length - operation.tag_size],
				    operation.tag_size);
	if (status != PSA_SUCCESS) {
		*plaintext_length = 0;
		goto error_exit;
	}
	*plaintext_length += update_length;

	return status;
error_exit:
	cracen_aead_abort(&operation);
	return status;
}
