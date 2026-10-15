/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/keyref.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/internal.h>
#include <cracen/statuscodes.h>
#include <zephyr/logging/log.h>
#include <cracen/common.h>
#include <cracen_sw_common.h>
#include <nrf_security_mem_helpers.h>

LOG_MODULE_DECLARE(cracen, CONFIG_CRACEN_LOG_LEVEL);

/* One AES job on a reserved CRACEN, ECB or, when iv is set, CBC encryption */
static psa_status_t aes_job(struct sxblkcipher *blkciph, const struct sxkeyref *key,
			    bool decrypt, const uint8_t *iv, const uint8_t *input, uint8_t *output,
			    size_t length)
{
	int sx_status;

	if (iv != NULL) {
		sx_status = sx_blkcipher_create_aescbc_enc(blkciph, key, iv);
	} else if (decrypt) {
		sx_status = sx_blkcipher_create_aesecb_dec(blkciph, key);
	} else {
		sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	}
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_crypt(blkciph, input, length, output);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	sx_status = sx_blkcipher_run(blkciph);
	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

	return silex_statuscodes_to_psa(sx_blkcipher_wait(blkciph));
}

/* Single-shot AES-ECB of whole blocks, reserving CRACEN for the call */
static psa_status_t ecb_crypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
			      bool decrypt, const uint8_t *input, size_t input_length,
			      uint8_t *output, size_t output_size, size_t *output_length)
{
	psa_status_t status;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = silex_statuscodes_to_psa(sx_hw_reserve(&blkciph->dma, SX_HW_RESERVE_CM_ENABLED));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = aes_job(blkciph, key, decrypt, NULL, input, output, input_length);
	sx_hw_release(&blkciph->dma);
	if (status != PSA_SUCCESS) {
		return status;
	}

	*output_length = input_length;
	return PSA_SUCCESS;
}

psa_status_t cracen_sw_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	return ecb_crypt(blkciph, key, false, input, input_length, output, output_size,
			 output_length);
}

psa_status_t cracen_sw_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length)
{
	return ecb_crypt(blkciph, key, true, input, input_length, output, output_size,
			 output_length);
}

psa_status_t cracen_sw_aes_primitive(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				     const uint8_t *input, uint8_t *output)
{
	return aes_job(blkciph, key, false, NULL, input, output, SX_BLKCIPHER_AES_BLK_SZ);
}

/* output = input XOR keystream, a word at a time when all are aligned */
static void xor_keystream(uint8_t *output, const uint8_t *input, const uint8_t *keystream,
			  size_t length)
{
	size_t i = 0;

	if ((((uintptr_t)output | (uintptr_t)input | (uintptr_t)keystream) & 3U) == 0U) {
		uint32_t *out32 = (uint32_t *)output;
		const uint32_t *in32 = (const uint32_t *)input;
		const uint32_t *ks32 = (const uint32_t *)keystream;

		for (; length - i >= sizeof(uint32_t); i += sizeof(uint32_t)) {
			*out32++ = *in32++ ^ *ks32++;
		}
	}
	for (; i < length; i++) {
		output[i] = input[i] ^ keystream[i];
	}
}

/* Keystream for num_blocks counter blocks with one job, encrypted in place in
 * keystream. Each block is fetched before its result is pushed, so the output
 * never overtakes the input.
 */
static psa_status_t ctr_keystream(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				  uint8_t *counter, size_t counter_start_pos, uint8_t *keystream,
				  size_t num_blocks)
{
	for (size_t i = 0; i < num_blocks; i++) {
		memcpy(&keystream[i * SX_BLKCIPHER_AES_BLK_SZ], counter, SX_BLKCIPHER_AES_BLK_SZ);
		/* Wraps modulo the counter field, as GCM's inc32 and plain CTR do */
		(void)cracen_sw_increment_counter_be(counter, SX_BLKCIPHER_AES_BLK_SZ,
						     counter_start_pos);
	}

	return aes_job(blkciph, key, false, NULL, keystream, keystream,
		       num_blocks * SX_BLKCIPHER_AES_BLK_SZ);
}

