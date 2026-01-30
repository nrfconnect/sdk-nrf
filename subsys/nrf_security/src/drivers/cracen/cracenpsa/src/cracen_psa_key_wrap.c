/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <cracen_psa_key_wrap.h>
#include <cracen_psa_primitives.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include <string.h>
#include <stdbool.h>
#include <silexpk/core.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>

/* RFC3394 and RFC5649 */
#define CRACEN_WRAP_BLOCK_SIZE			8u
#define CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE	CRACEN_WRAP_BLOCK_SIZE
#define CRACEN_WRAP_ITERATIONS_COUNT		5

/* RFC3394 */
#define CRACEN_KW_MIN_KEY_SIZE			(2 * CRACEN_WRAP_BLOCK_SIZE)

/* RFC5649 */
#define CRACEN_KWP_MIN_KEY_SIZE			1u
/* Key size for KWP is no more than 2^32 octets */
#define CRACEN_KWP_MAX_KEY_SIZE			(0xFFFFFFFF)
#define CRACEN_KWP_IV_SIZE			(CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE / 2)
#define CRACEN_KWP_MLI_SIZE			(CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE / 2)

/* AES-KW default initial value (RFC3394) */
static const uint8_t cracen_kw_iv[CRACEN_WRAP_BLOCK_SIZE] = {0xA6, 0xA6, 0xA6, 0xA6,
							     0xA6, 0xA6, 0xA6, 0xA6};
/* AES-KW default initial value (RFC5649) */
static const uint8_t cracen_kwp_iv[CRACEN_KWP_IV_SIZE] = {0xA6, 0x59, 0x59, 0xA6};

