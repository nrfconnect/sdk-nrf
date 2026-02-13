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
#include <cracen/common.h>
#include <cracen_sw_common.h>
#include <cracen_sw_aead.h>
#include <cracen_sw_aes_ccm.h>

#define CCM_NONCE_MIN_LEN		7u
#define CCM_NONCE_MAX_LEN		13u
/* Adata indicator bit in B0 flags byte (bit 6) */
#define CCM_B0_ADATA_BIT		0x40u
/* M' is encoded as (tag_length - 2)/2 and placed at bits [3:5] */
#define CCM_M_PRIME_SHIFT		3u
/* Compute Q (length field size) from nonce length: Q = 15 - nonce_len */
#define CCM_Q_LEN_FROM_NONCE(nonce_len) (SX_BLKCIPHER_AES_BLK_SZ - 1 - (nonce_len))
/* AAD length encoding boundaries and markers per RFC3610 */
#define CCM_AAD_SHORT_MAX		0xFF00u
#define CCM_AAD_MARKER0			0xFFu
#define CCM_AAD_MARKER1			0xFEu

static bool is_nonce_length_valid(size_t nonce_length)
{
	return nonce_length >= CCM_NONCE_MIN_LEN && nonce_length <= CCM_NONCE_MAX_LEN;
}

static psa_status_t cbc_mac_update_block(struct sxblkcipher *cipher, const struct sxkeyref *keyref,
					 uint8_t *mac_state, const uint8_t *block)
{
	uint8_t mac_input[SX_BLKCIPHER_AES_BLK_SZ];
	psa_status_t status;

	memcpy(mac_input, mac_state, SX_BLKCIPHER_AES_BLK_SZ);
	cracen_xorbytes(mac_input, block, SX_BLKCIPHER_AES_BLK_SZ);
	status = cracen_sw_aes_primitive(cipher, keyref, mac_input, mac_state);

	safe_memzero(mac_input, sizeof(mac_input));
	return status;
}

/* Format CCM B0 block: Flags | Nonce | Message_Length (RFC 3610) */
static void format_ccm_b0(uint8_t *b0, size_t nonce_length, const uint8_t *nonce,
			  size_t plaintext_length, size_t ad_length, size_t tag_length)
{
	size_t length_field_size = CCM_Q_LEN_FROM_NONCE(nonce_length);
	uint8_t flags = 0;

	/* Flags: [Reserved | Adata | M' | L-1] */
	if (ad_length > 0) {
		flags |= CCM_B0_ADATA_BIT;
	}
	flags |= ((tag_length - 2) / 2) << CCM_M_PRIME_SHIFT;
	flags |= length_field_size - 1;

	memset(b0, 0, SX_BLKCIPHER_AES_BLK_SZ);
	b0[0] = flags;
	memcpy(&b0[1], nonce, nonce_length);
	cracen_sw_encode_value_be(b0, SX_BLKCIPHER_AES_BLK_SZ, plaintext_length, length_field_size);
}

/* Format CCM counter block: Flags | Nonce | Counter (RFC 3610) */
static void format_ccm_ctr_block(uint8_t *ctr, size_t nonce_length, const uint8_t *nonce,
				 uint32_t counter)
{
	size_t length_field_size = CCM_Q_LEN_FROM_NONCE(nonce_length);

	memset(ctr, 0, SX_BLKCIPHER_AES_BLK_SZ);
	ctr[0] = length_field_size - 1;
	memcpy(&ctr[1], nonce, nonce_length);
	cracen_sw_encode_value_be(ctr, SX_BLKCIPHER_AES_BLK_SZ, counter, length_field_size);
}

/* Encode AAD length per RFC 3610: 0 bytes (none), 2 bytes (<65280), or 6 bytes (>=65280) */
static size_t encode_ccm_ad_length(uint8_t *output, size_t ad_length)
{
	if (ad_length == 0) {
		return 0;
	}

	if (ad_length < CCM_AAD_SHORT_MAX) {
		cracen_sw_encode_value_be(output, 2, ad_length, 2);
		return 2;
	}

	output[0] = CCM_AAD_MARKER0;
	output[1] = CCM_AAD_MARKER1;
	cracen_sw_encode_value_be(output + 2, 4, ad_length, 4);
	return 6;
}