psa_status_t cracen_sw_ctr_xor(struct sxblkcipher *blkciph, const struct sxkeyref *key,
			       uint8_t *counter, size_t counter_start_pos, uint8_t *keystream,
			       size_t *keystream_offset, const uint8_t *input, uint8_t *output,
			       size_t length)
{
	/* Keystream for whole blocks, in words so it is aligned for xor_keystream() */
	uint32_t workmem[CRACEN_SW_AES_BATCH_BLOCKS * SX_BLKCIPHER_AES_BLK_SZ / sizeof(uint32_t)];
	size_t used = MIN(length, SX_BLKCIPHER_AES_BLK_SZ - *keystream_offset);
	/* Not PSA_ERROR_CORRUPTION_DETECTED, as a call within the buffered keystream runs no job */
	psa_status_t status = PSA_SUCCESS;

	xor_keystream(output, input, &keystream[*keystream_offset], used);
	*keystream_offset += used;
	input += used;
	output += used;
	length -= used;

	while (length >= SX_BLKCIPHER_AES_BLK_SZ) {
		size_t num_blocks = MIN(length / SX_BLKCIPHER_AES_BLK_SZ,
					CRACEN_SW_AES_BATCH_BLOCKS);
		size_t bytes = num_blocks * SX_BLKCIPHER_AES_BLK_SZ;

		status = ctr_keystream(blkciph, key, counter, counter_start_pos,
				       (uint8_t *)workmem, num_blocks);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		xor_keystream(output, input, (uint8_t *)workmem, bytes);
		input += bytes;
		output += bytes;
		length -= bytes;
	}

	if (length > 0) {
		status = ctr_keystream(blkciph, key, counter, counter_start_pos, keystream, 1);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		xor_keystream(output, input, keystream, length);
		*keystream_offset = length;
	}

exit:
	safe_memzero(workmem, sizeof(workmem));
	return status;
}

psa_status_t cracen_sw_cbc_mac_update(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				      uint8_t *mac_state, uint8_t *partial, size_t *partial_len,
				      const uint8_t *input, size_t length, bool keep_last)
{
	/* CBC output; only its last block, the new MAC state, is kept */
	uint8_t chain[CRACEN_SW_AES_BATCH_BLOCKS * SX_BLKCIPHER_AES_BLK_SZ];
	const size_t keep = keep_last ? 1 : 0;
	/* Not PSA_ERROR_CORRUPTION_DETECTED, as a call that only buffers input runs no job */
	psa_status_t status = PSA_SUCCESS;

	/* While a whole block is available, plus one more byte with keep_last */
	while (length >= SX_BLKCIPHER_AES_BLK_SZ + keep - *partial_len) {
		const uint8_t *blocks = input;
		size_t num_blocks;

		if (*partial_len > 0 || length < SX_BLKCIPHER_AES_BLK_SZ) {
			/* Complete the buffered block and MAC it on its own */
			size_t fill = MIN(length, SX_BLKCIPHER_AES_BLK_SZ - *partial_len);

			memcpy(&partial[*partial_len], input, fill);
			*partial_len += fill;
			input += fill;
			length -= fill;
			blocks = partial;
			num_blocks = 1;
		} else {
			num_blocks = MIN((length - keep) / SX_BLKCIPHER_AES_BLK_SZ,
					 CRACEN_SW_AES_BATCH_BLOCKS);
			input += num_blocks * SX_BLKCIPHER_AES_BLK_SZ;
			length -= num_blocks * SX_BLKCIPHER_AES_BLK_SZ;
		}

		/* Single-part CBC reads the IV as part of the job, so no context is saved */
		status = aes_job(blkciph, key, false, mac_state, blocks, chain,
				 num_blocks * SX_BLKCIPHER_AES_BLK_SZ);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		memcpy(mac_state, &chain[(num_blocks - 1) * SX_BLKCIPHER_AES_BLK_SZ],
		       SX_BLKCIPHER_AES_BLK_SZ);
		if (blocks == partial) {
			*partial_len = 0;
		}
	}

	memcpy(&partial[*partial_len], input, length);
	*partial_len += length;

exit:
	safe_memzero(chain, sizeof(chain));
	return status;
}

psa_status_t cracen_sw_increment_counter_be(uint8_t *ctr_buf, size_t ctr_buf_size, size_t start_pos)
{
	for (size_t i = ctr_buf_size; i > start_pos; i--) {
		if (++ctr_buf[i - 1] != 0) {
			return PSA_SUCCESS;
		}
	}

	/* All counter bytes wrapped to zero which means it overflowed */
	return PSA_ERROR_INVALID_ARGUMENT;
}

void cracen_sw_encode_value_be(uint8_t *buffer, size_t buffer_size, size_t value,
			       size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[buffer_size - 1 - i] = value >> (i * 8);
	}
}
