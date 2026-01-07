/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <string.h>
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>

#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/aead.h>
#include <sxsymcrypt/chachapoly.h>
#include "../../../sxsymcrypt/src/blkcipherdefs.h"
#include "../../../sxsymcrypt/src/aeaddefs.h"
#include "../../../sxsymcrypt/src/cmdma.h"

#include <cracen/statuscodes.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <cracen_psa_primitives.h>
#include "../../../cracenpsa/src/common.h"
#include "poly1305_ext.h"
#include <cracen_sw_common.h>
#include <cracen_sw_aead.h>
#include <cracen_sw_chacha20_poly1305.h>

/* Nonce size must be 96 bits (RFC8439) */
#define CHACHA20_POLY1305_VALID_NONCE_LEN	12u

static psa_status_t cracen_chacha20_crypt(struct sxblkcipher *blkciph,
					  const uint8_t *input, size_t input_length,
					  uint8_t *output, size_t output_size,
					  size_t *output_length)
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

static psa_status_t cracen_chacha20_primitive(struct sxblkcipher *blkciph, struct sxkeyref *key,
				       const uint8_t *counter, const uint8_t *nonce,
				       const uint8_t *input, uint8_t *output, size_t data_size)
{
	int sx_status;
	size_t output_size;

	sx_status = sx_blkcipher_create_chacha20_enc(blkciph, key, counter, nonce);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return cracen_chacha20_crypt(blkciph, input, data_size, output, data_size, &output_size);
}

static bool is_nonce_length_valid(size_t nonce_length)
{
	return nonce_length == CHACHA20_POLY1305_VALID_NONCE_LEN;
}

static bool is_tag_length_valid(size_t tag_length)
{
	return tag_length == CRACEN_POLY1305_TAG_SIZE;
}

static psa_status_t increment_counter(uint8_t *ctr)
{
	for (size_t i = CRACEN_CHACHA20_COUNTER_SIZE; i > 0; i--) {
		if (++ctr[i - 1] != 0) {
			return PSA_SUCCESS;
		}
	}

	/* All counter bytes wrapped to zero which means it overflowed */
	return PSA_ERROR_INVALID_ARGUMENT;
}

/* Encode value as big-endian, right-aligned in buffer */
static void encode_big_endian_value(uint8_t *buffer, size_t buffer_size, size_t value,
				     size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[buffer_size - 1 - i] = value >> (i * 8);
	}
}

/* Encode value as little-endian, left-aligned in buffer */
static void encode_little_endian_value(uint8_t *buffer, size_t buffer_size, size_t value,
				       size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[i] = value >> (i * 8);
	}
}

static psa_status_t setup(cracen_aead_operation_t *operation, enum cipher_operation dir,
			  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			  size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t tag_size;

	safe_memzero(&operation->sw_chacha_poly_ctx, sizeof(operation->sw_chacha_poly_ctx));

	if (key_buffer_size > sizeof(operation->key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	tag_size = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_CHACHA20,
				       PSA_BYTES_TO_BITS(key_buffer_size), alg);
	if (!is_tag_length_valid(tag_size)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->key_buffer, key_buffer, key_buffer_size);
	status = cracen_load_keyref(attributes, operation->key_buffer, key_buffer_size,
				    &operation->keyref);
	if (status != PSA_SUCCESS) {
		return status;
	}
	safe_memzero(operation->unprocessed_input, CRACEN_MAX_AEAD_BLOCK_SIZE);
	operation->unprocessed_input_bytes = 0;
	operation->alg = PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg);
	operation->dir = dir;
	operation->tag_size = tag_size;
	return status;
}

