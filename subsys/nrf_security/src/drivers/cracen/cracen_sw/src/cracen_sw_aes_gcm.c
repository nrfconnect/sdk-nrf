/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Spec: http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
 *
 * See also [MGV]:
 * https://csrc.nist.rip/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-revised-spec.pdf
 *
 * We use the algorithm described as Shoup's method with 4-bit tables in
 * [MGV] 4.1, pp. 12-13, to enhance speed without using too much memory.
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

/** R is the element whose leftmost eight bits are 11100001,
 *  and whose rightmost 120 bits are all zeros
 */
#define GCM_R_VALUE			0xE1

#define GCM_SHOUP_TABLE_SIZE		16

#define GCM_BSWAP16(value)		((value & 0x00FF) << 8) | \
					((value & 0xFF00) >> 8)

/*
 * Shoup's method for multiplication use this table with last4[x] = x times P^128
 * where x and last4[x] are seen as elements of GF(2^128)
 */
static const uint16_t shoup_last4[GCM_SHOUP_TABLE_SIZE] = {
	0x0000, 0x1c20, 0x3840, 0x2460,
	0x7080, 0x6ca0, 0x48c0, 0x54e0,
	0xe100, 0xfd20, 0xd940, 0xc560,
	0x9180, 0x8da0, 0xa9c0, 0xb5e0
};

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

static inline void gcm_gen_table_rightshift(uint8_t *y, const uint8_t *x)
{
	const uint8_t r_value[SX_BLKCIPHER_AES_BLK_SZ] = {GCM_R_VALUE};
	const uint8_t zero[SX_BLKCIPHER_AES_BLK_SZ] = {};
	uint8_t xor_value[SX_BLKCIPHER_AES_BLK_SZ];
	bool msb;

	/** if MSB(X) == 0
	 *	Y <- rightshift(X)
	 *  else
	 *	Y <- rightshift(X) XOR R
	 */
	msb = x[SX_BLKCIPHER_AES_BLK_SZ - 1] & 0x01;
	cracen_be_rshift(x, 1, y, SX_BLKCIPHER_AES_BLK_SZ);
	constant_select_bin(msb, r_value, zero, xor_value, SX_BLKCIPHER_AES_BLK_SZ);
	cracen_xorbytes(y, xor_value, SX_BLKCIPHER_AES_BLK_SZ);
}

/*
 * From MbedTLS:
 * Precompute small multiples of H, that is set
 *      HH[i] || HL[i] = H times i,
 * where i is seen as a field element as in [MGV], ie high-order bits
 * correspond to low powers of P. The result is stored in the same way, that
 * is the high-order bit of HH corresponds to P^0 and the low-order bit of HL
 * corresponds to P^127.
 */
static void gcm_gen_table(cracen_aead_operation_t *operation, uint8_t *h)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;

	for (size_t i = 0; i < SX_BLKCIPHER_AES_BLK_SZ; i++) {
		/* M[128] <- H */
		gcm_ctx->h_table[CRACEN_AES_GCM_HTABLE_SIZE / 2][i] = h[i];

		/* 0 corresponds to 0 in GF(2^128) */
		gcm_ctx->h_table[0][i] = 0;
	}

	/* i <- 64 */
	for (size_t i = CRACEN_AES_GCM_HTABLE_SIZE / 4; i > 0; i >>= 1) {
		/* M[i] <- M[2*i] * P */
		gcm_gen_table_rightshift(gcm_ctx->h_table[i], gcm_ctx->h_table[i*2]);
	}

	for (size_t i = 2; i < CRACEN_AES_GCM_HTABLE_SIZE; i <<= 1) {
		for (size_t j = 1; j < i; j++) {
			cracen_xorbuffers(gcm_ctx->h_table[i+j],
					  gcm_ctx->h_table[i],
					  gcm_ctx->h_table[j],
					  SX_BLKCIPHER_AES_BLK_SZ);
		}
	}

	safe_memzero(gcm_ctx->h_table, SX_BLKCIPHER_AES_BLK_SZ);
}

static void h_table_xor(const uint8_t *table_entry, const uint8_t *in, uint8_t *out)
{
	uint8_t rem;
	uint8_t tmp_prod[SX_BLKCIPHER_AES_BLK_SZ] = {};
	uint16_t shoup_last4_be;

	rem = (uint8_t) in[SX_BLKCIPHER_AES_BLK_SZ - 1] & 0xf;
	cracen_be_rshift(in, 4, tmp_prod, SX_BLKCIPHER_AES_BLK_SZ);
	shoup_last4_be = GCM_BSWAP16(shoup_last4[rem]);
	cracen_xorbytes(tmp_prod, (uint8_t *)&shoup_last4_be, sizeof(shoup_last4_be));
	cracen_xorbuffers(out, tmp_prod, table_entry, SX_BLKCIPHER_AES_BLK_SZ);
}

/** Multiplication operation on blocks in GF(2^128)
 *  using 4-bit Shoup's table
 *
 * Algorithm is described in [MGV] 4.1.
 */
static void multiply_blocks_gf(cracen_aead_operation_t *operation, const uint8_t *block_x,
			       uint8_t *block_product)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;
	uint8_t lo;
	uint8_t hi;
	uint8_t result[SX_BLKCIPHER_AES_BLK_SZ] = {};

	lo = block_x[SX_BLKCIPHER_AES_BLK_SZ - 1] & 0xf;
	hi = (block_x[SX_BLKCIPHER_AES_BLK_SZ - 1] >> 4) & 0xf;

	h_table_xor((uint8_t *) gcm_ctx->h_table[hi], gcm_ctx->h_table[lo], result);

	for (size_t i = SX_BLKCIPHER_AES_BLK_SZ - 1; i > 0; i--) {
		lo = block_x[i - 1] & 0xf;
		hi = (block_x[i - 1] >> 4) & 0xf;

		h_table_xor((uint8_t *) gcm_ctx->h_table[lo], result, result);
		h_table_xor((uint8_t *) gcm_ctx->h_table[hi], result, result);
	}

	memcpy(block_product, result, SX_BLKCIPHER_AES_BLK_SZ);
}

/** GHASH_H(X1 || X2 || ... || Xm) = Ym
 *  Note: the current implementation follows NIST SP800-38D,
 *  no Shoup's tables are used now
 */
static void calc_gcm_ghash(cracen_aead_operation_t *operation, const uint8_t *input,
			   size_t input_len)
{
	cracen_sw_gcm_context_t *gcm_ctx = &operation->sw_gcm_ctx;

	for (size_t i = 0; i < input_len; i++) {
		operation->unprocessed_input[operation->unprocessed_input_bytes++] = input[i];
		if (operation->unprocessed_input_bytes == SX_BLKCIPHER_AES_BLK_SZ) {
			/** The size of the input data chunk of GHASH algorithm
			 * is expected to be multiple of block size (NIST SP800-38D)
			 */
			cracen_xorbytes(gcm_ctx->ghash_block, operation->unprocessed_input,
					SX_BLKCIPHER_AES_BLK_SZ);
			multiply_blocks_gf(operation, gcm_ctx->ghash_block, gcm_ctx->ghash_block);
			operation->unprocessed_input_bytes = 0;
		}
	}
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
	uint8_t h[SX_BLKCIPHER_AES_BLK_SZ] = {};
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (gcm_ctx->ghash_initialized) {
		return PSA_SUCCESS;
	}
	status = cracen_aes_primitive(cipher, &operation->keyref, zero, h);

	if (status == PSA_SUCCESS) {
		gcm_gen_table(operation, h);
		gcm_ctx->ghash_initialized = true;
	}
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
