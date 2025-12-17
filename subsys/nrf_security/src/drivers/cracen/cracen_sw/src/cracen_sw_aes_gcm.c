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
#include <sxsymcrypt/internal.h>
#include <sxsymcrypt/keyref.h>
#include <cracen/statuscodes.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <cracen_psa_primitives.h>
#include "../../../cracenpsa/src/common.h"
#include <cracen_sw_common.h>
#include <cracen_sw_aead.h>
#include <cracen_sw_aes_gcm.h>

/* PSA Crypto standard specifies nonce size of at least 1 byte */
#define GCM_VALID_NONCE_LEN		12u

#define GCM_MIN_TAG_SIZE		12u
#define GCM_MAX_TAG_SIZE		16u
/* For certain applications, tag size may be 64 or 32 bits (NIST SP800-38D) */
#define GCM_SPECIAL_TAG_SIZE_1		4u
#define GCM_SPECIAL_TAG_SIZE_2		8u

/* Compute Q (length field size) from nonce length: Q = 16 - nonce_len */
#define GCM_Q_LEN_FROM_NONCE(nonce_len) (SX_BLKCIPHER_AES_BLK_SZ - (nonce_len))

static bool is_nonce_length_valid(size_t nonce_length)
{
	return nonce_length == GCM_VALID_NONCE_LEN;
}

static bool is_tag_length_valid(size_t tag_length)
{
	return (tag_length >= GCM_MIN_TAG_SIZE && tag_length <= GCM_MAX_TAG_SIZE) ||
	       (tag_length == GCM_SPECIAL_TAG_SIZE_1) || (tag_length == GCM_SPECIAL_TAG_SIZE_2);
}

static psa_status_t increment_counter(uint8_t *ctr, size_t counter_size)
{
	size_t start_pos = SX_BLKCIPHER_AES_BLK_SZ - counter_size;

	for (size_t i = SX_BLKCIPHER_AES_BLK_SZ; i > start_pos; i--) {
		if (++ctr[i - 1] != 0) {
			return PSA_SUCCESS;
		}
	}

	/* All counter bytes wrapped to zero which means it overflowed */
	return PSA_ERROR_INVALID_ARGUMENT;
}

/* Encode value as big-endian, right-aligned in buffer */
static void encode_big_endian_length(uint8_t *buffer, size_t buffer_size, size_t value,
				     size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[buffer_size - 1 - i] = value >> (i * 8);
	}
}

static void rshift_block(uint8_t *block)
{
	uint8_t result[SX_BLKCIPHER_AES_BLK_SZ];
	cracen_be_rshift(block, 1, result, SX_BLKCIPHER_AES_BLK_SZ);
	memcpy(block, result, SX_BLKCIPHER_AES_BLK_SZ);
}

/* Multiplication operation on blocks in GF(2^128) */
static void multiply_blocks_gf(const uint8_t *block_x, const uint8_t *block_y,
			       uint8_t *block_product)
{
	const uint8_t r[SX_BLKCIPHER_AES_BLK_SZ] = {0xE1};
	uint8_t v[SX_BLKCIPHER_AES_BLK_SZ] = {};

	safe_memzero(block_product, SX_BLKCIPHER_AES_BLK_SZ);
	memcpy(v, block_y, SX_BLKCIPHER_AES_BLK_SZ);

	for (size_t byte = 0; byte < SX_BLKCIPHER_AES_BLK_SZ; byte++) {
		for (size_t bit = 0; bit < PSA_BYTES_TO_BITS(sizeof(uint8_t)); bit++) {
			if (block_x[byte] & (1 << (7 - bit))) {
				cracen_xorbytes(block_product, v, SX_BLKCIPHER_AES_BLK_SZ);
			}

			if (v[SX_BLKCIPHER_AES_BLK_SZ - 1] & 0x01) {
				rshift_block(v);
				cracen_xorbytes(v, r, SX_BLKCIPHER_AES_BLK_SZ);
			} else {
				rshift_block(v);
			}
		}
	}
}

/** GHASH_H(X1 || X2 || ... || Xm) = Ym
 *  Note: the current implementation follows NIST SP800-38D,
 *  no Shoup's tables are used now
 */
static void calc_gcm_ghash(cracen_aead_operation_t *operation, const uint8_t *input,
			   size_t input_len)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	uint8_t product[SX_BLKCIPHER_AES_BLK_SZ] = {};

	for (size_t i = 0; i < input_len; i++) {
		operation->unprocessed_input[operation->unprocessed_input_bytes++] = input[i];
		if (operation->unprocessed_input_bytes == SX_BLKCIPHER_AES_BLK_SZ) {
			/** The size of the input data chunk of GHASH algorithm
			 * is expected to be multiple of block size (NIST SP800-38D)
			 */
			cracen_xorbytes(gcm_ctx->ghash_block, operation->unprocessed_input,
					SX_BLKCIPHER_AES_BLK_SZ);
			multiply_blocks_gf(gcm_ctx->h, gcm_ctx->ghash_block, product);
			memcpy(gcm_ctx->ghash_block, product, SX_BLKCIPHER_AES_BLK_SZ);
			operation->unprocessed_input_bytes = 0;
		}
	}
	safe_memzero(product, sizeof(product));
}