static psa_status_t setup(cracen_aead_operation_t *operation, enum cipher_operation dir,
			  const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			  size_t key_buffer_size, psa_algorithm_t alg)
{
	psa_status_t status;
	size_t tag_size;

	memset(&operation->sw_ccm_ctx, 0, sizeof(operation->sw_ccm_ctx));

	/* Validate tag size: CCM requires even tag sizes from 4 to 16 bytes (RFC 3610) */
	tag_size = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, PSA_BYTES_TO_BITS(key_buffer_size), alg);
	if ((tag_size & 1) || (tag_size < 4) || (tag_size > 16)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	memcpy(operation->key_buffer, key_buffer, key_buffer_size);
	status = cracen_load_keyref(attributes, operation->key_buffer, key_buffer_size,
				    &operation->keyref);
	if (status != PSA_SUCCESS) {
		return status;
	}
	operation->alg = PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg);
	operation->dir = dir;
	operation->tag_size = tag_size;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_encrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_ENCRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_aes_ccm_decrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg)
{
	return setup(operation, CRACEN_DECRYPT, attributes, key_buffer, key_buffer_size, alg);
}

psa_status_t cracen_sw_aes_ccm_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
					 size_t nonce_length)
{
	if (!is_nonce_length_valid(nonce_length)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	memcpy(operation->nonce, nonce, nonce_length);
	operation->nonce_length = nonce_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
					   size_t plaintext_length)
{
	operation->ad_length = ad_length;
	operation->plaintext_length = plaintext_length;
	return PSA_SUCCESS;
}

static psa_status_t initialize_cbc_mac(cracen_aead_operation_t *operation,
				       struct sxblkcipher *cipher)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	uint8_t b0[SX_BLKCIPHER_AES_BLK_SZ];
	psa_status_t status;

	if (ccm_ctx->cbc_mac_initialized) {
		return PSA_SUCCESS;
	}
	format_ccm_b0(b0, operation->nonce_length, operation->nonce, operation->plaintext_length,
		      operation->ad_length, operation->tag_size);
	status = cracen_sw_aes_primitive(cipher, &operation->keyref, b0, ccm_ctx->cbc_mac);
	if (status != PSA_SUCCESS) {
		return status;
	}
	if (operation->ad_length > 0) {
		size_t ad_len_size =
			encode_ccm_ad_length(ccm_ctx->partial_block, operation->ad_length);
		ccm_ctx->has_partial_ad_block = true;
		ccm_ctx->total_ad_fed = ad_len_size;
	}
	ccm_ctx->cbc_mac_initialized = true;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
					 size_t input_length)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	struct sxblkcipher cipher;
	psa_status_t status;
	uint8_t ad_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t processed = 0;

	status = initialize_cbc_mac(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	/* Complete any partial AD block from previous call */
	if (ccm_ctx->has_partial_ad_block) {
		size_t position_in_block = ccm_ctx->total_ad_fed % SX_BLKCIPHER_AES_BLK_SZ;
		size_t space_in_block = SX_BLKCIPHER_AES_BLK_SZ - position_in_block;
		size_t to_copy = MIN(input_length, space_in_block);

		memcpy(&ccm_ctx->partial_block[position_in_block], input, to_copy);
		processed += to_copy;
		ccm_ctx->total_ad_fed += to_copy;
		if (ccm_ctx->total_ad_fed % SX_BLKCIPHER_AES_BLK_SZ == 0) {
			status = cbc_mac_update_block(&cipher, &operation->keyref, ccm_ctx->cbc_mac,
						      ccm_ctx->partial_block);
			if (status != PSA_SUCCESS) {
				return status;
			}
			ccm_ctx->has_partial_ad_block = false;
			ccm_ctx->total_ad_fed =
				ROUND_UP(ccm_ctx->total_ad_fed, SX_BLKCIPHER_AES_BLK_SZ);
		}
	}
	/* Process complete blocks */
	while (processed + SX_BLKCIPHER_AES_BLK_SZ <= input_length) {
		memcpy(ad_block, &input[processed], SX_BLKCIPHER_AES_BLK_SZ);
		status = cbc_mac_update_block(&cipher, &operation->keyref, ccm_ctx->cbc_mac,
					      ad_block);
		if (status != PSA_SUCCESS) {
			return status;
		}
		processed += SX_BLKCIPHER_AES_BLK_SZ;
		ccm_ctx->total_ad_fed += SX_BLKCIPHER_AES_BLK_SZ;
	}
	/* Handle remaining bytes that don't fill a complete block */
	if (processed < input_length) {
		size_t remaining = input_length - processed;

		memset(ad_block, 0, SX_BLKCIPHER_AES_BLK_SZ);
		memcpy(ad_block, &input[processed], remaining);
		/* If this completes to a block boundary, process immediately */
		if ((ccm_ctx->total_ad_fed + remaining) % SX_BLKCIPHER_AES_BLK_SZ == 0) {
			status = cbc_mac_update_block(&cipher, &operation->keyref, ccm_ctx->cbc_mac,
						      ad_block);
			if (status != PSA_SUCCESS) {
				return status;
			}
			ccm_ctx->has_partial_ad_block = false;
		} else {
			/* Save partial block for next call or finalization */
			memcpy(ccm_ctx->partial_block, ad_block, SX_BLKCIPHER_AES_BLK_SZ);
			ccm_ctx->has_partial_ad_block = true;
		}
		ccm_ctx->total_ad_fed += remaining;
	}
	return PSA_SUCCESS;
}

