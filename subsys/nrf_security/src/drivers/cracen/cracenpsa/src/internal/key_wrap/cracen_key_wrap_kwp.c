/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/key_wrap/cracen_key_wrap_kwp.h>
#include <internal/key_wrap/cracen_key_wrap_common.h>
#include <internal/aes/cracen_aes_ecb.h>

#include <stddef.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <silexpk/core.h>

/* RFC5649 */
#define CRACEN_KWP_MIN_KEY_SIZE			1u
/* Key size for KWP is no more than 2^32 octets */
#define CRACEN_KWP_MAX_KEY_SIZE			(0xFFFFFFFF)
#define CRACEN_KWP_IV_SIZE			(CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE / 2)
#define CRACEN_KWP_MLI_SIZE			(CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE / 2)

/* AES-KW default initial value (RFC5649) */
static const uint8_t cracen_kwp_iv[CRACEN_KWP_IV_SIZE] = {0xA6, 0x59, 0x59, 0xA6};

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

psa_status_t cracen_key_wrap_kwp_wrap(const psa_key_attributes_t *wrapping_key_attributes,
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

	pad_size = CRACEN_KEY_WRAP_BLOCK_SIZE - key_size % CRACEN_KEY_WRAP_BLOCK_SIZE;
	pad_size = pad_size % CRACEN_KEY_WRAP_BLOCK_SIZE;
	if (key_size + pad_size + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE > data_size) {
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
	memmove(data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE, key_data, key_size);
	safe_memzero(data + key_size + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE, pad_size);

	pad_data_size = key_size + pad_size + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE;

	/* NIST.SP.800-38F 6.3 */
	if (key_size <= CRACEN_KEY_WRAP_BLOCK_SIZE) {
		status = cracen_aes_ecb_encrypt(&cipher, &keyref,
					      data, pad_data_size,
					      data, pad_data_size,
					      &length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	} else {
		blocks_count = (key_size + pad_size) / CRACEN_KEY_WRAP_BLOCK_SIZE;
		status = cracen_key_wrap(&keyref, data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE,
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

psa_status_t cracen_key_wrap_kwp_unwrap(const psa_key_attributes_t *wrapping_key_attributes,
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

	if (data_size < (CRACEN_KEY_WRAP_BLOCK_SIZE + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE) ||
	    data_size % CRACEN_KEY_WRAP_BLOCK_SIZE != 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE > key_data_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = cracen_load_keyref(wrapping_key_attributes, wrapping_key_data, wrapping_key_size,
				    &keyref);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	blocks_count =
		(data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE) / CRACEN_KEY_WRAP_BLOCK_SIZE;

	if (blocks_count == 1) {
		status = cracen_aes_ecb_decrypt(&cipher, &keyref,
					      data, data_size,
					      cipher_output_block, SX_BLKCIPHER_AES_BLK_SZ,
					      &length);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		/** RFC5649:
		 *  The ciphertext contains exactly two 64-bit blocks (C[0] and C[1]),
		 *  and they are decrypted as a single AES block to recover the IV and
		 *  the padded plaintext key (A | P[1]).
		 */
		memcpy(key_data,
		       cipher_output_block + CRACEN_KEY_WRAP_BLOCK_SIZE,
		       CRACEN_KEY_WRAP_BLOCK_SIZE);
	} else {
		memcpy(cipher_output_block, data, CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE);
		memmove(key_data,
			data + CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE,
			data_size - CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE);
		status = cracen_key_unwrap(&keyref, cipher_output_block, key_data, blocks_count);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	}

	decode_be_value(&length, CRACEN_KWP_MLI_SIZE, cipher_output_block + CRACEN_KWP_IV_SIZE);
	pad_size = blocks_count * CRACEN_KEY_WRAP_BLOCK_SIZE - length;

	sig_check_fail = constant_memcmp(cipher_output_block,
					 cracen_kwp_iv,
					 CRACEN_KWP_IV_SIZE) != 0;
	sig_check_fail = sig_check_fail || (pad_size > (CRACEN_KEY_WRAP_BLOCK_SIZE - 1));
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