static psa_status_t setup(cracen_aead_operation_t *operation, enum cipher_operation dir,
			  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			  size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t tag_size;

	safe_memzero(&operation->sw_gcm_ctx, sizeof(operation->sw_gcm_ctx));

	tag_size = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, PSA_BYTES_TO_BITS(key_buffer_size), alg);
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

static psa_status_t initialize_gcm_h(cracen_aead_operation_t *operation,
				     struct sxblkcipher *cipher)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	const uint8_t zero[SX_BLKCIPHER_AES_BLK_SZ] = {};
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (gcm_ctx->ghash_initialized) {
		return PSA_SUCCESS;
	}
	status = cracen_aes_primitive(cipher, &operation->keyref, zero, gcm_ctx->h);
	gcm_ctx->ghash_initialized = status == PSA_SUCCESS;
	return status;
}

static void generate_gcm_j0(cracen_aead_operation_t *operation, uint8_t *block_j0)
{
	safe_memzero(block_j0, SX_BLKCIPHER_AES_BLK_SZ);
	/* Currently only 96 bit IV/nonce is supported */
	if (operation->nonce_length == GCM_VALID_NONCE_LEN) {
		/* J0 = IV || 0^31 || 1 */
		memcpy(block_j0, operation->nonce, operation->nonce_length);
		block_j0[SX_BLKCIPHER_AES_BLK_SZ - 1] = 1;
	}
}

psa_status_t cracen_sw_aes_gcm_encrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_ENCRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_aes_gcm_decrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_DECRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_aes_gcm_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
					 size_t nonce_length)
{
	if (!is_nonce_length_valid(nonce_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->nonce, nonce, nonce_length);
	operation->nonce_length = nonce_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_gcm_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
					   size_t plaintext_length)
{
	operation->ad_length = ad_length;
	operation->plaintext_length = plaintext_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_gcm_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
					 size_t input_length)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t processed = 0;

	status = initialize_gcm_h(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	while (processed < input_length) {
		size_t chunk_size =
			MIN(input_length - processed,
			    SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes);

		calc_gcm_ghash(operation, &input[processed], chunk_size);
		processed += chunk_size;
	}
	gcm_ctx->total_ad_fed += processed;
	return status;
}

static void finalize_ad_padding(cracen_aead_operation_t *operation)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	const uint8_t padding_block[SX_BLKCIPHER_AES_BLK_SZ] = {0};

	/* GCM requires AD to be padded to block boundary before processing plaintext */
	if (operation->unprocessed_input_bytes != 0) {
		/* Apply zero padding */
		calc_gcm_ghash(operation, padding_block,
			       SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes);
		gcm_ctx->total_ad_fed = ROUND_UP(gcm_ctx->total_ad_fed, SX_BLKCIPHER_AES_BLK_SZ);
	}
}

static void initialize_ctr(cracen_aead_operation_t *operation)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;

	if (gcm_ctx->ctr_initialized) {
		return;
	}
	generate_gcm_j0(operation, gcm_ctx->ctr_block);
	gcm_ctx->keystream_offset = SX_BLKCIPHER_AES_BLK_SZ;
	gcm_ctx->ctr_initialized = true;
}

/* XOR data with CTR mode keystream, managing keystream generation and counter */
static psa_status_t ctr_xor(cracen_aead_operation_t *operation, struct sxblkcipher *cipher,
			    const uint8_t *input, uint8_t *output, size_t length,
			    size_t counter_size)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	for (size_t i = 0; i < length; i++) {
		/* Generate new keystream block when current one is exhausted */
		if (gcm_ctx->keystream_offset >= SX_BLKCIPHER_AES_BLK_SZ) {
			status = increment_counter(gcm_ctx->ctr_block, counter_size);
			if (status != PSA_SUCCESS) {
				return status;
			}
			status = cracen_aes_primitive(cipher, &operation->keyref,
						      gcm_ctx->ctr_block, gcm_ctx->keystream);
			if (status != PSA_SUCCESS) {
				return status;
			}
			gcm_ctx->keystream_offset = 0;
		}
		output[i] = input[i] ^ gcm_ctx->keystream[gcm_ctx->keystream_offset++];
	}
	return PSA_SUCCESS;
}