static psa_status_t finalize_ad_padding(cracen_aead_operation_t *operation,
					struct sxblkcipher *cipher)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	psa_status_t status = PSA_SUCCESS;

	/* CCM requires AD to be padded to block boundary before processing plaintext */
	if (ccm_ctx->has_partial_ad_block) {
		/* Process partial block (already zero-padded) */
		status = cbc_mac_update_block(cipher, &operation->keyref, ccm_ctx->cbc_mac,
					      ccm_ctx->partial_block);
		if (status != PSA_SUCCESS) {
			return status;
		}
		ccm_ctx->has_partial_ad_block = false;
		ccm_ctx->total_ad_fed = ROUND_UP(ccm_ctx->total_ad_fed, SX_BLKCIPHER_AES_BLK_SZ);
	}

	return status;
}

static void initialize_ctr(cracen_aead_operation_t *operation)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;

	if (ccm_ctx->ctr_initialized) {
		return;
	}
	format_ccm_ctr_block(ccm_ctx->ctr_block, operation->nonce_length, operation->nonce, 0);
	ccm_ctx->keystream_offset = SX_BLKCIPHER_AES_BLK_SZ;
	ccm_ctx->ctr_initialized = true;
}

/* Finalize any partial data block with zero-padding and update CBC-MAC */
static psa_status_t finalize_data_padding(cracen_aead_operation_t *operation,
					  struct sxblkcipher *cipher)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	uint8_t padding_block[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	psa_status_t status = PSA_SUCCESS;

	if (ccm_ctx->data_partial_len > 0) {
		memcpy(padding_block, ccm_ctx->partial_block, ccm_ctx->data_partial_len);
		status = cbc_mac_update_block(cipher, &operation->keyref, ccm_ctx->cbc_mac,
					      padding_block);
		ccm_ctx->data_partial_len = 0;
		safe_memzero(padding_block, sizeof(padding_block));
	}
	return status;
}

/* Generate authentication tag by encrypting CBC-MAC with A0 block (counter=0) */
static psa_status_t generate_tag(cracen_aead_operation_t *operation, uint8_t *tag,
				 struct sxblkcipher *cipher)
{
	uint8_t a0_block[SX_BLKCIPHER_AES_BLK_SZ];
	uint8_t s0[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	psa_status_t status;

	format_ccm_ctr_block(a0_block, operation->nonce_length, operation->nonce, 0);
	status = cracen_sw_aes_primitive(cipher, &operation->keyref, a0_block, s0);
	if (status != PSA_SUCCESS) {
		safe_memzero(s0, sizeof(s0));
		return status;
	}
	memcpy(tag, operation->sw_ccm_ctx.cbc_mac, operation->tag_size);
	cracen_xorbytes(tag, s0, operation->tag_size);
	safe_memzero(s0, sizeof(s0));
	return PSA_SUCCESS;
}

/* Accumulate plaintext bytes for CBC-MAC, processing complete blocks */
static psa_status_t accumulate_for_mac(cracen_aead_operation_t *operation, const uint8_t *data,
				       size_t length, struct sxblkcipher *cipher)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	psa_status_t status;