static psa_status_t cracen_aes_ecb_crypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
					 const uint8_t *input, size_t input_length, uint8_t *output,
					 size_t output_size, size_t *output_length, bool encrypt)
{
	int sx_status;

	if (output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if ((input_length % SX_BLKCIPHER_AES_BLK_SZ) != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	*output_length = 0;

	if (encrypt) {
		sx_status = sx_blkcipher_create_aesecb_enc(blkciph, key);
	} else {
		sx_status = sx_blkcipher_create_aesecb_dec(blkciph, key);
	}

	if (sx_status != SX_OK) {
		return silex_statuscodes_to_psa(sx_status);
	}

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

	return silex_statuscodes_to_psa(sx_status);
}

/**
 * @brief Compute buf = buf ^ xor_val.
 *
 * @param buf		Big-endian value.
 * @param buf_sz	size of buf.
 * @param xor_val	A value to XOR buf with.
 */
void cracen_be_xor(uint8_t *buf, size_t buf_sz, size_t xor_val)
{
	while (buf_sz > 0 && xor_val > 0) {
		buf_sz--;
		buf[buf_sz] ^= xor_val & 0xFF;
		xor_val >>= 8;
	}
}

static psa_status_t cracen_wrap(const struct sxkeyref *keyref, uint8_t *block_array,
				size_t plaintext_blocks_count,
				uint8_t *integrity_check_reg)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	struct sxblkcipher cipher;
	size_t length;
	uint8_t intermediate_block[SX_BLKCIPHER_AES_BLK_SZ];
	uint8_t cipher_input_data[SX_BLKCIPHER_AES_BLK_SZ];
	size_t enc_step;

	for (size_t j = 0; j <= CRACEN_WRAP_ITERATIONS_COUNT; j++) {
		for (size_t i = 0; i < plaintext_blocks_count; i++) {
			/* A | R[i] */
			memcpy(cipher_input_data,
			       integrity_check_reg,
			       CRACEN_WRAP_BLOCK_SIZE);
			memcpy(cipher_input_data + CRACEN_WRAP_BLOCK_SIZE,
			       &block_array[i * CRACEN_WRAP_BLOCK_SIZE],
			       CRACEN_WRAP_BLOCK_SIZE);

			/* B = AES(K, A | R[i]) */
			status = cracen_aes_ecb_crypt(&cipher, keyref,
						      cipher_input_data, SX_BLKCIPHER_AES_BLK_SZ,
						      intermediate_block, SX_BLKCIPHER_AES_BLK_SZ,
						      &length, true);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/* R[i] = LSB(64, B) */
			memcpy(&block_array[i * CRACEN_WRAP_BLOCK_SIZE],
			       intermediate_block + CRACEN_WRAP_BLOCK_SIZE,
			       CRACEN_WRAP_BLOCK_SIZE);

			/** A = MSB(64, B) ^ t where t = (n*j)+i
			 *
			 *  According to RFC3394, initial value of i is 1, so
			 *  incrementing it here.
			 */
			enc_step = (plaintext_blocks_count * j) + i + 1;
			cracen_be_xor(intermediate_block, CRACEN_WRAP_BLOCK_SIZE, enc_step);
			memcpy(integrity_check_reg, intermediate_block, CRACEN_WRAP_BLOCK_SIZE);
		}
	}

exit:
	safe_memzero(intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
	safe_memzero(cipher_input_data, SX_BLKCIPHER_AES_BLK_SZ);
	return status;
}

static psa_status_t cracen_unwrap(const struct sxkeyref *keyref, uint8_t *cipher_input_block,
				  uint8_t *block_array, size_t ciphertext_blocks_count)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	struct sxblkcipher cipher;
	size_t length;
	uint8_t intermediate_block[SX_BLKCIPHER_AES_BLK_SZ];
	size_t enc_step;

	for (size_t j = CRACEN_WRAP_ITERATIONS_COUNT + 1; j > 0; j--) {
		for (size_t i = ciphertext_blocks_count; i > 0; i--) {
			/* cipher_input_block = (A ^ t) | R[i] */
			enc_step = (ciphertext_blocks_count * (j - 1)) + i;
			cracen_be_xor(cipher_input_block, CRACEN_WRAP_BLOCK_SIZE, enc_step);
			memcpy(cipher_input_block + CRACEN_WRAP_BLOCK_SIZE,
			       &block_array[(i - 1) * CRACEN_WRAP_BLOCK_SIZE],
			       CRACEN_WRAP_BLOCK_SIZE);

			/* B = AES-1(K, cipher_input_block) where t = n*j+i */
			status = cracen_aes_ecb_crypt(&cipher, keyref,
						      cipher_input_block, SX_BLKCIPHER_AES_BLK_SZ,
						      intermediate_block, SX_BLKCIPHER_AES_BLK_SZ,
						      &length, false);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/* A = MSB(64, B) */
			memcpy(cipher_input_block, intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
			/* R[i] = LSB(64, B) */
			memcpy(&block_array[(i - 1) * CRACEN_WRAP_BLOCK_SIZE],
			       cipher_input_block + CRACEN_WRAP_BLOCK_SIZE,
			       CRACEN_WRAP_BLOCK_SIZE);
		}
	}

exit:
	safe_memzero(intermediate_block, SX_BLKCIPHER_AES_BLK_SZ);
	return status;
}

static psa_status_t cracen_kw_wrap(const psa_key_attributes_t *wrapping_key_attributes,
				   const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				   const uint8_t *key_data, size_t key_size,
				   uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	struct sxkeyref keyref;
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_size < CRACEN_KW_MIN_KEY_SIZE || key_size % CRACEN_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Ciphertext contains (n+1) AES KW blocks since it adds integrity check register */
	if (key_size + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* "data" contains both integrity check register and wrapped key */
	memcpy(data, cracen_kw_iv, CRACEN_WRAP_BLOCK_SIZE);
	memmove(data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE, key_data, key_size);
	blocks_count = key_size / CRACEN_WRAP_BLOCK_SIZE;

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_wrap(&keyref, data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE,
			     blocks_count, data);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	*data_length = key_size + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE;
	return status;
exit:
	safe_memzero(data, data_size);
	return status;
}

static psa_status_t cracen_kw_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
				     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				     const uint8_t *data, size_t data_size,
				     uint8_t *key_data, size_t key_data_size,
				     size_t *key_data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	struct sxkeyref keyref;
	uint8_t cipher_input_block[SX_BLKCIPHER_AES_BLK_SZ];
	int sig_cmp_res;
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (data_size < (CRACEN_KW_MIN_KEY_SIZE + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE) ||
	    data_size % CRACEN_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE > key_data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	memmove(key_data,
		data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE,
		data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE);
	blocks_count = (data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE) / CRACEN_WRAP_BLOCK_SIZE;
	memcpy(cipher_input_block, data, CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE);

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_unwrap(&keyref, cipher_input_block, key_data, blocks_count);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	sig_cmp_res = constant_memcmp(cipher_input_block, cracen_kw_iv,
				      CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE);
	safe_memzero(cipher_input_block, SX_BLKCIPHER_AES_BLK_SZ);

	if (sig_cmp_res != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	*key_data_length = data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE;
	return status;
exit:
	safe_memzero(key_data, key_data_size);
	return status;
}

/* Encodes value as a big endian */
static void encode_be_value(uint8_t *buffer, size_t buffer_size, size_t value,
			    size_t value_size)
{
	for (size_t i = 0; i < value_size; i++) {
		buffer[buffer_size - 1 - i] = value >> (i * 8);
	}
}

/* Dencodes value as a big endian */
static void decode_be_value(size_t *value, size_t value_size, const uint8_t *buffer)
{
	*value = 0;
	for (size_t i = 0; i < value_size; i++) {
		*value |= (size_t)buffer[i] << ((value_size - 1 - i) * 8);
	}
}

static psa_status_t cracen_kwp_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
				      const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				      const uint8_t *data, size_t data_size,
				      uint8_t *key_data, size_t key_data_size,
				      size_t *key_data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	struct sxkeyref keyref;
	struct sxblkcipher cipher;
	size_t length;
	size_t pad_size;
	bool sig_check_fail;
	uint8_t cipher_output_block[SX_BLKCIPHER_AES_BLK_SZ];
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (data_size < (CRACEN_WRAP_BLOCK_SIZE + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE) ||
	    data_size % CRACEN_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE > key_data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	blocks_count = (data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE) / CRACEN_WRAP_BLOCK_SIZE;

	if (blocks_count == 1) {
		status = cracen_aes_ecb_crypt(&cipher, &keyref,
					      data, data_size,
					      cipher_output_block, SX_BLKCIPHER_AES_BLK_SZ,
					      &length, false);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		/* RFC5649: A | P[1] = DEC(K, C[0] | C[1]) */
		memcpy(key_data,
		       cipher_output_block + CRACEN_WRAP_BLOCK_SIZE,
		       CRACEN_WRAP_BLOCK_SIZE);
	} else {
		memcpy(cipher_output_block, data, CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE);
		memmove(key_data,
			data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE,
			data_size - CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE);
		status = cracen_unwrap(&keyref, cipher_output_block, key_data, blocks_count);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	decode_be_value(&length, CRACEN_KWP_MLI_SIZE, cipher_output_block + CRACEN_KWP_IV_SIZE);
	pad_size = blocks_count * CRACEN_WRAP_BLOCK_SIZE - length;

	sig_check_fail = constant_memcmp(cipher_output_block,
					 cracen_kwp_iv,
					 CRACEN_KWP_IV_SIZE) != 0;
	sig_check_fail = sig_check_fail || (pad_size > (CRACEN_WRAP_BLOCK_SIZE - 1));
	sig_check_fail = sig_check_fail ||
			 (pad_size > 0 &&
			  !constant_memcmp_is_zero(key_data + length, pad_size));

	safe_memzero(cipher_output_block, SX_BLKCIPHER_AES_BLK_SZ);
	if (sig_check_fail) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	*key_data_length = length;
	return status;
exit:
	safe_memzero(key_data, key_data_size);
	return status;
}

static psa_status_t cracen_kwp_wrap(const psa_key_attributes_t *wrapping_key_attributes,
				    const uint8_t *wrapping_key_data, size_t wrapping_key_size,
				    const uint8_t *key_data, size_t key_size,
				    uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t blocks_count;
	size_t pad_size;
	struct sxkeyref keyref;
	struct sxblkcipher cipher;
	size_t length;
	size_t pad_data_size;
	psa_key_type_t key_type = psa_get_key_type(wrapping_key_attributes);

	if (key_type != PSA_KEY_TYPE_AES) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_size < CRACEN_KWP_MIN_KEY_SIZE || key_size > CRACEN_KWP_MAX_KEY_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	pad_size = CRACEN_WRAP_BLOCK_SIZE - key_size % CRACEN_WRAP_BLOCK_SIZE;
	pad_size = pad_size % CRACEN_WRAP_BLOCK_SIZE;
	if (key_size + pad_size + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE > data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* data = (IV | MLI: message length indicator | key | optional padding) */
	memcpy(data, cracen_kwp_iv, CRACEN_KWP_IV_SIZE);
	encode_be_value(data + CRACEN_KWP_IV_SIZE,
			CRACEN_KWP_MLI_SIZE, key_size, CRACEN_KWP_MLI_SIZE);
	memmove(data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE, key_data, key_size);
	safe_memzero(data + key_size + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE, pad_size);

	pad_data_size = key_size + pad_size + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE;

	/* NIST.SP.800-38F 6.3 */
	if (key_size <= CRACEN_WRAP_BLOCK_SIZE) {
		status = cracen_aes_ecb_crypt(&cipher, &keyref,
					      data, pad_data_size,
					      data, pad_data_size,
					      &length, true);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	} else {
		blocks_count = (key_size + pad_size) / CRACEN_WRAP_BLOCK_SIZE;
		status = cracen_wrap(&keyref, data + CRACEN_WRAP_INTEGRITY_CHECK_REG_SIZE,
				     blocks_count, data);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	*data_length = pad_data_size;
	return status;
exit:
	safe_memzero(data, data_size);
	return status;
}

psa_status_t cracen_wrap_key(const psa_key_attributes_t *wrapping_key_attributes,
			     const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			     psa_algorithm_t alg,
			     const psa_key_attributes_t *key_attributes,
			     const uint8_t *key_data, size_t key_size,
			     uint8_t *data, size_t data_size, size_t *data_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_AES_KW) && alg == PSA_ALG_KW) {
		status = cracen_kw_wrap(wrapping_key_attributes, wrapping_key_data,
					wrapping_key_size, key_data, key_size, data, data_size,
					data_length);
	} else if (IS_ENABLED(CONFIG_PSA_NEED_CRACEN_AES_KWP) && alg == PSA_ALG_KWP) {
		status = cracen_kwp_wrap(wrapping_key_attributes, wrapping_key_data,
					 wrapping_key_size, key_data, key_size, data, data_size,
					 data_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}

psa_status_t cracen_unwrap_key(const psa_key_attributes_t *attributes,
			       const psa_key_attributes_t *wrapping_key_attributes,
			       const uint8_t *wrapping_key_data, size_t wrapping_key_size,
			       psa_algorithm_t alg,
			       const uint8_t *data, size_t data_length,
			       uint8_t *key, size_t key_size, size_t *key_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (IS_ENABLED(PSA_NEED_CRACEN_AES_KW) && alg == PSA_ALG_KW) {
		status = cracen_kw_unwrap(wrapping_key_attributes, wrapping_key_data,
					  wrapping_key_size, data, data_length, key, key_size,
					  key_length);
	} else if (IS_ENABLED(CONFIG_PSA_NEED_CRACEN_AES_KWP) && alg == PSA_ALG_KWP) {
		status = cracen_kwp_unwrap(wrapping_key_attributes, wrapping_key_data,
					   wrapping_key_size, data, data_length, key, key_size,
					   key_length);
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
	}

	return status;
}