/* Finalize any partial data block with zero-padding and update ghash */
static void finalize_data_padding(cracen_aead_operation_t *operation)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	uint8_t padding_block[SX_BLKCIPHER_AES_BLK_SZ] = {0};

	gcm_ctx->total_data_enc += operation->unprocessed_input_bytes;

	/* GHASH( [len(AAD)]64 || [LEN(C)]64 ) */
	encode_big_endian_length(padding_block,
				 SX_BLKCIPHER_AES_BLK_SZ / 2,
				 PSA_BYTES_TO_BITS(gcm_ctx->total_ad_fed),
				 SX_BLKCIPHER_AES_BLK_SZ / 2);
	encode_big_endian_length(padding_block + SX_BLKCIPHER_AES_BLK_SZ / 2,
				 SX_BLKCIPHER_AES_BLK_SZ / 2,
				 PSA_BYTES_TO_BITS(gcm_ctx->total_data_enc),
				 SX_BLKCIPHER_AES_BLK_SZ / 2);
	calc_gcm_ghash(operation, padding_block, SX_BLKCIPHER_AES_BLK_SZ);

	operation->unprocessed_input_bytes = 0;
	safe_memzero(padding_block, sizeof(padding_block));
}

/* Generate authentication tag by encrypting J0 block (counter=0) */
static psa_status_t generate_tag(cracen_aead_operation_t *operation, uint8_t *tag,
				 struct sxblkcipher *cipher)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	uint8_t pre_counter_block[SX_BLKCIPHER_AES_BLK_SZ];
	uint8_t s0[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	generate_gcm_j0(operation, pre_counter_block);
	status = cracen_aes_primitive(cipher, &operation->keyref,
				      pre_counter_block, s0);
	if (status != PSA_SUCCESS) {
		safe_memzero(s0, sizeof(s0));
		return status;
	}

	cracen_xorbytes(gcm_ctx->ghash_block, s0, SX_BLKCIPHER_AES_BLK_SZ);
	memcpy(tag, operation->sw_gcm_ctx.ghash_block, operation->tag_size);
	safe_memzero(s0, sizeof(s0));
	return status;
}

psa_status_t cracen_sw_aes_gcm_update(cracen_aead_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t processed = 0;
	size_t counter_size = GCM_Q_LEN_FROM_NONCE(operation->nonce_length);

	operation->ad_finished = true;

	status = initialize_gcm_h(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	safe_memzero(output, input_length);

	/* Process data with CTR mode encryption/decryption */
	if (operation->dir == CRACEN_ENCRYPT) {
		/* Encrypt: apply CTR keystream. then GHASH */
		while (processed < input_length) {
			size_t chunk_size =
				MIN(input_length - processed,
				    SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes);

			status = ctr_xor(operation, &cipher, &input[processed], &output[processed],
					 chunk_size, counter_size);
			if (status != PSA_SUCCESS) {
				return status;
			}
			calc_gcm_ghash(operation, &output[processed], chunk_size);
			processed += chunk_size;
		}
	} else {
		/** Decryption does the reverse of encryption, so apply GHASH first,
		 *  then CTR keystream
		 */
		while (processed < input_length) {
			size_t chunk_size =
				MIN(input_length - processed,
				    SX_BLKCIPHER_AES_BLK_SZ - operation->unprocessed_input_bytes);

			calc_gcm_ghash(operation, &input[processed], chunk_size);
			status = ctr_xor(operation, &cipher, &input[processed], &output[processed],
					 chunk_size, counter_size);
			if (status != PSA_SUCCESS) {
				return status;
			}
			processed += chunk_size;
		}
	}
	*output_length = processed;
	gcm_ctx->total_data_enc += processed;
	return status;
}

psa_status_t cracen_sw_aes_gcm_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				      size_t ciphertext_size, size_t *ciphertext_length,
				      uint8_t *tag, size_t tag_size, size_t *tag_length)
{
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	status = initialize_gcm_h(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	finalize_data_padding(operation);

	status = generate_tag(operation, tag, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	*tag_length = operation->tag_size;
	return status;
}

psa_status_t cracen_sw_aes_gcm_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				      size_t plaintext_size, size_t *plaintext_length,
				      const uint8_t *tag, size_t tag_length)
{
	struct sxblkcipher cipher;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t computed_tag[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	uint32_t tag_mismatch = 0;

	status = initialize_gcm_h(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	finalize_ad_padding(operation);
	finalize_data_padding(operation);

	status = generate_tag(operation, computed_tag, &cipher);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* Constant-time tag comparison to prevent timing attacks */
	for (size_t i = 0; i < operation->tag_size; i++) {
		tag_mismatch |= computed_tag[i] ^ tag[i];
	}
	if (tag_mismatch != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
	}

exit:
	safe_memzero(computed_tag, sizeof(computed_tag));
	return status;
}

psa_status_t cracen_sw_aes_gcm_abort(cracen_aead_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}