static psa_status_t initialize_poly_key(cracen_aead_operation_t *operation,
				     struct sxblkcipher *cipher)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	/* feeding zeros as the input should return "keystream" value (RFC8439, 2.4.1) */
	const uint8_t zero[SX_BLKCIPHER_CHACHA20_BLK_SZ] = {};
	const uint8_t zero_counter[CRACEN_CHACHA20_COUNTER_SIZE] = {};
	uint8_t res_block[SX_BLKCIPHER_CHACHA20_BLK_SZ] = {};
	uint8_t onetime_key[CRACEN_POLY1305_KEY_SIZE] = {};
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (chacha_poly_ctx->poly_initialized) {
		return PSA_SUCCESS;
	}

	status = cracen_chacha20_primitive(cipher, &operation->keyref, zero_counter,
					   operation->nonce, zero, res_block,
					   SX_BLKCIPHER_CHACHA20_BLK_SZ);

	if (status == PSA_SUCCESS) {
		/** RFC8439:
		 *  We take the first 256 bits of the serialized state,
		 *  and use those as the one-time Poly1305 key:
		 *  the first 128 bits are clamped and form "r", while
		 *  the next 128 bits become "s".
		 *  The other 256 bits are discarded.
		 */
		memcpy(onetime_key, res_block, CRACEN_POLY1305_KEY_SIZE);
		poly1305_ext_starts(&chacha_poly_ctx->poly_ctx, onetime_key);
		chacha_poly_ctx->poly_initialized = true;
	}
	safe_memzero(res_block, SX_BLKCIPHER_CHACHA20_BLK_SZ);
	safe_memzero(onetime_key, CRACEN_POLY1305_KEY_SIZE);
	return status;
}

static void initialize_ctr(cracen_aead_operation_t *operation)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;

	if (chacha_poly_ctx->ctr_initialized) {
		return;
	}
	/* RFC8439: initial counter value is 1 */
	encode_big_endian_value(chacha_poly_ctx->ctr, CRACEN_CHACHA20_COUNTER_SIZE, 1, 1);
	chacha_poly_ctx->ctr_initialized = true;
}

static void calc_poly1305_mac(cracen_aead_operation_t *operation, const uint8_t *input,
			   size_t input_len)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;

	for (size_t i = 0; i < input_len; i++) {
		operation->unprocessed_input[operation->unprocessed_input_bytes++] = input[i];
		if (operation->unprocessed_input_bytes == CRACEN_POLY1305_TAG_SIZE) {
			poly1305_ext_process(&chacha_poly_ctx->poly_ctx, 1,
					     operation->unprocessed_input, 1);

			operation->unprocessed_input_bytes = 0;
		}
	}
}