	for (size_t i = 0; i < length; i++) {
		ccm_ctx->partial_block[ccm_ctx->data_partial_len++] = data[i];
		if (ccm_ctx->data_partial_len == SX_BLKCIPHER_AES_BLK_SZ) {
			status = cbc_mac_update_block(cipher, &operation->keyref, ccm_ctx->cbc_mac,
						      ccm_ctx->partial_block);
			if (status != PSA_SUCCESS) {
				return status;
			}
			ccm_ctx->data_partial_len = 0;
		}
	}
	return PSA_SUCCESS;
}

/* XOR data with CTR mode keystream, managing keystream generation and counter */
static psa_status_t ctr_xor(cracen_aead_operation_t *operation, struct sxblkcipher *cipher,
			    const uint8_t *input, uint8_t *output, size_t length,
			    size_t counter_size)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	psa_status_t status;
	size_t counter_start_pos = SX_BLKCIPHER_AES_BLK_SZ - counter_size;

	for (size_t i = 0; i < length; i++) {
		/* Generate new keystream block when current one is exhausted */
		if (ccm_ctx->keystream_offset >= SX_BLKCIPHER_AES_BLK_SZ) {
			status = cracen_sw_increment_counter_be(ccm_ctx->ctr_block,
								SX_BLKCIPHER_AES_BLK_SZ,
								counter_start_pos);
			if (status != PSA_SUCCESS) {
				return status;
			}
			status = cracen_sw_aes_primitive(cipher, &operation->keyref,
							 ccm_ctx->ctr_block, ccm_ctx->keystream);
			if (status != PSA_SUCCESS) {
				return status;
			}
			ccm_ctx->keystream_offset = 0;
		}
		output[i] = input[i] ^ ccm_ctx->keystream[ccm_ctx->keystream_offset++];
	}
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_update(cracen_aead_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length)
{
	cracen_sw_ccm_context_t *ccm_ctx = &operation->sw_ccm_ctx;
	struct sxblkcipher cipher;
	psa_status_t status;
	size_t processed = 0;
	size_t counter_size = CCM_Q_LEN_FROM_NONCE(operation->nonce_length);

	operation->ad_finished = true;
	status = initialize_cbc_mac(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	status = finalize_ad_padding(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	initialize_ctr(operation);

	/* Process data with CTR mode encryption/decryption and CBC-MAC authentication */
	if (operation->dir == CRACEN_ENCRYPT) {
		/* Encrypt: MAC plaintext, then apply CTR keystream */
		while (processed < input_length) {
			size_t chunk_size =
				MIN(input_length - processed,
				    SX_BLKCIPHER_AES_BLK_SZ - ccm_ctx->data_partial_len);

			status = accumulate_for_mac(operation, &input[processed], chunk_size,
						    &cipher);
			if (status != PSA_SUCCESS) {
				return status;
			}

			status = ctr_xor(operation, &cipher, &input[processed], &output[processed],
					 chunk_size, counter_size);
			if (status != PSA_SUCCESS) {
				return status;
			}
			processed += chunk_size;
		}
	} else {
		/*
		 * Decryption does the reverse of encryption, so apply CTR keystream first,
		 * then MAC plaintext
		 */
		while (processed < input_length) {
			size_t chunk_size =
				MIN(input_length - processed,
				    SX_BLKCIPHER_AES_BLK_SZ - ccm_ctx->data_partial_len);

			status = ctr_xor(operation, &cipher, &input[processed], &output[processed],
					 chunk_size, counter_size);
			if (status != PSA_SUCCESS) {
				return status;
			}

			status = accumulate_for_mac(operation, &output[processed], chunk_size,
						    &cipher);
			if (status != PSA_SUCCESS) {
				return status;
			}
			processed += chunk_size;
		}
	}
	*output_length = processed;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				      size_t ciphertext_size, size_t *ciphertext_length,
				      uint8_t *tag, size_t tag_size, size_t *tag_length)
{
	struct sxblkcipher cipher;
	psa_status_t status;

	status = initialize_cbc_mac(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = finalize_ad_padding(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = finalize_data_padding(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = generate_tag(operation, tag, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}
	*tag_length = operation->tag_size;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				      size_t plaintext_size, size_t *plaintext_length,
				      const uint8_t *tag, size_t tag_length)
{
	struct sxblkcipher cipher;
	psa_status_t status;
	uint8_t computed_tag[SX_BLKCIPHER_AES_BLK_SZ] = {0};
	uint32_t tag_mismatch = 0;

	status = initialize_cbc_mac(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = finalize_ad_padding(operation, &cipher);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = finalize_data_padding(operation, &cipher);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

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

psa_status_t cracen_sw_aes_ccm_abort(cracen_aead_operation_t *operation)
{
	safe_memzero(operation, sizeof(*operation));
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ccm_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *nonce,
				       size_t nonce_length, const uint8_t *additional_data,
				       size_t additional_data_length, const uint8_t *plaintext,
				       size_t plaintext_length, uint8_t *ciphertext,
				       size_t ciphertext_size, size_t *ciphertext_length)
{
	psa_status_t status;
	cracen_aead_operation_t operation = {0};
	size_t update_length = 0;
	size_t finish_ciphertext_length = 0;
	size_t tag_length = 0;

	*ciphertext_length = 0;
	if (ciphertext_size <
	    plaintext_length + PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES,
						   PSA_BYTES_TO_BITS(key_buffer_size), alg)) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}
	status = cracen_sw_aes_ccm_encrypt_setup(&operation, attributes, key_buffer,
						 key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	cracen_sw_aes_ccm_set_lengths(&operation, additional_data_length, plaintext_length);
	status = cracen_sw_aes_ccm_set_nonce(&operation, nonce, nonce_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	if (additional_data_length > 0) {
		status = cracen_sw_aes_ccm_update_ad(&operation, additional_data,
						     additional_data_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}
	if (plaintext_length > 0) {
		status = cracen_sw_aes_ccm_update(&operation, plaintext, plaintext_length,
						  ciphertext, ciphertext_size, &update_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}
	/* CCM finish produces no additional ciphertext, only the tag */
	status = cracen_sw_aes_ccm_finish(&operation, NULL, 0, &finish_ciphertext_length,
					  ciphertext + update_length,
					  ciphertext_size - update_length, &tag_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	*ciphertext_length = update_length + tag_length;

exit:
	if (status != PSA_SUCCESS && ciphertext != NULL && ciphertext_size > 0) {
		safe_memzero(ciphertext, ciphertext_size);
	}
	cracen_sw_aes_ccm_abort(&operation);
	return status;
}

psa_status_t cracen_sw_aes_ccm_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *nonce,
				       size_t nonce_length, const uint8_t *additional_data,
				       size_t additional_data_length, const uint8_t *ciphertext,
				       size_t ciphertext_length, uint8_t *plaintext,
				       size_t plaintext_size, size_t *plaintext_length)
{
	psa_status_t status;
	cracen_aead_operation_t operation = {0};
	size_t update_length = 0;
	size_t tag_size;
	const uint8_t *tag;
	uint8_t tag_buffer[SX_BLKCIPHER_AES_BLK_SZ];

	tag_size = PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, PSA_BYTES_TO_BITS(key_buffer_size), alg);
	if (ciphertext_length < tag_size) {
		*plaintext_length = 0;
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	*plaintext_length = ciphertext_length - tag_size;
	if (plaintext_size < *plaintext_length) {
		*plaintext_length = 0;
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Copy tag to local buffer to avoid issues if ciphertext and plaintext overlap */
	tag = ciphertext + *plaintext_length;
	memcpy(tag_buffer, tag, tag_size);
	tag = tag_buffer;
	status = cracen_sw_aes_ccm_decrypt_setup(&operation, attributes, key_buffer,
						 key_buffer_size, alg);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	cracen_sw_aes_ccm_set_lengths(&operation, additional_data_length, *plaintext_length);
	status = cracen_sw_aes_ccm_set_nonce(&operation, nonce, nonce_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	if (additional_data_length > 0) {
		status = cracen_sw_aes_ccm_update_ad(&operation, additional_data,
						     additional_data_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}
	if (*plaintext_length > 0) {
		status = cracen_sw_aes_ccm_update(&operation, ciphertext, *plaintext_length,
						  plaintext, plaintext_size, &update_length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}
	status = cracen_sw_aes_ccm_verify(&operation, NULL, 0, &update_length, tag, tag_size);
	if (status != PSA_SUCCESS) {
		safe_memzero(plaintext, *plaintext_length);
		*plaintext_length = 0;
		goto exit;
	}
exit:
	safe_memzero(tag_buffer, sizeof(tag_buffer));
	cracen_sw_aes_ccm_abort(&operation);
	return status;
}
