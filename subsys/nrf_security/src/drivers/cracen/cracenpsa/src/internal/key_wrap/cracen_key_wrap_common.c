/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_wrap/cracen_key_wrap_common.h>
#include <internal/aes/cracen_aes_ecb.h>

#include <stddef.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <silexpk/core.h>

/**
 * @brief Compute buf = buf ^ xor_val.
 *
 * @param buf		Big-endian value.
 * @param buf_sz	size of buf.
 * @param xor_val	A value to XOR buf with.
 */
static void cracen_be_xor(uint8_t *buf, size_t buf_sz, size_t xor_val)
{
	while (buf_sz > 0 && xor_val > 0) {
		buf_sz--;
		buf[buf_sz] ^= xor_val & 0xFF;
		xor_val >>= 8;
	}
}

psa_status_t cracen_key_wrap(const struct sxkeyref *keyref, uint8_t *block_array,
			     size_t plaintext_blocks_count, uint8_t *integrity_check_reg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	struct sxblkcipher cipher;
	size_t length;
	uint8_t intermediate_block[SX_BLKCIPHER_AES_BLK_SZ];
	uint8_t cipher_input_data[SX_BLKCIPHER_AES_BLK_SZ];
	size_t enc_step;

	for (size_t j = 0; j <= CRACEN_KEY_WRAP_ITERATIONS_COUNT; j++) {
		for (size_t i = 0; i < plaintext_blocks_count; i++) {
			/* A | R[i] */
			memcpy(cipher_input_data,
			       integrity_check_reg,
			       CRACEN_KEY_WRAP_BLOCK_SIZE);
			memcpy(cipher_input_data + CRACEN_KEY_WRAP_BLOCK_SIZE,
			       &block_array[i * CRACEN_KEY_WRAP_BLOCK_SIZE],
			       CRACEN_KEY_WRAP_BLOCK_SIZE);

			/* B = AES(K, A | R[i]) */
			status = cracen_aes_ecb_encrypt(&cipher, keyref,
						      cipher_input_data, SX_BLKCIPHER_AES_BLK_SZ,
						      intermediate_block, SX_BLKCIPHER_AES_BLK_SZ,
						      &length);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/* R[i] = LSB(64, B) */
			memcpy(&block_array[i * CRACEN_KEY_WRAP_BLOCK_SIZE],
			       intermediate_block + CRACEN_KEY_WRAP_BLOCK_SIZE,
			       CRACEN_KEY_WRAP_BLOCK_SIZE);

			/** A = MSB(64, B) ^ t where t = (n*j)+i
			 *
			 *  According to RFC3394, initial value of i is 1, so
			 *  incrementing it here.
			 */
			enc_step = (plaintext_blocks_count * j) + i + 1;
			cracen_be_xor(intermediate_block, CRACEN_KEY_WRAP_BLOCK_SIZE, enc_step);
			memcpy(integrity_check_reg, intermediate_block, CRACEN_KEY_WRAP_BLOCK_SIZE);
		}
	}

exit:
	safe_memzero(intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
	safe_memzero(cipher_input_data, SX_BLKCIPHER_AES_BLK_SZ);
	return status;
}

psa_status_t cracen_key_unwrap(const struct sxkeyref *keyref, uint8_t *cipher_input_block,
			       uint8_t *block_array, size_t ciphertext_blocks_count)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	struct sxblkcipher cipher;
	size_t length;
	uint8_t intermediate_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t enc_step;

	for (size_t j = CRACEN_KEY_WRAP_ITERATIONS_COUNT + 1; j > 0; j--) {
		for (size_t i = ciphertext_blocks_count; i > 0; i--) {
			/* cipher_input_block = (A ^ t) | R[i] */
			enc_step = (ciphertext_blocks_count * (j - 1)) + i;
			cracen_be_xor(cipher_input_block, CRACEN_KEY_WRAP_BLOCK_SIZE, enc_step);
			memcpy(cipher_input_block + CRACEN_KEY_WRAP_BLOCK_SIZE,
			       &block_array[(i - 1) * CRACEN_KEY_WRAP_BLOCK_SIZE],
			       CRACEN_KEY_WRAP_BLOCK_SIZE);

			/* B = AES-1(K, cipher_input_block) where t = n*j+i */
			status = cracen_aes_ecb_decrypt(&cipher, keyref,
						      cipher_input_block, SX_BLKCIPHER_AES_BLK_SZ,
						      intermediate_block, SX_BLKCIPHER_AES_BLK_SZ,
						      &length);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/* A = MSB(64, B) */
			memcpy(cipher_input_block, intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
			/* R[i] = LSB(64, B) */
			memcpy(&block_array[(i - 1) * CRACEN_KEY_WRAP_BLOCK_SIZE],
			       cipher_input_block + CRACEN_KEY_WRAP_BLOCK_SIZE,
			       CRACEN_KEY_WRAP_BLOCK_SIZE);
		}
	}

exit:
	safe_memzero(intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
	return status;
}