static psa_status_t calc_chacha20(cracen_aead_operation_t *operation, struct sxblkcipher *cipher,
			    const uint8_t *input, uint8_t *output, size_t length)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	status = cracen_chacha20_primitive(cipher, &operation->keyref,
					   chacha_poly_ctx->ctr,
					   operation->nonce, input, output, length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (length == SX_BLKCIPHER_CHACHA20_BLK_SZ) {
		return increment_counter(chacha_poly_ctx->ctr);
	}
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_chacha20_poly1305_encrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_ENCRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_chacha20_poly1305_decrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_DECRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_chacha20_poly1305_set_nonce(cracen_aead_operation_t *operation,
						   const uint8_t *nonce, size_t nonce_length)
{
	if (!is_nonce_length_valid(nonce_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->nonce, nonce, nonce_length);
	operation->nonce_length = nonce_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_chacha20_poly1305_set_lengths(cracen_aead_operation_t *operation,
						     size_t ad_length, size_t plaintext_length)
{
	operation->ad_length = ad_length;
	operation->plaintext_length = plaintext_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_chacha20_poly1305_update_ad(cracen_aead_operation_t *operation,
						   const uint8_t *input, size_t input_length)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (operation->ad_finished) {
		return PSA_ERROR_BAD_STATE;
	}

	status = initialize_poly_key(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	calc_poly1305_mac(operation, input, input_length);
	chacha_poly_ctx->total_ad_fed += input_length;

	return status;
}

static void finalize_ad_padding(cracen_aead_operation_t *operation)
{
	uint8_t padding_block[CRACEN_POLY1305_TAG_SIZE] = {0};

	if (operation->ad_finished) {
		return;
	}

	/** ChaCha20-Poly1305 requires AD to be padded to block boundary
	 *  before processing plaintext
	 */
	if (operation->unprocessed_input_bytes != 0) {
		/* Apply zero padding */
		calc_poly1305_mac(operation, padding_block,
				  CRACEN_POLY1305_TAG_SIZE - operation->unprocessed_input_bytes);
	}
}

/* Finalize any partial data block with zero-padding and update MAC */
static void finalize_data_padding(cracen_aead_operation_t *operation)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	uint8_t padding_block[CRACEN_POLY1305_TAG_SIZE] = {0};

	if (operation->unprocessed_input_bytes != 0) {
		/* Apply zero padding */
		calc_poly1305_mac(operation, padding_block,
				  CRACEN_POLY1305_TAG_SIZE - operation->unprocessed_input_bytes);
	}
	safe_memzero(padding_block, sizeof(padding_block));

	/* Poly1305( [len(AAD)]64 || [LEN(C)]64 ) */
	encode_little_endian_value(padding_block,
				   CRACEN_POLY1305_TAG_SIZE / 2,
				   chacha_poly_ctx->total_ad_fed,
				   CRACEN_POLY1305_TAG_SIZE / 2);
	encode_little_endian_value(padding_block + CRACEN_POLY1305_TAG_SIZE / 2,
				   CRACEN_POLY1305_TAG_SIZE / 2,
				   chacha_poly_ctx->total_data_enc,
				   CRACEN_POLY1305_TAG_SIZE / 2);

	calc_poly1305_mac(operation, padding_block, CRACEN_POLY1305_TAG_SIZE);

	operation->unprocessed_input_bytes = 0;
	safe_memzero(padding_block, sizeof(padding_block));
}

psa_status_t cracen_sw_chacha20_poly1305_update(cracen_aead_operation_t *operation,
						const uint8_t *input, size_t input_length,
						uint8_t *output, size_t output_size,
						size_t *output_length)
{
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t processed = 0;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = initialize_poly_key(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	operation->ad_finished = true;

	safe_memzero(output, output_size);

	/* Process data with CTR mode encryption/decryption */
	if (operation->dir == CRACEN_ENCRYPT) {
		/* Encrypt: apply ChaCha20 keystream, then calc Poly1305 MAC */
		while (processed < input_length) {
			size_t chunk_size = MIN(input_length - processed,
						SX_BLKCIPHER_CHACHA20_BLK_SZ -
							operation->unprocessed_input_bytes);

			status = calc_chacha20(operation, &cipher, &input[processed],
					       &output[processed], chunk_size);
			if (status != PSA_SUCCESS) {
				return status;
			}

			calc_poly1305_mac(operation, &output[processed], chunk_size);
			processed += chunk_size;
		}
	} else {
		/** Decryption does the reverse of encryption, so apply Poly1305 first,
		 * then ChaCha20
		 */
		while (processed < input_length) {
			size_t chunk_size = MIN(input_length - processed,
						SX_BLKCIPHER_CHACHA20_BLK_SZ -
							operation->unprocessed_input_bytes);

			calc_poly1305_mac(operation, &input[processed], chunk_size);

			status = calc_chacha20(operation, &cipher, &input[processed],
					       &output[processed], chunk_size);
			if (status != PSA_SUCCESS) {
				return status;
			}
			processed += chunk_size;
		}
	}
	*output_length = processed;
	chacha_poly_ctx->total_data_enc += processed;
	return status;
}

psa_status_t cracen_sw_chacha20_poly1305_finish(cracen_aead_operation_t *operation,
						uint8_t *ciphertext, size_t ciphertext_size,
						size_t *ciphertext_length, uint8_t *tag,
						size_t tag_size, size_t *tag_length)
{
	struct sxblkcipher cipher;
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (tag_size < operation->tag_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = initialize_poly_key(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	finalize_data_padding(operation);

	poly1305_ext_compute_mac(&chacha_poly_ctx->poly_ctx, tag);
	*tag_length = operation->tag_size;
	return status;
}

psa_status_t cracen_sw_chacha20_poly1305_verify(cracen_aead_operation_t *operation,
						uint8_t *plaintext, size_t plaintext_size,
						size_t *plaintext_length, const uint8_t *tag,
						size_t tag_length)
{
	struct sxblkcipher cipher;
	cracen_sw_chacha20_poly1305_context_t *chacha_poly_ctx = &operation->sw_chacha_poly_ctx;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t computed_tag[CRACEN_POLY1305_TAG_SIZE] = {0};
	uint32_t tag_mismatch = 0;

	status = initialize_poly_key(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	finalize_data_padding(operation);

	poly1305_ext_compute_mac(&chacha_poly_ctx->poly_ctx, computed_tag);

	/* Constant-time tag comparison to prevent timing attacks */
	tag_mismatch = constant_memcmp(computed_tag, tag, operation->tag_size);
	if (tag_mismatch != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
	}

	safe_memzero(computed_tag, sizeof(computed_tag));
	return status;
}

psa_status_t cracen_sw_chacha20_poly1305_abort(cracen_aead_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
